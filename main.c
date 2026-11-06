#include <stdio.h>
#include <stdlib.h>
#include <time.h> 
#include "screen.h"
#include "keyboard.h"
#include "timer.h"
#include "motor.h"


// Função para gerar uma nova fatia aleatória e inserir no final da lista...
void enfileirarFatia(EstadoJogo *jogo) {
    NoFatia *novaFatia = (NoFatia *)malloc(sizeof(NoFatia));
    novaFatia->pedacos = (rand() % 5) + 1; // valor aleatório 1 a 5
    novaFatia->proximo = NULL;
    
    if (jogo->filaFatias == NULL) {
        jogo->filaFatias = novaFatia;
    } else {
        NoFatia *atual = jogo->filaFatias;
        while (atual->proximo != NULL) {
            atual = atual->proximo;
        }
        atual->proximo = novaFatia;
    }
}

// Função para remover a fatia do início da lista e retornar seu valor
int desenfileirarFatia(EstadoJogo *jogo) {
    if (jogo->filaFatias == NULL) return 1; // Segurança
    
    NoFatia *noRemovido = jogo->filaFatias;
    int valorFatia = noRemovido->pedacos;
    
    jogo->filaFatias = noRemovido->proximo;
    free(noRemovido); 
    
    enfileirarFatia(jogo); 
    
    return valorFatia;
}

// Função para limpar a memória da lista ao encerrar o jogo
void limparFila(EstadoJogo *jogo) {
    NoFatia *atual = jogo->filaFatias;
    while (atual != NULL) {
        NoFatia *proximo = atual->proximo;
        free(atual);
        atual = proximo;
    }
    jogo->filaFatias = NULL;
}


// INICIALIZAÇÃO E ENTRADA REORGANIZADOS: compilador lê de cima para baixo, por isso troquei as implementações para o topo.

void iniciarJogo (EstadoJogo *jogo) {
    // Inicializa a semente de números aleatórios usando o relógio do sistema
    srand(time(NULL)); 

    for (int i = 0; i < alturaTab; i++) {
        for (int j = 0; j < larguraTab; j++) {
            jogo->tab[i][j] = 0;
        }
    }
    
    jogo->pontuacao = 0;
    jogo->gameOver = false; 
    jogo->fatiaAtual.x = larguraTab / 2; 
    jogo->fatiaAtual.y = 0; 
    
    // Lista inicia vazia
    jogo->filaFatias = NULL;
    
    // Preenche a fila inicial com 3 fatias
    for (int i = 0; i < 3; i++) {
        enfileirarFatia(jogo);
    }

    /*  
    Tirei aquele "3" temporário pela função que tira a primeira peça da fila.
    A fila dinâmica e a alocação de memória agora controlam as peças.
    */
    jogo->fatiaAtual.pedacos = desenfileirarFatia(jogo); 
}

// Hora de mover as peças
void processaEntrada (EstadoJogo *jogo) {
    if (keyhit ()) {
        int tecla = readch();
        // switch-case:  simplificando caracteres 
        switch (tecla) {
            case 'a':
            case 'A':
                // para não ultrapassar borda esquerda! importante pois impede que o jogador mova a peça para fora do tabuleiro e faça o programa crashar
                if (jogo->fatiaAtual.x > 0) jogo->fatiaAtual.x--;
                break;
                
            case 'd':     
            case 'D':
                // A matriz tem largura 10, mas as colunas vão de 0 a 9, então
                if (jogo->fatiaAtual.x < larguraTab - 1) jogo->fatiaAtual.x++;
                break;
                
            case 's':     
            case 'S':
                jogo->fatiaAtual.y++; 
                break;

            case 'q':     
            case 'Q':
            case 27:      // ASCII para esc
                jogo->gameOver = true;
                break;
        }
    }
}