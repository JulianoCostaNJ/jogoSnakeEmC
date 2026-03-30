/*
 * input.h
 * -------
 * Interface do modulo de processamento de entrada do teclado.
 *
 * Cada tela do jogo tem sua propria funcao de input para separar
 * as logicas de controle. As funcoes sao nao-bloqueantes: usam
 * _kbhit() para verificar se ha tecla disponivel antes de _getch().
 *
 * Fluxo do input:
 *   1. _kbhit() retorna 0 -> funcao retorna imediatamente (sem bloquear)
 *   2. _kbhit() retorna 1 -> _getch() le a tecla sem ecoar no console
 *   3. Teclas especiais (setas, F-keys) geram 2 bytes: 0 ou 224, depois o codigo
 */

#ifndef INPUT_H
#define INPUT_H

#include "../include/constants.h"  /* GameState enum */

/*
 * input_menu: processa teclas na tela do menu principal.
 *
 * Teclas reconhecidas:
 *   W/S ou setas cima/baixo -> navega entre opcoes
 *   Enter                   -> seleciona a opcao atual
 *   1,2,3,4                 -> atalhos diretos para cada opcao
 *   ESC                     -> sair
 */
void input_menu(GameState *state);

/*
 * input_ranking: processa teclas na tela de ranking.
 *
 * Teclas reconhecidas:
 *   Enter, ESC, M/m -> volta ao menu
 */
void input_ranking(GameState *state);

/*
 * input_game: processa teclas durante a partida.
 *
 * Teclas reconhecidas:
 *   W/A/S/D ou setas -> muda direcao (somente modo manual)
 *   P                -> pausa/despausa
 *   T                -> alterna entre modo manual e automatico
 *   V                -> toggle do overlay de caminho BFS
 *   G                -> toggle do overlay de celulas visitadas BFS
 *   Q                -> abandona a partida, volta ao menu
 */
void input_game(GameState *state);

/*
 * input_gameover: processa teclas na tela de game over.
 *
 * Teclas reconhecidas:
 *   W/S ou setas -> navega entre "Jogar Novamente" e "Voltar ao Menu"
 *   Enter        -> confirma a opcao selecionada
 *   1,2          -> atalhos diretos
 */
void input_gameover(GameState *state);

#endif /* INPUT_H */
