/*
 * PIZZALIGN - main.c  (interface em raylib; as regras e a matematica ficam no motor.c)
 *
 * Compilar no Windows (MinGW), dentro da pasta do jogo:
 *   gcc main.c motor.c -o pizzalign.exe -I ../../raylib/src -L ../../raylib/src -lraylib -lopengl32 -lgdi32 -lwinmm -lm
 * Compilar no Linux:
 *   gcc main.c motor.c -o pizzalign -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
 *
 * assets_embutidos.h guarda a logo, as cartas e as fontes dentro do codigo
 * (para trocar algum arquivo de assets/, rode gerar_embutidos.py e compile de novo).
 *
 * Dois jogadores se revezam, uma carta cada vez:
 *   Jogador 1 (comeca): A / D = mover | S = descer mais rapido
 *   Jogador 2         : seta esquerda / direita = mover | seta para baixo = descer
 *   P = pausa | M = menu | Q = sair
 *
 * Regras (estao no motor.c): a carta pousa e empilha. Ao pousar, se a vizinha de baixo, da esquerda
 * ou da direita (de qualquer jogador) somar 6 com ela, vale 3 pontos; se for igual, vale 1 ponto.
 * Quando pontua, as duas cartas se fundem e somem; senao, so acumulam. Tomate nunca pontua.
 *
 * VISUAL: tudo segue o traco da logo (chapeu de chef desenhado a canetao):
 *   - contorno grosso de tinta + cor chapada em paineis, botoes, teclas e cartas;
 *   - paleta da bandeira italiana: verde = Jogador 1, vermelho = Jogador 2, papel claro no meio;
 *   - uma fonte so, a Caveat (negrito nos titulos e numeros, seminegrito no texto corrido);
 *   - fundo de toalha xadrez de cantina, desenhado em codigo.
 *
 * Mapa do arquivo: 1) recursos  2) texto  3) pecas da interface  4) telas  5) logica da partida  6) main
 */
#include "raylib.h"
#include "motor.h"
#include "assets_embutidos.h"
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

/* ---------- tela (medidas "logicas": a janela so escala tudo para caber no monitor) ---------- */
#define LARGURA   640
#define ALTURA    980
#define QUAD      36                          /* lado de um quadradinho do tabuleiro */
#define CARTA     (QUAD * CASAS_CARTA)        /* cada carta ocupa 2x2 quadradinhos = 72 px */
#define MESA_L    (larguraTab * CARTA)        /* tabuleiro: 10 x 20 quadradinhos = 360 x 720 px */
#define MESA_A    (alturaTab * CARTA)
#define MESA_X    ((LARGURA - MESA_L) / 2)
#define MESA_Y    180
#define MARGEM    20                          /* respiro das bordas da tela */
#define RODAPE_A  54                          /* faixa de tinta com as teclas, no pe de todas as telas */
#define RODAPE_Y  (ALTURA - RODAPE_A)

/* ---------- hierarquia dos textos (Caveat) ----------
 * A Caveat e uma letra de mao: as minusculas sao pequenas, entao os tamanhos sao maiores que os de uma fonte comum. */
#define T_TITULO  78    /* titulo de tela: Como jogar, Fim de jogo      (negrito) */
#define T_BOTAO   52    /* botoes do menu                               (negrito) */
#define T_SUB     40    /* subtitulos e nomes dos jogadores             (negrito) */
#define T_CORPO   32    /* texto corrido                                (seminegrito) */
#define T_PEQUENO 28    /* legendas e dicas de tecla                    (seminegrito) */
#define MEIO_LETRA 0.54f /* na Caveat, o meio visual das letras fica a 54% da altura da linha */

#define TRACO     3.5f  /* espessura do contorno de tinta dos paineis */

#define ARQUIVO_RECORDES  "recordes.txt"
#define TAM_NOME_DIGITADO 12
#define VIDA_FANTASMA     0.55f
#define COR(r, g, b) ((Color){(r), (g), (b), 255})

/* ---------- paleta: as cores da logo ---------- */
#define TINTA     COR(24, 20, 18)       /* contornos e texto */
#define PAPEL     COR(255, 248, 232)    /* paineis */
#define PAPEL_2   COR(247, 233, 204)    /* papel um tom abaixo (xadrez do tabuleiro, linhas alternadas) */
#define VERDE     COR(71, 137, 101)
#define VERMELHO  COR(176, 42, 4)
#define QUEIJO    COR(246, 196, 69)     /* destaque: de quem e a vez, pontos ganhos */
#define TOALHA    COR(247, 236, 216)

static const Color COR_J[2] = {{71, 137, 101, 255}, {176, 42, 4, 255}};     /* Jogador 1 = verde, Jogador 2 = vermelho */

typedef enum { TELA_MENU, TELA_COMO, TELA_JOGO } Tela;
typedef struct { float x, y, vida; int valor; } Fantasma;          /* carta que acabou de pontuar, sumindo */
typedef struct { int pontos; bool pizza; float x, y, vida; } Popup; /* "+3" / "+1" subindo */
typedef struct { const char *tecla1, *tecla2, *texto; } Dica;      /* uma dica do rodape: tecla(s) + o que faz */
typedef struct {
    float tempo, dispX, dispY, tEsq, tDir;
    float inclina;           /* o chapeu do topo se inclina para o lado de quem joga */
    Fantasma fantasmas[2 * MAX_PARES];
    Popup popups[MAX_PARES];
    char nome[TAM_NOME_DIGITADO + 1];
    int tamNome, vez;        /* vez: 0 = jogador 1, 1 = jogador 2 */
    int pontos[2];           /* pontos de cada jogador (o motor guarda so o total) */
    bool salvo, pausado;
} Partida;

/* ================================================================
 *  1. RECURSOS (tudo vem de assets_embutidos.h)
 * ================================================================ */
static Font fTitulo, fTexto;
static Texture2D spr_carta[9], spr_tomate, spr_logo, spr_mascote;   /* spr_carta[CARTA_CAIXA..CARTA_INTEIRA] */

static Texture2D textura(const unsigned char *dados, int tam) {
    Image img = LoadImageFromMemory(".png", dados, tam);
    if (img.data == NULL) { TraceLog(LOG_WARNING, "PIZZALIGN: uma imagem embutida nao carregou"); return (Texture2D){0}; }
    Texture2D t = LoadTextureFromImage(img);
    UnloadImage(img);
    GenTextureMipmaps(&t);                                   /* a arte e grande: os mipmaps deixam ela lisa quando encolhe */
    SetTextureFilter(t, TEXTURE_FILTER_TRILINEAR);
    return t;
}

static Font fonte(const unsigned char *dados, int tam, int tamanhoBase) {
    int pontos[200], n = 0;                                  /* ASCII + letras acentuadas (Latin-1) */
    for (int c = 32; c < 127; c++) pontos[n++] = c;
    for (int c = 160; c < 256; c++) pontos[n++] = c;
    Font f = LoadFontFromMemory(".ttf", dados, tam, tamanhoBase, pontos, n);
    if (f.texture.id == 0) { TraceLog(LOG_WARNING, "PIZZALIGN: a fonte nao carregou, usando a padrao"); return GetFontDefault(); }
    GenTextureMipmaps(&f.texture);
    SetTextureFilter(f.texture, TEXTURE_FILTER_TRILINEAR);
    return f;
}

static void carregar_recursos(void) {
    spr_carta[CARTA_CAIXA]    = textura(CARTA_CAIXA_PNG, CARTA_CAIXA_PNG_TAM);
    spr_carta[CARTA_FATIA_E]  = textura(CARTA_FATIA_E_PNG, CARTA_FATIA_E_PNG_TAM);
    spr_carta[CARTA_FATIA_D]  = textura(CARTA_FATIA_D_PNG, CARTA_FATIA_D_PNG_TAM);
    spr_carta[CARTA_METADE_E] = textura(CARTA_METADE_E_PNG, CARTA_METADE_E_PNG_TAM);
    spr_carta[CARTA_METADE_D] = textura(CARTA_METADE_D_PNG, CARTA_METADE_D_PNG_TAM);
    spr_carta[CARTA_FALTA_E]  = textura(CARTA_FALTA_E_PNG, CARTA_FALTA_E_PNG_TAM);
    spr_carta[CARTA_FALTA_D]  = textura(CARTA_FALTA_D_PNG, CARTA_FALTA_D_PNG_TAM);
    spr_carta[CARTA_INTEIRA]  = textura(CARTA_INTEIRA_PNG, CARTA_INTEIRA_PNG_TAM);
    spr_tomate  = textura(TOMATE_PNG, TOMATE_PNG_TAM);
    spr_logo    = textura(LOGO_PNG, LOGO_PNG_TAM);
    spr_mascote = textura(MASCOTE_PNG, MASCOTE_PNG_TAM);
    fTitulo = fonte(FONTE_TITULO_TTF, FONTE_TITULO_TTF_TAM, 104);
    fTexto  = fonte(FONTE_TEXTO_TTF, FONTE_TEXTO_TTF_TAM, 64);

    Image icone = LoadImageFromMemory(".png", ICONE_PNG, ICONE_PNG_TAM);     /* icone da janela */
    if (icone.data != NULL) {
        ImageFormat(&icone, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        SetWindowIcon(icone);
        UnloadImage(icone);
    }
}

static void liberar_recursos(void) {
    for (int i = CARTA_CAIXA; i <= CARTA_INTEIRA; i++) UnloadTexture(spr_carta[i]);
    UnloadTexture(spr_tomate); UnloadTexture(spr_logo); UnloadTexture(spr_mascote);
    if (fTitulo.texture.id != GetFontDefault().texture.id) UnloadFont(fTitulo);
    if (fTexto.texture.id != GetFontDefault().texture.id) UnloadFont(fTexto);
}

/* ================================================================
 *  2. TEXTO
 * ================================================================ */
static float mede(Font f, const char *t, float tam) { return MeasureTextEx(f, t, tam, 0).x; }

/* escreve com o meio das letras na altura cy.
 * alinh: -1 = o texto comeca em x | 0 = x e o centro | 1 = o texto termina em x */
static void texto(Font f, const char *t, float x, float cy, float tam, Color cor, int alinh) {
    float larg = mede(f, t, tam);
    float px = (alinh < 0) ? x : (alinh == 0) ? x - larg / 2 : x - larg;
    DrawTextEx(f, t, (Vector2){px, cy - MEIO_LETRA * tam}, tam, 0, cor);
}

/* texto com contorno de tinta em volta (para ler em cima do tabuleiro) */
static void texto_contornado(Font f, const char *t, float x, float cy, float tam, Color cor, float alfa) {
    const float dx[8] = {-1, 0, 1, -1, 1, -1, 0, 1}, dy[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    for (int i = 0; i < 8; i++) texto(f, t, x + dx[i] * 3, cy + dy[i] * 3, tam, Fade(TINTA, alfa), -1);
    texto(f, t, x, cy, tam, Fade(cor, alfa), -1);
}

/* escreve um paragrafo quebrando as linhas; devolve o y logo abaixo da ultima linha */
static float paragrafo(const char *txt, float x, float y, float larg, float tam, Color cor) {
    char linha[512] = "", palavra[64], teste[512];
    float passo = tam * 0.90f;                               /* a Caveat ja tem bastante ar entre as linhas */
    const char *p = txt;
    while (*p) {
        int n = 0;
        while (*p && *p != ' ' && n < 63) palavra[n++] = *p++;
        palavra[n] = '\0';
        while (*p == ' ') p++;
        snprintf(teste, sizeof teste, "%s%s%s", linha, linha[0] ? " " : "", palavra);
        if (mede(fTexto, teste, tam) > larg && linha[0]) {
            texto(fTexto, linha, x, y + passo / 2, tam, cor, -1);
            y += passo;
            snprintf(linha, sizeof linha, "%s", palavra);
        } else {
            snprintf(linha, sizeof linha, "%s", teste);
        }
    }
    if (linha[0]) { texto(fTexto, linha, x, y + passo / 2, tam, cor, -1); y += passo; }
    return y;
}

/* ================================================================
 *  3. PECAS DA INTERFACE (as mesmas em todas as telas)
 * ================================================================ */
static void retangulo(Rectangle r, float raio, Color cor) {
    float menor = fminf(r.width, r.height);
    if (menor <= 0) return;
    DrawRectangleRounded(r, fminf(1.0f, fmaxf(0.0f, 2 * raio / menor)), 12, cor);
}

/* painel no traco da logo: contorno de tinta, cor chapada e uma "espessura" de tinta embaixo */
static void caixa(Rectangle r, float raio, Color fundo, float espessura) {
    if (espessura > 0) retangulo((Rectangle){r.x, r.y + espessura, r.width, r.height}, raio, TINTA);
    retangulo(r, raio, TINTA);
    retangulo((Rectangle){r.x + TRACO, r.y + TRACO, r.width - 2 * TRACO, r.height - 2 * TRACO}, raio - TRACO, fundo);
}

/* desenha uma tecla do teclado e devolve a largura dela. "<" ">" "v" viram setas. */
static float tecla(const char *rotulo, float x, float cy, float alt) {
    bool seta = (rotulo[1] == '\0') && (rotulo[0] == '<' || rotulo[0] == '>' || rotulo[0] == 'v');
    float tam = alt * 0.96f;
    float larg = seta ? alt : fmaxf(alt, mede(fTitulo, rotulo, tam) + alt * 0.62f);
    Rectangle r = {x, cy - alt / 2 - 1, larg, alt};
    retangulo((Rectangle){r.x, r.y + 3, r.width, r.height}, 7, TINTA);
    retangulo(r, 7, TINTA);
    retangulo((Rectangle){r.x + 2, r.y + 2, r.width - 4, r.height - 4}, 5, PAPEL);
    if (seta) {
        float giro = (rotulo[0] == '>') ? 0 : (rotulo[0] == 'v') ? 90 : 180;
        DrawPoly((Vector2){r.x + r.width / 2, r.y + r.height / 2}, 3, alt * 0.27f, giro, TINTA);
    } else {
        texto(fTitulo, rotulo, r.x + r.width / 2, r.y + r.height / 2, tam, TINTA, 0);
    }
    return larg;
}

/* faixa de tinta no pe da tela com as teclas que valem agora */
static void desenhar_rodape(const Dica *dicas, int n) {
    const float alt = 30, entre = 30;
    float total = 0;
    for (int i = 0; i < n; i++) {
        total += fmaxf(alt, mede(fTitulo, dicas[i].tecla1, alt * 0.96f) + alt * 0.62f) + 8 + mede(fTexto, dicas[i].texto, T_PEQUENO);
        if (dicas[i].tecla2) total += fmaxf(alt, mede(fTitulo, dicas[i].tecla2, alt * 0.96f) + alt * 0.62f) + 5;
        if (i < n - 1) total += entre;
    }
    DrawRectangle(0, RODAPE_Y, LARGURA, RODAPE_A, TINTA);
    float x = (LARGURA - total) / 2, cy = RODAPE_Y + RODAPE_A / 2.0f;
    for (int i = 0; i < n; i++) {
        x += tecla(dicas[i].tecla1, x, cy, alt);
        if (dicas[i].tecla2) x += 5 + tecla(dicas[i].tecla2, x + 5, cy, alt);
        x += 8;
        texto(fTexto, dicas[i].texto, x, cy, T_PEQUENO, PAPEL, -1);
        x += mede(fTexto, dicas[i].texto, T_PEQUENO) + entre;
    }
}

/* sublinhado ondulado, como um risco de caneta */
static void rabisco(float x, float y, float larg, Color cor) {
    Vector2 antes = {x, y};
    for (float dx = 6; dx <= larg; dx += 6) {
        Vector2 p = {x + dx, y + 3.2f * sinf(dx * 0.16f)};
        DrawLineEx(antes, p, 4, cor);
        DrawCircleV(p, 2, cor);
        antes = p;
    }
}

static void tracejado(float x, float y, float larg) {
    for (float dx = 0; dx + 10 <= larg; dx += 18) DrawRectangleRounded((Rectangle){x + dx, y, 10, 3}, 1, 4, Fade(TINTA, 0.35f));
}

/* selo redondo de pontos: "+3" amarelo, "+1" claro */
static void selo(float cx, float cy, int pontos) {
    DrawCircleV((Vector2){cx, cy + 2}, 23, TINTA);
    DrawCircleV((Vector2){cx, cy}, 23, TINTA);
    DrawCircleV((Vector2){cx, cy}, 19.5f, pontos >= PONTOS_PIZZA ? QUEIJO : PAPEL_2);
    texto(fTitulo, TextFormat("+%d", pontos), cx - 1, cy, 36, TINTA, 0);
}

static void desenhar_textura(Texture2D t, float x, float y, float tam, float alfa) {
    DrawTexturePro(t, (Rectangle){0, 0, (float)t.width, (float)t.height}, (Rectangle){x, y, tam, tam},
                   (Vector2){0, 0}, 0, Fade(WHITE, alfa));
}

/* carta: CARTA_CAIXA..CARTA_INTEIRA = os desenhos das pizzas, TOMATE = bloco */
static void desenhar_carta(int valor, float x, float y, float tam, float alfa) {
    Texture2D t = (valor >= TOMATE) ? spr_tomate : spr_carta[valor < CARTA_CAIXA ? CARTA_CAIXA : valor];
    if (t.id == 0) { DrawRectangle((int)x, (int)y, (int)tam, (int)tam, Fade(QUEIJO, alfa)); return; }
    desenhar_textura(t, x, y, tam, alfa);
}

/* logo e mascote sao "adesivos": a imagem girada em torno do centro, com uma sombra embaixo */
static void desenhar_adesivo(Texture2D t, float cx, float cy, float larg, float graus) {
    if (t.id == 0) { texto(fTitulo, "Pizzalign", cx, cy, T_TITULO, TINTA, 0); return; }
    float alt = larg * t.height / t.width;
    Rectangle src = {0, 0, (float)t.width, (float)t.height};
    Vector2 centro = {larg / 2, alt / 2};
    DrawTexturePro(t, src, (Rectangle){cx, cy + larg * 0.022f + 2, larg, alt}, centro, graus, Fade(TINTA, 0.40f));
    DrawTexturePro(t, src, (Rectangle){cx, cy, larg, alt}, centro, graus, WHITE);
}

/* toalha xadrez de cantina: faixas vermelhas transparentes na vertical e na horizontal.
 * Onde duas faixas se cruzam o vermelho soma e fica mais forte, igual ao tecido de verdade. */
static void desenhar_fundo(void) {
    const int faixa = 40;
    DrawRectangle(0, 0, LARGURA, ALTURA, TOALHA);
    for (int x = 0; x < LARGURA; x += 2 * faixa) DrawRectangle(x, 0, faixa, ALTURA, Fade(VERMELHO, 0.21f));
    for (int y = 0; y < ALTURA; y += 2 * faixa) DrawRectangle(0, y, LARGURA, faixa, Fade(VERMELHO, 0.21f));
    DrawRectangleGradientV(0, 0, LARGURA, 260, Fade(TINTA, 0.20f), Fade(TINTA, 0.0f));          /* escurece as pontas: */
    DrawRectangleGradientV(0, ALTURA - 320, LARGURA, 320, Fade(TINTA, 0.0f), Fade(TINTA, 0.26f)); /* o olho vai para o meio */
}

/* ================================================================
 *  4. TELAS
 * ================================================================ */
static int linha_de_pouso(const EstadoJogo *j) {
    int y = j->fatiaAtual.y;
    while (y + 1 < alturaTab && j->tab[y + 1][j->fatiaAtual.x] == VAZIA) y++;
    return y;
}

/* tabuleiro: folha de papel com um xadrez discreto de 10 x 20 quadradinhos; cada carta ocupa 2 x 2 deles */
static void desenhar_mesa(const EstadoJogo *jogo, float dispX, float dispY, int vez) {
    caixa((Rectangle){MESA_X - 8, MESA_Y - 8, MESA_L + 16, MESA_A + 16}, 14, PAPEL, 7);
    for (int r = 0; r < alturaTab * CASAS_CARTA; r++)
        for (int c = 0; c < larguraTab * CASAS_CARTA; c++)
            if ((r + c) % 2 == 0) DrawRectangle(MESA_X + c * QUAD, MESA_Y + r * QUAD, QUAD, QUAD, PAPEL_2);

    if (!jogo->gameOver)                                     /* coluna de quem joga, na cor do jogador */
        DrawRectangle(MESA_X + jogo->fatiaAtual.x * CARTA, MESA_Y, CARTA, MESA_A, Fade(COR_J[vez], 0.20f));

    for (int r = 0; r < alturaTab; r++)
        for (int c = 0; c < larguraTab; c++)
            if (jogo->tab[r][c] != VAZIA)
                desenhar_carta(jogo->tab[r][c], (float)(MESA_X + c * CARTA), (float)(MESA_Y + r * CARTA), CARTA, 1.0f);

    if (!jogo->gameOver) {
        int x = jogo->fatiaAtual.x, v = jogo->fatiaAtual.pedacos;
        desenhar_carta(v, (float)(MESA_X + x * CARTA), (float)(MESA_Y + linha_de_pouso(jogo) * CARTA), CARTA, 0.28f);   /* onde vai pousar */
        desenhar_carta(v, MESA_X + dispX * CARTA, MESA_Y + dispY * CARTA, CARTA, 1.0f);                                   /* carta caindo */
    }
}

/* cartao de um jogador no topo. O de quem esta na vez fica colorido, levantado e com a etiqueta "sua vez". */
static void desenhar_jogador(int j, const Partida *pa, bool fim) {
    const float larg = 196, alt = 108;
    bool ativo = (pa->vez == j) && !fim;
    Rectangle r = {(j == 0) ? MARGEM : LARGURA - MARGEM - larg, ativo ? 26 : 31, larg, alt};
    caixa(r, 16, ativo ? COR_J[j] : PAPEL, ativo ? 7 : 2);

    texto(fTitulo, TextFormat("Jogador %d", j + 1), r.x + 16, r.y + 28, 36, ativo ? PAPEL : COR_J[j], -1);
    texto(fTitulo, TextFormat("%d", pa->pontos[j]), r.x + larg - 18, r.y + 58, pa->pontos[j] < 100 ? 92 : 62,   /* 3 digitos: encolhe para caber */
          ativo ? PAPEL : Fade(TINTA, 0.78f), 1);

    float kx = r.x + 16, ky = r.y + 76;                      /* as teclas deste jogador */
    kx += tecla(j ? "<" : "A", kx, ky, 26) + 4;
    kx += tecla(j ? "v" : "S", kx, ky, 26) + 4;
    tecla(j ? ">" : "D", kx, ky, 26);

    if (ativo) {
        Rectangle e = {r.x + (larg - 104) / 2, r.y + alt - 9, 104, 34};
        caixa(e, 17, QUEIJO, 0);
        texto(fTitulo, "sua vez", e.x + e.width / 2, e.y + e.height / 2 - 1, 30, TINTA, 0);
    }
}

/* lateral esquerda: nivel (com o progresso ate o proximo) e a cola de quanto vale cada par */
static void desenhar_lateral_esquerda(const EstadoJogo *jogo, int nivel) {
    Rectangle n = {MARGEM, MESA_Y - 8, 100, 124};
    caixa(n, 14, PAPEL, 5);
    float cx = n.x + n.width / 2;
    texto(fTexto, "nível", cx, n.y + 24, T_PEQUENO, Fade(TINTA, 0.75f), 0);
    texto(fTitulo, TextFormat("%d", nivel), cx, n.y + 66, 72, TINTA, 0);
    for (int i = 0; i < 10; i++)                             /* 10 pontos = 1 nivel */
        DrawRectangleRounded((Rectangle){n.x + 13 + i * 7.5f, n.y + 102, 5.5f, 9}, 0.8f, 4,
                             i < jogo->pontuacao % 10 ? VERMELHO : Fade(TINTA, 0.18f));

    Rectangle c = {MARGEM, n.y + n.height + 22, 100, 162};
    caixa(c, 14, PAPEL, 5);
    desenhar_carta(CARTA_METADE_E, c.x + 9, c.y + 14, 30, 1);
    desenhar_carta(CARTA_METADE_D, c.x + 33, c.y + 14, 30, 1);
    texto(fTitulo, "+3", c.x + c.width - 12, c.y + 29, 38, TINTA, 1);
    texto(fTexto, "completou", cx, c.y + 60, T_PEQUENO - 3, Fade(TINTA, 0.75f), 0);
    tracejado(c.x + 14, c.y + 80, c.width - 28);
    desenhar_carta(CARTA_FATIA_D, c.x + 7, c.y + 90, 30, 1);
    desenhar_carta(CARTA_FATIA_D, c.x + 31, c.y + 90, 30, 1);
    texto(fTitulo, "+1", c.x + c.width - 12, c.y + 105, 38, TINTA, 1);
    texto(fTexto, "iguais", cx, c.y + 136, T_PEQUENO - 3, Fade(TINTA, 0.75f), 0);
}

/* lateral direita: as proximas cartas. A primeira (a proxima a cair) e maior e destacada. */
static void desenhar_proximas(const EstadoJogo *jogo) {
    Rectangle p = {LARGURA - MARGEM - 100, MESA_Y - 8, 100, 268};
    caixa(p, 14, PAPEL, 5);
    texto(fTexto, "próximas", p.x + p.width / 2, p.y + 24, T_PEQUENO, Fade(TINTA, 0.75f), 0);
    retangulo((Rectangle){p.x + 11, p.y + 44, 78, 78}, 12, Fade(QUEIJO, 0.55f));
    const NoFatia *n = jogo->filaFatias;
    for (int i = 0; i < 3 && n; i++, n = n->proximo) {
        if (i == 0) desenhar_carta(n->pedacos, p.x + 16, p.y + 49, 68, 1.0f);
        else        desenhar_carta(n->pedacos, p.x + 24, p.y + 136 + (i - 1) * 62, 52, 0.85f);
    }
}

/* "+3" com a pizza (completou) ou "+1" (iguais), subindo */
static void desenhar_popup(const Popup *p) {
    float a = fminf(1.0f, p->vida * 1.6f);
    float tam = 64 + 26 * fmaxf(0.0f, (p->vida - 1.25f) / 0.15f);            /* nasce maior e assenta */
    const char *t = TextFormat("+%d", p->pontos);
    float larg = mede(fTitulo, t, tam) + (p->pizza ? 50 : 0);
    float x = p->x + (CARTA - larg) / 2, cy = p->y + 30;
    if (p->pizza) { desenhar_textura(spr_carta[CARTA_INTEIRA], x, cy - 23, 46, a); x += 50; }
    texto_contornado(fTitulo, t, x, cy, tam, p->pizza ? QUEIJO : PAPEL, a);
}

static Rectangle botao_menu(int i) { return (Rectangle){150, 584.0f + i * 100, 340, 78}; }

static void desenhar_menu(int opcao, const Recorde *recordes) {
    float t = (float)GetTime();
    desenhar_adesivo(spr_logo, LARGURA / 2.0f, 292 + 4 * sinf(t * 1.6f), 372, -3 + 1.2f * sinf(t * 0.9f));

    const char *nomes[2] = {"Novo jogo", "Como jogar"};
    for (int i = 0; i < 2; i++) {
        Rectangle b = botao_menu(i);
        bool sel = (i == opcao);
        if (sel) b.y -= 4;                                   /* o escolhido sobe e ganha cor */
        caixa(b, 20, sel ? VERMELHO : PAPEL, sel ? 10 : 6);
        texto(fTitulo, nomes[i], b.x + b.width / 2, b.y + b.height / 2, T_BOTAO, sel ? PAPEL : TINTA, 0);
        if (sel) desenhar_carta(CARTA_FATIA_E, b.x - 92 + 5 * sinf(t * 5), b.y + b.height / 2 - 42, 84, 1);   /* fatia apontando */
    }

    if (recordes) {                                          /* melhor placar salvo */
        const char *r = TextFormat("recorde: %s, %d pontos", recordes->nome, recordes->pontos);
        float larg = mede(fTexto, r, T_CORPO) + 52;
        Rectangle e = {(LARGURA - larg) / 2, 806, larg, 46};
        caixa(e, 23, PAPEL, 4);
        texto(fTexto, r, LARGURA / 2.0f, e.y + e.height / 2, T_CORPO, TINTA, 0);
    }

    Dica dicas[3] = {{"W", "S", "escolher"}, {"Enter", NULL, "confirmar"}, {"Q", NULL, "sair"}};
    desenhar_rodape(dicas, 3);
}

/* uma linha de exemplo do "Como jogar": carta + carta, o selo de pontos e a explicacao */
static float linha_exemplo(int a, int b, int pontos, const char *explica, float x, float y) {
    const float ts = 50;
    float cy = y + ts / 2;
    desenhar_carta(a, x, y, ts, 1);
    texto(fTitulo, "+", x + 64, cy, T_SUB, TINTA, 0);
    desenhar_carta(b, x + 78, y, ts, 1);
    selo(x + 166, cy, pontos);
    texto(fTexto, explica, x + 204, cy, T_CORPO, TINTA, -1);
    return y + ts + 8;
}

static void desenhar_como_jogar(void) {
    Rectangle p = {30, 22, 580, 874};
    caixa(p, 22, PAPEL, 8);
    float cx = LARGURA / 2.0f, x = p.x + 34, l = p.width - 68;
    texto(fTitulo, "Como jogar", cx, p.y + 54, T_TITULO, TINTA, 0);
    rabisco(cx - 116, p.y + 94, 232, VERMELHO);

    /* controles: um cartao por jogador, na cor dele */
    float y = p.y + 120, lc = (l - 16) / 2;
    for (int j = 0; j < 2; j++) {
        Rectangle c = {x + j * (lc + 16), y, lc, 98};
        caixa(c, 14, COR_J[j], 0);
        texto(fTitulo, TextFormat("Jogador %d", j + 1), c.x + 16, c.y + 27, T_SUB, PAPEL, -1);
        float kx = c.x + 16, ky = c.y + 68;
        kx += tecla(j ? "<" : "A", kx, ky, 28) + 4;
        kx += tecla(j ? ">" : "D", kx, ky, 28) + 7;
        texto(fTexto, "mover", kx, ky, T_PEQUENO, PAPEL, -1);
        kx += mede(fTexto, "mover", T_PEQUENO) + 14;
        kx += tecla(j ? "v" : "S", kx, ky, 28) + 7;
        texto(fTexto, "descer", kx, ky, T_PEQUENO, PAPEL, -1);
    }
    y = paragrafo("Cada um joga uma carta e passa a vez.", x, y + 98 + 10, l, T_CORPO, TINTA);

    texto(fTitulo, "Como pontuar", x, y + 32, T_SUB, TINTA, -1);
    y = paragrafo("Quando a carta pousa, ela combina com a vizinha de baixo ou dos lados, de qualquer jogador. "
                  "As duas somem e as de cima descem.", x, y + 56, l, T_CORPO, TINTA);
    y += 10;
    y = linha_exemplo(CARTA_METADE_E, CARTA_METADE_D, PONTOS_PIZZA, "as duas metades", x, y);
    y = linha_exemplo(CARTA_FATIA_E, CARTA_FALTA_E, PONTOS_PIZZA, "a fatia que falta, do mesmo lado", x, y);
    y = linha_exemplo(CARTA_INTEIRA, CARTA_CAIXA, PONTOS_PIZZA, "a pizza inteira e a caixa vazia", x, y);
    y = linha_exemplo(CARTA_FATIA_E, CARTA_FATIA_E, PONTOS_IGUAL, "duas cartas iguais", x, y);

    texto(fTitulo, "Fique de olho", x, y + 30, T_SUB, TINTA, -1);
    y += 56;
    desenhar_carta(TOMATE, x, y + 2, 50, 1);
    float yt = paragrafo("O tomate não combina com nada, só ocupa espaço. Ele aparece mais conforme os pontos sobem.",
                         x + 64, y, l - 64, T_CORPO, TINTA);
    paragrafo("Depois de 30 pontos a queda acelera. Quando o topo enche, vence quem tiver mais pontos.",
              x, fmaxf(yt, y + 56) + 8, l, T_CORPO, TINTA);

    Dica dicas[1] = {{"Enter", NULL, "voltar ao menu"}};
    desenhar_rodape(dicas, 1);
}

static int vencedor(const Partida *pa) {
    if (pa->pontos[0] == pa->pontos[1]) return -1;
    return (pa->pontos[0] > pa->pontos[1]) ? 0 : 1;
}

static void desenhar_pausa(void) {
    DrawRectangle(0, 0, LARGURA, ALTURA, Fade(TINTA, 0.60f));
    Rectangle p = {140, 380, 360, 168};
    caixa(p, 22, PAPEL, 8);
    texto(fTitulo, "Pausado", LARGURA / 2.0f, p.y + 62, T_TITULO, TINTA, 0);
    texto(fTexto, "o forno espera por vocês", LARGURA / 2.0f, p.y + 122, T_CORPO, Fade(TINTA, 0.75f), 0);
    Dica dicas[3] = {{"P", NULL, "continuar"}, {"M", NULL, "menu"}, {"Q", NULL, "sair"}};
    desenhar_rodape(dicas, 3);
}

static void desenhar_fim(const Recorde *recordes, const Partida *pa) {
    int v = vencedor(pa);
    float cx = LARGURA / 2.0f;
    DrawRectangle(0, 0, LARGURA, ALTURA, Fade(TINTA, 0.60f));
    int linhas = 0;                                          /* quantas linhas do ranking vao aparecer (ate 5) */
    for (const Recorde *r = recordes; r && linhas < 5; r = r->prox) linhas++;
    Rectangle p = {60, 0, 520, pa->salvo ? 356.0f + linhas * 50 : 468.0f};
    p.y = (RODAPE_Y - p.height) / 2 - 6;
    caixa(p, 22, PAPEL, 8);
    float x = p.x + 44, l = p.width - 88;

    texto(fTitulo, "Fim de jogo", cx, p.y + 56, T_TITULO, TINTA, 0);

    Rectangle faixa = {x, p.y + 104, l, 62};                 /* quem venceu, na cor de quem venceu */
    caixa(faixa, 31, v < 0 ? QUEIJO : COR_J[v], 0);
    texto(fTitulo, v < 0 ? "Empate!" : TextFormat("Jogador %d venceu!", v + 1), cx, faixa.y + faixa.height / 2 - 1,
          46, v < 0 ? TINTA : PAPEL, 0);

    float py = p.y + 212;                                    /* placar: nome e pontos de cada lado do x */
    texto(fTitulo, "x", cx, py, 34, Fade(TINTA, 0.55f), 0);
    texto(fTitulo, TextFormat("%d", pa->pontos[0]), cx - 26, py, 70, TINTA, 1);
    texto(fTitulo, TextFormat("%d", pa->pontos[1]), cx + 26, py, 70, TINTA, -1);
    texto(fTitulo, "Jogador 1", x, py, 34, COR_J[0], -1);
    texto(fTitulo, "Jogador 2", x + l, py, 34, COR_J[1], 1);
    tracejado(x, p.y + 258, l);

    if (!pa->salvo) {
        texto(fTexto, v < 0 ? "Digite um nome para o ranking" : "Nome de quem venceu, para o ranking", x, p.y + 296, T_CORPO, TINTA, -1);
        Rectangle campo = {x, p.y + 322, l, 68};
        caixa(campo, 14, WHITE, 0);
        float cyc = campo.y + campo.height / 2;
        texto(fTitulo, pa->nome, campo.x + 20, cyc, 48, TINTA, -1);
        if (((int)(GetTime() * 2)) % 2 == 0)                 /* cursor piscando */
            DrawRectangleRounded((Rectangle){campo.x + 24 + mede(fTitulo, pa->nome, 48), cyc - 18, 4, 36}, 1, 4, VERMELHO);
        texto(fTexto, TextFormat("até %d letras ou números", TAM_NOME_DIGITADO), x, p.y + 418, T_PEQUENO, Fade(TINTA, 0.7f), -1);
        Dica dicas[1] = {{"Enter", NULL, "salvar"}};
        desenhar_rodape(dicas, 1);
    } else {
        texto(fTitulo, "Ranking", x, p.y + 298, T_SUB, TINTA, -1);
        int i = 0;
        for (const Recorde *r = recordes; r && i < 5; r = r->prox, i++) {
            Rectangle linha = {x - 12, p.y + 330 + i * 50, l + 24, 44};
            if (i == 0) retangulo(linha, 22, Fade(QUEIJO, 0.65f));           /* o primeiro lugar em destaque */
            else if (i % 2 == 0) retangulo(linha, 22, PAPEL_2);
            float cyl = linha.y + linha.height / 2;
            texto(fTitulo, TextFormat("%d", i + 1), x + 8, cyl, 34, Fade(TINTA, 0.6f), -1);
            texto(fTexto, r->nome, x + 44, cyl, 32, TINTA, -1);
            texto(fTitulo, TextFormat("%d", r->pontos), x + l - 4, cyl, 38, TINTA, 1);
        }
        Dica dicas[3] = {{"R", NULL, "jogar de novo"}, {"M", NULL, "menu"}, {"Q", NULL, "sair"}};
        desenhar_rodape(dicas, 3);
    }
}

/* ================================================================
 *  5. LOGICA DA PARTIDA (chama o motor)
 * ================================================================ */
static void nova_partida(Partida *p, EstadoJogo *jogo) {
    iniciarJogo(jogo);
    memset(p, 0, sizeof *p);
    p->dispX = (float)jogo->fatiaAtual.x;
}

/* ate 30 pontos a queda e fixa (0,55 s por linha); depois fica um pouquinho mais rapida a cada ponto */
static float intervalo_de_queda(int pontos) {
    float t = 0.55f;
    if (pontos > 30) t -= 0.004f * (pontos - 30);
    return t < 0.18f ? 0.18f : t;
}

/* tomates: o motor so cria cartas de 1 a 5, entao trocamos a ultima da fila (a recem-criada) por um tomate.
 * Eles so aparecem depois de 10 pontos e vao ficando mais comuns (no maximo 20%). */
static void sortear_tomate(EstadoJogo *jogo) {
    int chance = (jogo->pontuacao < 10) ? 0 : 2 + 2 * (jogo->pontuacao / 10);
    if (chance > 20) chance = 20;
    if (rand() % 100 >= chance) return;
    NoFatia *n = jogo->filaFatias;
    while (n && n->proximo) n = n->proximo;
    if (n) n->pedacos = TOMATE;
}

/* mostra os efeitos dos pares que pontuaram na ultima carta que pousou */
static void registrar_efeitos(Partida *pa, const EstadoJogo *jogo) {
    for (int i = 0; i < jogo->numPares; i++) {
        const Par *p = &jogo->pares[i];
        float x1 = (float)(MESA_X + p->col1 * CARTA), y1 = (float)(MESA_Y + p->lin1 * CARTA);
        float x2 = (float)(MESA_X + p->col2 * CARTA), y2 = (float)(MESA_Y + p->lin2 * CARTA);
        for (int k = 0; k < 2 * MAX_PARES; k++)
            if (pa->fantasmas[k].vida <= 0) { pa->fantasmas[k] = (Fantasma){x1, y1, VIDA_FANTASMA, p->valor1}; break; }
        for (int k = 0; k < 2 * MAX_PARES; k++)
            if (pa->fantasmas[k].vida <= 0) { pa->fantasmas[k] = (Fantasma){x2, y2, VIDA_FANTASMA, p->valor2}; break; }
        for (int k = 0; k < MAX_PARES; k++)
            if (pa->popups[k].vida <= 0) {
                pa->popups[k] = (Popup){p->pontos, p->pontos == PONTOS_PIZZA, (x1 + x2) / 2, (y1 + y2) / 2, 1.4f};
                break;
            }
    }
}

/* uma descida. Quando a carta pousa: o motor pontua, o ponto vai para quem jogou e passa a vez */
static void avancar(Partida *pa, EstadoJogo *jogo) {
    int antes = jogo->pecasFixadas;
    descer_peca(jogo);
    if (jogo->pecasFixadas == antes) return;                 /* so desceu uma linha */

    pa->pontos[pa->vez] += jogo->ganhoUltima;
    registrar_efeitos(pa, jogo);
    sortear_tomate(jogo);
    pa->vez = 1 - pa->vez;
    pa->tEsq = pa->tDir = 0;
    pa->dispX = (float)jogo->fatiaAtual.x;                   /* carta nova nao desliza */
}

static bool tecla_com_repeticao(int tecla, float *t, float dt) {
    if (IsKeyPressed(tecla)) { *t = 0; return true; }
    if (!IsKeyDown(tecla)) { *t = 0; return false; }
    *t += dt;
    if (*t >= 0.22f) { *t -= 0.06f; return true; }
    return false;
}

static void atualizar_partida(Partida *pa, EstadoJogo *jogo, float dt, float intervalo) {
    if (IsKeyPressed(KEY_P)) pa->pausado = !pa->pausado;
    if (pa->pausado) return;

    /* so as teclas de quem esta na vez valem: J1 = A D S, J2 = as tres setas */
    int kEsq = pa->vez ? KEY_LEFT : KEY_A, kDir = pa->vez ? KEY_RIGHT : KEY_D, kBaixo = pa->vez ? KEY_DOWN : KEY_S;
    if (tecla_com_repeticao(kEsq, &pa->tEsq, dt)) mover_peca(jogo, -1, 0);
    if (tecla_com_repeticao(kDir, &pa->tDir, dt)) mover_peca(jogo, 1, 0);

    pa->tempo += dt;
    float passo = IsKeyDown(kBaixo) ? fminf(intervalo, 0.05f) : intervalo;
    if (pa->tempo >= passo) { pa->tempo = 0; avancar(pa, jogo); }
}

static void atualizar_nome(Partida *pa, Recorde **recordes) {
    int ch;
    while ((ch = GetCharPressed()) > 0)
        if (ch < 128 && isalnum(ch) && pa->tamNome < TAM_NOME_DIGITADO) {
            pa->nome[pa->tamNome++] = (char)toupper(ch);
            pa->nome[pa->tamNome] = '\0';
        }
    if (IsKeyPressed(KEY_BACKSPACE) && pa->tamNome > 0) pa->nome[--pa->tamNome] = '\0';
    if (IsKeyPressed(KEY_ENTER)) {
        int v = vencedor(pa), pts = (v < 1) ? pa->pontos[0] : pa->pontos[1];   /* empate: os dois valem o mesmo */
        recorde_inserir(recordes, pa->tamNome ? pa->nome : "JOGADOR", pts);
        recordes_salvar(*recordes, ARQUIVO_RECORDES);
        pa->salvo = true;
    }
}

/* ================================================================
 *  6. MAIN
 * ================================================================ */
/* a janela e vertical e pode ter qualquer tamanho: o jogo escala tudo mantendo a proporcao */
static Camera2D camera_ajustada(void) {
    float z = fminf((float)GetScreenWidth() / LARGURA, (float)GetScreenHeight() / ALTURA);
    Camera2D c = {0};
    c.zoom = z;
    c.offset = (Vector2){(GetScreenWidth() - LARGURA * z) / 2, (GetScreenHeight() - ALTURA * z) / 2};
    return c;
}

int main(void) {
    srand((unsigned)time(NULL));
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);   /* MSAA: cantos arredondados lisos */
    InitWindow(LARGURA / 2, ALTURA / 2, "Pizzalign");
    SetExitKey(KEY_NULL);                          /* so o Q (ou fechar a janela) encerra */
    SetTargetFPS(60);
    ChangeDirectory(GetApplicationDirectory());    /* recordes.txt fica ao lado do executavel */

    int mon = GetCurrentMonitor();                 /* janela vertical que cabe na tela */
    float z = (GetMonitorHeight(mon) > 0) ? GetMonitorHeight(mon) * 0.88f / ALTURA : 0.75f;
    if (z > 1.4f) z = 1.4f;
    if (z < 0.5f) z = 0.5f;
    SetWindowSize((int)(LARGURA * z), (int)(ALTURA * z));
    Vector2 mp = GetMonitorPosition(mon);
    SetWindowPosition((int)(mp.x + (GetMonitorWidth(mon) - LARGURA * z) / 2), (int)(mp.y + (GetMonitorHeight(mon) - ALTURA * z) / 2 - 20));
    SetWindowMinSize(LARGURA / 2, ALTURA / 2);

    carregar_recursos();

    EstadoJogo *jogo = estado_criar();
    if (!jogo) return 1;
    Partida pa;
    nova_partida(&pa, jogo);
    Recorde *recordes = recordes_carregar(ARQUIVO_RECORDES);

    Tela tela = TELA_MENU;
    int opcao = 0;
    bool sair = false;

    while (!sair && !WindowShouldClose()) {
        float dt = GetFrameTime();
        Camera2D cam = camera_ajustada();
        bool digitando = (tela == TELA_JOGO && jogo->gameOver && !pa.salvo);
        int nivel = jogo->pontuacao / 10 + 1;
        float intervalo = intervalo_de_queda(jogo->pontuacao);

        if (IsKeyPressed(KEY_Q) && !digitando) sair = true;

        /* ----- atualizar ----- */
        if (tela == TELA_MENU) {
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) opcao = 1 - opcao;
            Vector2 m = GetScreenToWorld2D(GetMousePosition(), cam), mv = GetMouseDelta();
            bool clicou = false;
            for (int i = 0; i < 2; i++)
                if (CheckCollisionPointRec(m, botao_menu(i))) {
                    if (mv.x != 0 || mv.y != 0) opcao = i;
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { opcao = i; clicou = true; }
                }
            if (clicou || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                if (opcao == 0) { nova_partida(&pa, jogo); tela = TELA_JOGO; }
                else tela = TELA_COMO;
            }
        } else if (tela == TELA_COMO) {
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_M) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_SPACE)) tela = TELA_MENU;
        } else {
            if (IsKeyPressed(KEY_M) && !digitando) tela = TELA_MENU;
            else if (!jogo->gameOver) atualizar_partida(&pa, jogo, dt, intervalo);
            else if (digitando) atualizar_nome(&pa, &recordes);
            else if (IsKeyPressed(KEY_R)) nova_partida(&pa, jogo);

            /* animacao suave da carta (so visual; a posicao real e a do motor) */
            pa.dispX += ((float)jogo->fatiaAtual.x - pa.dispX) * fminf(1.0f, 20.0f * dt);
            float frac = (!pa.pausado && !jogo->gameOver && pode_mover(jogo, 0, 1)) ? fminf(1.0f, pa.tempo / intervalo) : 0;
            pa.dispY = jogo->fatiaAtual.y + frac;
            float alvo = jogo->gameOver ? 0.0f : (pa.vez ? 6.0f : -6.0f);    /* o chapeu olha para quem joga */
            pa.inclina += (alvo - pa.inclina) * fminf(1.0f, 9.0f * dt);
            for (int i = 0; i < 2 * MAX_PARES; i++) if (pa.fantasmas[i].vida > 0) pa.fantasmas[i].vida -= dt;
            for (int i = 0; i < MAX_PARES; i++)
                if (pa.popups[i].vida > 0) { pa.popups[i].vida -= dt; pa.popups[i].y -= 40 * dt; }
        }

        /* ----- desenhar ----- */
        BeginDrawing();
        ClearBackground(TINTA);
        BeginMode2D(cam);
        desenhar_fundo();
        if (tela == TELA_MENU) {
            desenhar_menu(opcao, recordes);
        } else if (tela == TELA_COMO) {
            desenhar_como_jogar();
        } else {
            desenhar_jogador(0, &pa, jogo->gameOver);
            desenhar_jogador(1, &pa, jogo->gameOver);
            desenhar_adesivo(spr_mascote, LARGURA / 2.0f, 82, 138, pa.inclina);
            desenhar_mesa(jogo, pa.dispX, pa.dispY, pa.vez);
            desenhar_lateral_esquerda(jogo, nivel);
            desenhar_proximas(jogo);

            for (int i = 0; i < 2 * MAX_PARES; i++)                  /* cartas que pontuaram: crescem e somem */
                if (pa.fantasmas[i].vida > 0) {
                    float t = pa.fantasmas[i].vida / VIDA_FANTASMA, tam = CARTA * (1 + (1 - t) * 0.35f);
                    desenhar_carta(pa.fantasmas[i].valor, pa.fantasmas[i].x - (tam - CARTA) / 2, pa.fantasmas[i].y - (tam - CARTA) / 2, tam, t);
                }
            for (int i = 0; i < MAX_PARES; i++) if (pa.popups[i].vida > 0) desenhar_popup(&pa.popups[i]);

            if (jogo->gameOver) desenhar_fim(recordes, &pa);
            else if (pa.pausado) desenhar_pausa();
            else {
                Dica dicas[3] = {{"P", NULL, "pausa"}, {"M", NULL, "menu"}, {"Q", NULL, "sair"}};
                desenhar_rodape(dicas, 3);
            }
        }
        EndMode2D();
        EndDrawing();
    }

    recordes_liberar(recordes);
    estado_liberar(jogo);
    liberar_recursos();
    CloseWindow();
    return 0;
}
