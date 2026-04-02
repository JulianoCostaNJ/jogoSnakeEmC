/*
 * main.c
 * ------
 * Ponto de entrada do programa e loop principal do jogo.
 *
 * Responsabilidades deste modulo (e apenas estas):
 *   1. Inicializar o subsistema de console
 *   2. Configurar o gerador de numeros aleatorios
 *   3. Executar a maquina de estados principal
 *   4. Finalizar o programa de forma limpa
 *
 * Maquina de estados do jogo:
 *
 *   [MENU] --(jogar)--> [PLAYING] --(morreu)--> [GAMEOVER]
 *      ^                    |                       |
 *      |                    v                       |
 *      +-----------(Q/sair)-+<-------(voltar)-------+
 *      |
 *      +--(ranking)--> [RANKING] --(voltar)---> [MENU]
 *      |
 *      +--(sair)-----> [EXIT]
 *
 * Compile: gcc src/main.c src/*.c -o snake_bfs.exe -std=c11
 */

#include <stdio.h>     /* printf */
#include <stdlib.h>    /* srand */
#include <time.h>      /* time */
#include <windows.h>   /* Sleep */

/* Modulos do jogo */
#include "../include/console.h"    /* console_init, hide_cursor, show_cursor, clear_screen */
#include "../include/constants.h"  /* GameState enum */
#include "../include/game.h"       /* init_game, update_game, gameOver, paused, etc. */
#include "../include/render.h"     /* update_side_panel, draw_pause_overlay */
#include "../include/input.h"      /* input_menu, input_game, input_ranking, input_gameover */
#include "../include/screens.h"    /* render_menu_screen, render_ranking_screen, etc. */
#include "../include/ranking.h"    /* insert_ranking */
#include "../include/effects.h"    /* anim_death */
int main(void) {

    /* ============================================================
     * INICIALIZACAO
     * ============================================================ */

    /* Inicializa console: UTF-8, tamanho do buffer, titulo da janela.
     * Deve ser a primeira operacao pois as demais dependem do hConsole. */
    if (console_init() != 0) {
        return 1;   /* Falha critica: sem console, nao pode continuar */
    }

    /* Inicializa o gerador de numeros aleatorios com o timestamp atual.
     * Sem isso, a sequencia de numeros seria identica a cada execucao,
     * frutas e obstaculos apareceriam sempre nos mesmos lugares. */
    srand((unsigned int)time(NULL));

    /* Oculta o cursor piscante para visual mais limpo durante o jogo.
     * O cursor sera exibido novamente antes de qualquer entrada de texto. */
    hide_cursor();

    /* ============================================================
     * LOOP PRINCIPAL: MAQUINA DE ESTADOS
     * ============================================================ */

    GameState state = STATE_MENU;   /* Estado inicial: menu principal */

    while (state != STATE_EXIT) {

        /* --------------------------------------------------------
         * ESTADO: MENU PRINCIPAL
         * -------------------------------------------------------- */
        if (state == STATE_MENU) {
            render_menu_screen();   /* Desenha o menu (incremental) */
            input_menu(&state);     /* Processa teclas do menu */
            Sleep(30);              /* ~33fps para o menu (baixo recurso) */
        }
        else if (state == STATE_DIFFICULTY) {
            render_difficulty_screen();/* desenha o submenu de dificuldade */
            input_difficulty(&state);/* Processa teclas do submenu de dificuldade*/ 
            Sleep(50);              
        }
        

        /* --------------------------------------------------------
         * ESTADO: RANKING
         * -------------------------------------------------------- */
        else if (state == STATE_RANKING) {
            render_ranking_screen();   /* Exibe tabela de recordes */
            input_ranking(&state);     /* Aguarda tecla para voltar */
            Sleep(50);                 /* Polling suave */
        }

        /* --------------------------------------------------------
         * ESTADO: PARTIDA EM ANDAMENTO
         * -------------------------------------------------------- */
        else if (state == STATE_PLAYING) {

            /* Processa input ANTES do update para reduzir latencia:
             * O jogador pressiona a tecla e ela e aplicada no mesmo frame. */
            input_game(&state);

            /* Verifica se o input mudou o estado (ex: Q para voltar ao menu) */
            if (state != STATE_PLAYING) continue;

            if (!paused) {
                /* --- Frame normal: atualiza o jogo --- */
                update_game();   /* Logica de movimento, colisao, pontuacao */

                if (gameOver) {
                    /* --- Fim de partida: processa game over --- */
                    anim_death();   /* Animacao de morte (bloqueante) */

                    /* Solicita o nome do jogador para o ranking */
                    char name[32];
                    show_cursor();   /* Exibe cursor para o usuario digitar */

                    /* Posiciona o prompt abaixo da arena */
                    gotoxy(0, HEIGHT + 2);
                    set_color(C_TITLE);
                    printf("  Digite seu nome: ");
                    fflush(stdout);
                    set_color(C_PANEL_VAL);

                    /* Le o nome (com echo, ate Enter) */
                    if (fgets(name, sizeof(name), stdin) != NULL)
                        trim_newline(name);   /* Remove '\n' do final */
                    else
                        strcpy(name, "Jogador");   /* Fallback se leitura falhar */

                    if (strlen(name) == 0)
                        strcpy(name, "Jogador");   /* Nome padrao se vazio */

                    /* Salva no ranking e persiste no arquivo */
                    insert_ranking(name, score);
                    hide_cursor();   /* Oculta cursor novamente */

                    /* Transita para a tela de game over */
                    gameOverIndex       = 0;    /* Seleciona "Jogar Novamente" por padrao */
                    gameoverNeedsRedraw = 1;    /* Forca redesenho completo */
                    state = STATE_GAMEOVER;

                } else {
                    /* --- Sem game over: aplica delay para controlar velocidade ---
                     * No modo automatico, usa velocidade ligeiramente mais rapida
                     * (maxima 80ms) para que o BFS seja mais impactante visualmente.
                     * No modo manual, usa speedMs normal. */
                    int sp = autoMode ? (speedMs > 80 ? 80 : speedMs) : speedMs;
                    Sleep(sp);   /* Delay em ms: controla o frame rate do jogo */
                }

            } else {
                /* --- Jogo pausado: polling suave sem atualizar o jogo --- */
                Sleep(50);   /* 20fps durante pausa (economiza CPU) */
            }
        }

        /* --------------------------------------------------------
         * ESTADO: GAME OVER
         * -------------------------------------------------------- */
        else if (state == STATE_GAMEOVER) {
            render_gameover_screen();   /* Exibe estatisticas e opcoes */
            input_gameover(&state);     /* Processa escolha do jogador */
            Sleep(50);
        }

        /* STATE_EXIT e tratado pela condicao do while */
    }

    /* ============================================================
     * FINALIZACAO
     * ============================================================ */

    show_cursor();    /* Restaura o cursor antes de sair */
    clear_screen();   /* Limpa a tela para o terminal voltar ao normal */
    set_color(C_RESET);   /* Restaura a cor padrao do terminal */

    return 0;   /* Encerramento limpo */
}
