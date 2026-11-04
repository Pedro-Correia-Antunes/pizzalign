#ifndef MOTOR_H
#define MOTOR_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define larguraTab 10     /* colunas */
#define alturaTab  20     /* linhas (a de cima e' a linha 0) */

#define VAZIA       0     /* casa sem peca (o main.c ja zera assim) */
#define PEDACOS_MAX 5     /* uma peca tem de 1 a 5 pedacos */
#define PIZZA_INTEIRA 6   /* soma que entrega a pizza */
#define PONTOS_PIZZA 3    /* pontos por pizza entregue */
#define TAM_NOME    32

/* ---------- ESTRUTURAS ---------- */

/* A peca que esta caindo. 'pedacos' vale de 1 a 5. */
typedef struct {
    int x;          /* coluna */
    int y;          /* linha (cresce para baixo, como no main.c) */
    int pedacos;    /* 1 a 5 */
} Peca;

/* No da lista encadeada com as proximas pecas (fila). */
typedef struct NoFatia {
    int pedacos;
    struct NoFatia *prox;
} NoFatia;

/* O estado inteiro do jogo. 'tab' e' a matriz alocada com malloc. */
typedef struct {
    int **tab;           /* alturaTab x larguraTab; VAZIA ou 1 a 5 */
    Peca fatiaAtual;     /* peca caindo agora */
    NoFatia *filaFatias; /* lista com as proximas pecas */
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

/* ---------- FILA (lista encadeada) ---------- */

/* TODO: cria um no com 'pedacos' e coloca no fim da fila (malloc). */
void fila_adicionar(NoFatia **inicio, int pedacos);

/* TODO: tira o primeiro no da fila, devolve o valor e libera o no (free).
 * Se a fila estiver vazia, devolve 0 (ou sorteia: decidir). */
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

/* ---------- FIXAR, FUNDIR E ENTREGAR ---------- */

/* TODO: fixa a peca atual na matriz. Se cair sobre outra peca, tenta fundir:
 *   soma <= 6 -> funde (chama fundir_peca)
 *   soma  > 6 -> trava e empilha (a peca fica na casa de cima)
 * Depois chama entregar_pizzas (se houver soma 6), aplicar_gravidade,
 * peca_proxima. Se a nova peca nao cabe no topo, gameOver = true. */
void fixar_peca(EstadoJogo *jogo);

/* TODO: funde 'pedacos' na casa (lin, col): a casa passa a ter a soma.
 * Devolve a soma. */
int fundir_peca(EstadoJogo *jogo, int lin, int col, int pedacos);

/* TODO: procura casas com soma 6 (pizza inteira). Para cada uma: apaga a casa,
 * soma PONTOS_PIZZA em pontuacao. Devolve quantas pizzas foram entregues. */
int entregar_pizzas(EstadoJogo *jogo);

/* TODO: gravidade continua (Tetris). Cada peca solta cai ate bater em outra
 * peca ou no fundo. Pode ser preciso repetir ate nao ter mais o que cair. */
void aplicar_gravidade(EstadoJogo *jogo);

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
