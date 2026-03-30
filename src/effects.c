/*
 * effects.c
 * ---------
 * Implementacao dos efeitos visuais e animacoes do jogo.
 *
 * Todos os efeitos aqui sao BLOQUEANTES (usam Sleep), o que significa
 * que o jogo nao processa input durante eles. Isso e intencional:
 * cria um momento de "drama" que deixa o jogo mais expressivo.
 *
 * Para efeitos nao-bloqueantes, seria necessario um sistema de animacao
 * baseado em timer (ver sugestoes de melhoria no README).
 */

#include <stdio.h>    /* printf, snprintf, fflush */
#include <string.h>   /* strlen */
#include <windows.h>  /* Sleep */

#include "../include/effects.h"    /* Propria interface */
#include "../include/constants.h"  /* SYM_*, C_*, BOX_*, dimensoes */
#include "../include/console.h"    /* gotoxy, set_color, write_at, fill_spaces */
#include "../include/render.h"     /* draw_box_unicode, draw_board_border, draw_fruit, etc. */
#include "../include/game.h"       /* snake[], fruits[], obstacles[], snakeLen, level */

/*
 * flash_eat: efeito visual ao comer uma fruta.
 *
 * Alterna a borda da arena entre a cor do nivel e a cor de flash,
 * criando um efeito de "pulsar". A intensidade (flashes, cor) varia
 * conforme a raridade para dar peso visual ao evento.
 *
 * rarity: 0=comum(1 flash verde), 1=raro(2 flashes amarelo), 2=epico(3 flashes ciano)
 */
void flash_eat(int rarity) {
    int flashes    = rarity + 1;   /* Mais raro = mais piscadas */
    int flashColor = (rarity == 2) ? 11 :   /* Epico: ciano */
                     (rarity == 1) ? 14 :   /* Raro: amarelo */
                                     10;    /* Comum: verde */
    int bc = border_color_for_level();   /* Cor original da borda (para restaurar) */

    for (int f = 0; f < flashes; f++) {
        /* Piscada: borda na cor de flash... */
        draw_box_unicode(ARENA_LEFT, TOP, ARENA_RIGHT, HEIGHT, flashColor);
        Sleep(25);   /* Visivel por 25ms */

        /* ...depois volta para a cor do nivel... */
        draw_box_unicode(ARENA_LEFT, TOP, ARENA_RIGHT, HEIGHT, bc);
        Sleep(25);   /* Pausa de 25ms antes da proxima piscada */
    }

    /* Garante que a borda termina na cor correta do nivel */
    draw_board_border();
}

/*
 * anim_death: animacao de fim de jogo.
 *
 * Cria um efeito de "alarme" alternando a borda vermelha com uma
 * mensagem de GAME OVER centralizada. 4 ciclos de piscada total.
 *
 * Usa strlen(msg) para centralizar, mas como msg contem UTF-8 (emojis),
 * a centralizacao e aproximada (strlen retorna bytes, nao caracteres visuais).
 */
void anim_death(void) {
    int cx  = ARENA_LEFT + WIDTH;      /* Centro horizontal da arena (em colunas de console) */
    int cy  = (TOP + HEIGHT) / 2;      /* Centro vertical da arena */

    char msg[32];
    snprintf(msg, sizeof(msg), " %s GAME OVER %s ", SYM_SKULL, SYM_SKULL);

    for (int i = 0; i < 4; i++) {
        /* Frame par: borda vermelha + mensagem */
        draw_box_unicode(ARENA_LEFT, TOP, ARENA_RIGHT, HEIGHT, C_GAMEOVER);
        if (i % 2 == 0)
            write_at(cx - (int)strlen(msg)/2, cy, C_GAMEOVER, msg);
        else
            /* Frame impar: apaga a mensagem (efeito piscante) */
            fill_spaces(cx - (int)strlen(msg)/2, cy, (int)strlen(msg));

        Sleep(150);   /* Cada frame dura 150ms */
    }

    /* Restaura a borda ao estado original */
    draw_board_border();
}

/*
 * anim_level_up: animacao de subida de nivel.
 *
 * Exibe o numero do novo nivel piscando no centro da arena.
 * Apos a animacao, restaura tudo que estava na linha do centro.
 *
 * A restauracao e necessaria porque write_at sobrescreve qualquer
 * elemento que estava na linha cy (cobra, fruta, etc.).
 */
void anim_level_up(void) {
    char msg[32];
    int cx = ARENA_LEFT + WIDTH;      /* Centro horizontal */
    int cy = (TOP + HEIGHT) / 2;      /* Linha central da arena */

    snprintf(msg, sizeof(msg), "  LEVEL %d!  ", level);   /* Mensagem com nivel atual */

    for (int i = 0; i < 3; i++) {
        /* Mostra a mensagem em branco brilhante */
        write_at(cx - (int)strlen(msg)/2, cy, C_FLASH, msg);
        Sleep(120);   /* Visivel por 120ms */

        /* Apaga a mensagem */
        fill_spaces(cx - (int)strlen(msg)/2, cy, (int)strlen(msg));
        Sleep(80);    /* Pausa antes da proxima piscada */
    }

    /* Restaura a linha central: primeiro apaga tudo, depois redesenha elementos */
    for (int x = 1; x <= WIDTH; x++)
        erase_cell(x, cy);   /* Limpa toda a linha central */

    /* Redesenha frutas que estavam na linha central */
    for (int i = 0; i < FRUIT_COUNT; i++)
        if (fruits[i].y == cy) draw_fruit(i);

    /* Redesenha obstaculos que estavam na linha central */
    for (int i = 0; i < obstacleCount; i++)
        if (obstacles[i].y == cy)
            draw_obstacle(obstacles[i].x, obstacles[i].y);

    /* Redesenha segmentos da cobra na linha central (do fim para o inicio) */
    for (int i = snakeLen - 1; i >= 0; i--) {
        if (snake[i].y != cy) continue;   /* So nos da linha central */
        if (i == 0) draw_head(snake[i].x, snake[i].y);  /* Cabeca por cima */
        else        draw_body(snake[i].x, snake[i].y);  /* Corpo */
    }

    /* Redesenha a borda com a nova cor do nivel */
    draw_board_border();
}
