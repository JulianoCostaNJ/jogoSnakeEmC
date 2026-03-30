/*
 * input.c
 * -------
 * Implementacao do processamento de entrada do teclado.
 *
 * Principio: as funcoes de input apenas DISPARAM mudancas no estado do jogo,
 * nunca executam logica complexa. Por exemplo, input_game() altera nextDir
 * mas e update_game() em game.c que aplica o movimento.
 *
 * _kbhit() e _getch() sao funcoes do <conio.h> especificas do Windows.
 * Em Linux/Mac, seriam substituidas por configuracoes do terminal (termios.h).
 */

#include <conio.h>    /* _kbhit, _getch - entrada de teclado sem echo */
#include <windows.h>  /* Sleep */
#include <time.h>      /* time_t, time */

#include "../include/input.h"      /* Propria interface */
#include "../include/constants.h"  /* GameState, Direction, etc. */
#include "../include/game.h"       /* nextDir, dir, paused, autoMode, etc. */
#include "../include/render.h"     /* draw_pause_overlay, clear_pause_overlay, draw_bfs_* */
#include "../include/screens.h"    /* flags menuNeedsRedraw, gameoverNeedsRedraw */
#include "../include/bfs.h"        /* bfs_path_len, bfs_visited_count */

/* ================================================================
 * INPUT DO MENU PRINCIPAL
 * ================================================================ */

/*
 * menuIndex: indice da opcao atualmente selecionada (0=Jogar Manual, etc.)
 * Declarado em screens.c e usado aqui para navegacao.
 */
extern int menuIndex;

/*
 * input_menu: navegacao e selecao no menu principal.
 *
 * Teclas especiais do Windows chegam como 2 bytes:
 *   Primeiro byte: 0 (teclas de funcao antigas) ou 224 (teclas extendidas como setas)
 *   Segundo byte: codigo da tecla especifica
 *      72 = seta cima, 80 = seta baixo
 */
void input_menu(GameState *state) {
    if (!_kbhit()) return;   /* Sem tecla pressionada: retorna imediatamente */

    int k = _getch();   /* Le a tecla (sem echo, sem bloquear) */

    /* Detecta tecla especial (setas, F-keys): chegam em 2 bytes */
    if (k == 0 || k == 224) {
        k = _getch();   /* Le o segundo byte com o codigo da tecla */
        if      (k == 72) { menuIndex--; if (menuIndex < 0) menuIndex = 3; }  /* Seta cima */
        else if (k == 80) { menuIndex++; if (menuIndex > 3) menuIndex = 0; }  /* Seta baixo */
        return;   /* Tecla especial processada */
    }

    /* Teclas normais */
    if      (k == 'w' || k == 'W') { menuIndex--; if (menuIndex < 0) menuIndex = 3; }
    else if (k == 's' || k == 'S') { menuIndex++; if (menuIndex > 3) menuIndex = 0; }
    else if (k == '1') { autoMode = 0; init_game(); menuNeedsRedraw = 1; *state = STATE_PLAYING; }
    else if (k == '2') { autoMode = 1; init_game(); menuNeedsRedraw = 1; *state = STATE_PLAYING; }
    else if (k == '3') { rankingNeedsRedraw = 1; *state = STATE_RANKING; }
    else if (k == '4' || k == 27) { *state = STATE_EXIT; }   /* ESC ou '4' = sair */
    else if (k == 13) {   /* Enter: confirma opcao selecionada */
        if      (menuIndex == 0) { autoMode = 0; init_game(); menuNeedsRedraw = 1; *state = STATE_PLAYING; }
        else if (menuIndex == 1) { autoMode = 1; init_game(); menuNeedsRedraw = 1; *state = STATE_PLAYING; }
        else if (menuIndex == 2) { rankingNeedsRedraw = 1; *state = STATE_RANKING; }
        else                     { *state = STATE_EXIT; }
    }
}

/* ================================================================
 * INPUT DO RANKING
 * ================================================================ */

/*
 * input_ranking: qualquer tecla de "voltar" retorna ao menu.
 */
void input_ranking(GameState *state) {
    if (!_kbhit()) return;

    int k = _getch();
    /* Enter, ESC ou 'M' voltam ao menu */
    if (k == 13 || k == 27 || k == 'm' || k == 'M') {
        menuNeedsRedraw = 1;   /* Forca redesenho completo do menu */
        *state = STATE_MENU;
    }
}

/* ================================================================
 * INPUT DO JOGO
 * ================================================================ */

/*
 * input_game: processa teclas durante a partida em andamento.
 *
 * Separacao importante:
 *   - Direcao: so altera nextDir, nao dir (evita mudar 2x no mesmo frame)
 *   - Anti-reverso: nao permite inverter a direcao imediatamente
 *     (nao pode ir para baixo se esta indo para cima, etc.)
 *   - Modo auto: teclas de direcao sao ignoradas (o BFS controla)
 */
void input_game(GameState *state) {
    if (!_kbhit()) return;

    int k = _getch();

    /* Teclas especiais (setas do teclado numerico) */
    if (k == 0 || k == 224) {
        k = _getch();
        if (!autoMode) {   /* Setas so funcionam no modo manual */
            if      (k == 72 && dir != DIR_DOWN)  nextDir = DIR_UP;     /* Seta cima */
            else if (k == 77 && dir != DIR_LEFT)  nextDir = DIR_RIGHT;  /* Seta direita */
            else if (k == 80 && dir != DIR_UP)    nextDir = DIR_DOWN;   /* Seta baixo */
            else if (k == 75 && dir != DIR_RIGHT) nextDir = DIR_LEFT;   /* Seta esquerda */
        }
        return;
    }

    /* Teclas WASD de movimento (apenas modo manual) */
    if (!autoMode) {
        if      ((k == 'w' || k == 'W') && dir != DIR_DOWN)  nextDir = DIR_UP;
        else if ((k == 'd' || k == 'D') && dir != DIR_LEFT)  nextDir = DIR_RIGHT;
        else if ((k == 's' || k == 'S') && dir != DIR_UP)    nextDir = DIR_DOWN;
        else if ((k == 'a' || k == 'A') && dir != DIR_RIGHT) nextDir = DIR_LEFT;
    }

    /* Pausa/despausa (funciona em ambos os modos) */
    if (k == 'p' || k == 'P') {
        if (!paused) {
            /* Pausando: registra o momento e exibe overlay */
            paused    = 1;
            pauseStart = time(NULL);    /* Timestamp de inicio da pausa */
            update_side_panel();        /* Exibe "PAUSADO" no painel */
            draw_pause_overlay();       /* Caixa de pausa no centro */
        } else {
            /* Despausando: acumula o tempo pausado e limpa overlay */
            paused = 0;
            pausedSeconds += (int)(time(NULL) - pauseStart);  /* Adiciona segundos pausados */
            clear_pause_overlay();      /* Remove a caixa, restaura arena */
            update_side_panel();        /* Remove "PAUSADO" do painel */
        }
    }

    /* Abandona a partida e volta ao menu */
    else if (k == 'q' || k == 'Q') {
        menuNeedsRedraw = 1;
        *state = STATE_MENU;
    }

    /* Toggle: exibir/ocultar caminho BFS */
    else if (k == 'v' || k == 'V') {
        showBfsPath = !showBfsPath;   /* Inverte o flag */
        if (!showBfsPath) {
            /* Ocultando: limpa os pontos azuis da arena */
            for (int i = 0; i < bfs_path_len; i++) {
                int px = bfs_path[i].x;
                int py = bfs_path[i].y;
                /* So apaga se nao ha outro elemento por cima */
                if (!is_on_snake(px, py) && fruit_at(px, py) < 0 && !is_obstacle(px, py))
                    erase_cell(px, py);   /* Restaura fundo */
            }
        } else {
            /* Exibindo: redesenha o overlay */
            draw_bfs_path_overlay();
        }
    }

    /* Toggle: exibir/ocultar celulas visitadas pelo BFS */
    else if (k == 'g' || k == 'G') {
        showBfsVisit = !showBfsVisit;
        if (!showBfsVisit) {
            /* Ocultando: limpa todos os pontos verdes e redesenha a arena */
            clear_bfs_overlay();
            draw_all_obstacles();
            for (int i = 0; i < FRUIT_COUNT; i++) draw_fruit(i);
            /* Redesenha cobra (do fim para o inicio para ordem correta) */
            for (int i = snakeLen - 1; i >= 0; i--) {
                if (i == 0) draw_head(snake[i].x, snake[i].y);
                else        draw_body(snake[i].x, snake[i].y);
            }
            /* Se caminho ainda ativo: redesenha so o caminho */
            if (showBfsPath) draw_bfs_path_overlay();
        } else {
            /* Exibindo: mostra a area explorada */
            draw_bfs_visited_overlay();
        }
    }

    /* Toggle: alterna entre modo manual e automatico (BFS) */
    else if (k == 't' || k == 'T') {
        autoMode = !autoMode;     /* Inverte o modo */
        update_side_panel();      /* Atualiza indicador de modo no painel */
    }
}

/* ================================================================
 * INPUT DA TELA DE GAME OVER
 * ================================================================ */

extern int gameOverIndex;   /* Indice da opcao selecionada no game over (0 ou 1) */

/*
 * input_gameover: navega e confirma na tela de game over.
 */
void input_gameover(GameState *state) {
    if (!_kbhit()) return;

    int k = _getch();

    /* Teclas especiais (setas) */
    if (k == 0 || k == 224) {
        k = _getch();
        if      (k == 72) gameOverIndex = 0;   /* Seta cima: "Jogar Novamente" */
        else if (k == 80) gameOverIndex = 1;   /* Seta baixo: "Voltar ao Menu" */
        return;
    }

    if      (k == 'w' || k == 'W') gameOverIndex = 0;
    else if (k == 's' || k == 'S') gameOverIndex = 1;

    /* Atalhos diretos */
    else if (k == '1') {
        init_game();
        gameoverNeedsRedraw = 1;
        menuNeedsRedraw     = 1;
        *state = STATE_PLAYING;
    }
    else if (k == '2') {
        menuNeedsRedraw     = 1;
        gameoverNeedsRedraw = 1;
        *state = STATE_MENU;
    }

    /* Enter: confirma a opcao selecionada */
    else if (k == 13) {
        if (gameOverIndex == 0) {
            /* "Jogar Novamente": reinicia a partida */
            init_game();
            gameoverNeedsRedraw = 1;
            menuNeedsRedraw     = 1;
            *state = STATE_PLAYING;
        } else {
            /* "Voltar ao Menu": volta ao menu principal */
            menuNeedsRedraw     = 1;
            gameoverNeedsRedraw = 1;
            *state = STATE_MENU;
        }
    }
}
