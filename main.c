#include <stdio.h>
#include <stdlib.h>
#include "screen.h"
#include "keyboard.h"
#include "timer.h"
#include "motor.h"

void iniciarJogo (EstadoJogo *jogo) {
    for (int i = 0; i < alturaTab; i++) {
        for (int j = 0; j < larguraTab; j++) {
            jogo->tab[i][j] = 0;
        }
    }
    jogo->pontuacao = 0;
    jogo->gameOver = false; 
    jogo->fatiaAtual.x = larguraTab / 2; 
    jogo->fatiaAtual.y = 0; 
    /*  
    Defini o "3" de forma temporária para termos algo visível na tela enquanto testamos os botões e a tela do terminal.
    Quando for implementar a lista encadeada (a fila de próximas peças) e a alocação, vou apagar essa linha e trocar por algo dinâmico. 
    */
    jogo->fatiaAtual.pedacos = 3; 
    // Lista vazia por enquanto
    jogo->filaFatias = NULL;
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


