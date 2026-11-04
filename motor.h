#ifndef MOTOR_H
#define MOTOR_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* TODO: definir o tamanho da queda (valores ainda nao decididos) */
#define alturaTab 10
#define larguraTab 5

#define TIPOS_CARTA 8   /* cartas de 0 a 7 */
#define TAM_NOME    32

/* Tabela das cartas (ja existe, nao mexer):
 * 0 caixa vazia, 1 uma fatia (esq), 2 metade (esq), 3 falta 1 (dir),
 * 4 inteira, 5 falta 1 (esq), 6 metade (dir), 7 uma fatia (dir) */
static const int FATIAS[TIPOS_CARTA] = {0, 1, 3, 5, 6, 5, 3, 1};
static const int LADO[TIPOS_CARTA]   = {0, -1, -1, 1, 0, -1, 1, 1};

/* ---------- ESTRUTURAS ---------- */

/* A peca que esta caindo. 'tipo' substitui o antigo 'pedacos'. */
typedef struct {
    int x;      /* coluna */
    int y;      /* linha (cresce para baixo, como no main.c) */
    int tipo;   /* 0 a 7 */
} Peca;

/* No da lista encadeada com as proximas pecas (fila). */
typedef struct NoFatia {
    int tipo;
    struct NoFatia *prox;
} NoFatia;

/* O estado inteiro do jogo. 'tab' e' a matriz alocada com malloc. */
typedef struct {
    int **tab;          /* alturaTab x larguraTab, alocada dinamicamente */
    Peca fatiaAtual;    /* peca caindo agora */
    NoFatia *filaFatias;/* lista com as proximas pecas */
    int pontuacao;
    bool gameOver;
} EstadoJogo;

/* No da lista encadeada do placar (top scores). */
typedef struct Recorde {
    char nome[TAM_NOME];
    int pontos;
    struct Recorde *prox;
} Recorde;

/* ---------- ESTADO (alocacao dinamica) ---------- */

/* TODO: aloca o EstadoJogo e a matriz tab (alturaTab x larguraTab) com malloc.
 * Devolve NULL se faltar memoria. */
EstadoJogo *estado_criar(void);

/* TODO: libera a matriz tab, a fila filaFatias e depois a struct (free). */
void estado_liberar(EstadoJogo *jogo);

/* TODO: zera tab, pontuacao e gameOver; coloca a primeira peca no topo do meio;
 * enche a fila. (chamada pelo main.c, como ja esta hoje) */
void iniciarJogo(EstadoJogo *jogo);

/* ---------- PECA E FILA (lista encadeada) ---------- */

/* TODO: cria um no com 'tipo' e coloca no fim da fila (malloc). */
void fila_adicionar(NoFatia **inicio, int tipo);

/* TODO: tira o primeiro no da fila, devolve o tipo e libera o no (free).
 * Se a fila estiver vazia, devolve -1 (ou sorteia: decidir). */
int fila_remover(NoFatia **inicio);

/* TODO: libera todos os nos da fila (free em cada um). */
void fila_liberar(NoFatia *inicio);

/* TODO: a peca atual passa a ser a proxima da fila (x no meio do topo, y = 0). */
void peca_proxima(EstadoJogo *jogo);

/* ---------- MOVIMENTO ---------- */

/* TODO: a peca pode ir para (x + dx, y + dy)? Confere borda e casa ocupada. */
bool pode_mover(const EstadoJogo *jogo, int dx, int dy);

/* TODO: se pode_mover, move a peca atual. Usado por 'a', 'd' e 's' no main.c. */
void mover_peca(EstadoJogo *jogo, int dx, int dy);

/* TODO: a peca desce uma linha. Se nao pode mais descer, chama fixar_peca. */
void descer_peca(EstadoJogo *jogo);

/* ---------- FIXAR E PONTUAR ---------- */

/* TODO: grava a peca atual em tab, calcula os pontos (pontuar_jogada),
 * soma em pontuacao, remove o que pontuou (remover_pontuados),
 * e chama peca_proxima. Se a nova peca nao cabe, gameOver = true. */
void fixar_peca(EstadoJogo *jogo);

/* TODO: quantos pontos vale a peca 'tipo' na casa (lin, col)? O tab nao muda.
 *   +1 para cada vizinha igual
 *   +3 quando as fatias da pizza inteira (6) se completam com as vizinhas
 * Regra exata de "completar 6" ainda em definicao: ver a pergunta ao grupo. */
int pontuar_jogada(const EstadoJogo *jogo, int lin, int col, int tipo);

/* TODO: apaga as cartas que pontuaram e faz as cartas de cima descerem
 * (gravidade). Decidir se roda em loop ate nao ter mais nada para apagar. */
void remover_pontuados(EstadoJogo *jogo);

/* ---------- PLACAR (lista encadeada + arquivo) ---------- */

/* TODO: insere um recorde na lista, mantendo a lista ordenada por pontos (maior primeiro). */
void recorde_inserir(Recorde **inicio, const char *nome, int pontos);

/* TODO: le o arquivo de recordes (fopen/fscanf) e devolve a lista. NULL se o arquivo nao existir. */
Recorde *recordes_carregar(const char *arquivo);

/* TODO: grava a lista no arquivo (fopen/fprintf). Devolve 1 se deu certo, 0 se nao. */
int recordes_salvar(const Recorde *inicio, const char *arquivo);

/* TODO: libera todos os nos da lista de recordes. */
void recordes_liberar(Recorde *inicio);

#endif
