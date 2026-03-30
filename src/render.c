/*
 * render.c
 * --------
 * Implementacao de toda a renderizacao visual do jogo.
 *
 * Principio fundamental: este modulo APENAS LE o estado (game.h, bfs.h)
 * e escreve no console. Nunca chama update_game() ou altera variaveis
 * de logica. Isso separa o "o que acontece" do "como parece".
 *
 * Tecnica de desenho incremental:
 *   - Elementos estaticos (borda, fundo) sao desenhados uma vez
 *   - Elementos dinamicos (cobra, frutas, BFS) redesenham apenas
 *     as celulas que mudaram
 *   - O painel lateral compara com variaveis de cache (last*)
 */

#include <stdio.h>    /* printf, fflush */
#include <string.h>   /* strlen, snprintf */
#include <windows.h>  /* Sleep, GetTickCount */
#include <time.h>     /* time */

#include "../include/render.h"     /* Propria interface */
#include "../include/console.h"    /* gotoxy, set_color, write_at, fill_spaces */
#include "../include/constants.h"  /* Todas as constantes */
#include "../include/game.h"       /* Estado do jogo: snake, fruits, etc. */
#include "../include/bfs.h"        /* bfs_path, bfs_visited_cells, is_on_bfs_path */
#include "../include/ranking.h"    /* best_score */

/* ================================================================
 * UTILIDADES DE COR E LAYOUT
 * ================================================================ */

/*
 * border_color_for_level: retorna a cor de borda para o nivel atual.
 * O nivel cicla a cada 5 para dar feedback visual de progressao.
 * ((level-1) % 5) mapeia niveis 1,2,3,4,5,6,7... para indices 0,1,2,3,4,0,1,...
 */
int border_color_for_level(void) {
    switch ((level - 1) % 5) {          /* Ciclo de 5 cores */
        case 0: return C_BORDER_L1;     /* Nivel 1,6,11...: ciano */
        case 1: return C_BORDER_L2;     /* Nivel 2,7,12...: verde */
        case 2: return C_BORDER_L3;     /* Nivel 3,8,13...: amarelo */
        case 3: return C_BORDER_L4;     /* Nivel 4,9,14...: vermelho */
        case 4: return C_BORDER_L5;     /* Nivel 5,10,15..: magenta */
    }
    return C_BORDER_L1;   /* Fallback (nao deve ocorrer) */
}

/*
 * speed_icon / speed_label: retorna emoji e texto conforme a velocidade atual.
 * speedMs maior = jogo mais lento (delay maior entre frames).
 * Os thresholds foram calibrados para INITIAL_SPEED=140 e MIN_SPEED=50.
 */
const char *speed_icon(void) {
    if (speedMs > 110) return SYM_SPEED_SLOW;   /* > 110ms: tartaruga */
    if (speedMs >  85) return SYM_SPEED_MED;    /* 86-110ms: caminhando */
    if (speedMs >  65) return SYM_SPEED_FAST;   /* 66-85ms: foguete */
    return SYM_SPEED_INS;                         /* <= 65ms: raio */
}

const char *speed_label(void) {
    if (speedMs > 110) return "SLOW  ";
    if (speedMs >  85) return "NORMAL";
    if (speedMs >  65) return "FAST  ";
    return "INSANE";
}

/* ================================================================
 * CAIXAS UNICODE
 * ================================================================ */

/*
 * draw_box_unicode: desenha retangulo com bordas simples.
 * Parametros: cantos (left,top) e (right,bottom) em coordenadas de console.
 */
void draw_box_unicode(int left, int top, int right, int bottom, int color) {
    set_color(color);

    /* Linha superior: canto + horizontais + canto */
    gotoxy(left, top);
    printf("%s", BOX_TL);
    for (int x = left + 1; x < right; x++) printf("%s", BOX_H);
    printf("%s", BOX_TR);

    /* Linhas laterais: vertical esquerda e direita */
    for (int y = top + 1; y < bottom; y++) {
        gotoxy(left,  y); printf("%s", BOX_V);
        gotoxy(right, y); printf("%s", BOX_V);
    }

    /* Linha inferior: canto + horizontais + canto */
    gotoxy(left, bottom);
    printf("%s", BOX_BL);
    for (int x = left + 1; x < right; x++) printf("%s", BOX_H);
    printf("%s", BOX_BR);

    set_color(C_RESET);
    fflush(stdout);
}

/*
 * draw_box_double: desenha retangulo com bordas duplas (mais formal/elegante).
 * Usa os caracteres Unicode de dupla linha (╔═══╗ ║ ║ ╚═══╝).
 */
void draw_box_double(int left, int top, int right, int bottom, int color) {
    set_color(color);

    /* Linha superior com bordas duplas */
    gotoxy(left, top);
    printf("%s", BOX_TLH);
    for (int x = left + 1; x < right; x++) printf("%s", BOX_DH);
    printf("%s", BOX_TRH);

    /* Laterais com vertical dupla */
    for (int y = top + 1; y < bottom; y++) {
        gotoxy(left,  y); printf("%s", BOX_DV);
        gotoxy(right, y); printf("%s", BOX_DV);
    }

    /* Linha inferior */
    gotoxy(left, bottom);
    printf("%s", BOX_BLH);
    for (int x = left + 1; x < right; x++) printf("%s", BOX_DH);
    printf("%s", BOX_BRH);

    set_color(C_RESET);
    fflush(stdout);
}

/*
 * draw_separator_double: linha separadora horizontal (╠════╣).
 * Usada dentro de caixas duplas para dividir secoes do painel.
 */
void draw_separator_double(int left, int y, int right, int color) {
    set_color(color);
    gotoxy(left, y);
    printf("%s", BOX_LM);                                /* ╠ juncao esquerda */
    for (int x = left + 1; x < right; x++) printf("%s", BOX_DH);  /* ═ linhas */
    printf("%s", BOX_RM);                                /* ╣ juncao direita */
    set_color(C_RESET);
    fflush(stdout);
}

/*
 * clear_inside_box: preenche o interior de uma caixa com espacos.
 * Usado para "limpar" o conteudo antes de redesenhar, sem apagar a borda.
 */
void clear_inside_box(int left, int top, int right, int bottom) {
    for (int y = top + 1; y < bottom; y++) {
        gotoxy(left + 1, y);                              /* Uma posicao apos a borda */
        for (int x = left + 1; x < right; x++) putchar(' ');  /* Espacos ate a borda */
    }
    fflush(stdout);
}

/*
 * print_centered_range: centraliza texto entre 'left' e 'right'.
 * Calcula o x de inicio para que o texto fique no centro da largura disponivel.
 * Funciona mesmo com strings UTF-8 (usa strlen de bytes, nao de caracteres visuais).
 */
void print_centered_range(int left, int right, int y, int color, const char *text) {
    int width   = right - left - 1;        /* Largura util (sem as bordas) */
    int textlen = (int)strlen(text);        /* Comprimento em bytes (aproximacao) */
    int x = left + 1 + (width - textlen) / 2;  /* Posicao inicial centralizada */
    if (x < left + 1) x = left + 1;       /* Garante que nao ultrapassa borda */
    write_at(x, y, color, text);
}

/* ================================================================
 * BARRA DE PROGRESSO
 * ================================================================ */

/*
 * draw_progress_bar: barra visual de progresso entre 0 e maxval.
 * Usa blocos cheios (█) para a parte preenchida e vazios (░) para o resto.
 * 'bar_w' e a largura total da barra em caracteres.
 */
void draw_progress_bar(int x, int y, int bar_w, int value, int maxval) {
    if (maxval <= 0) maxval = 1;   /* Evita divisao por zero */

    /* Calcula quantos blocos cheios mostrar (proporcional) */
    int filled = (value * bar_w) / maxval;
    if (filled > bar_w) filled = bar_w;   /* Nao ultrapassar largura total */

    gotoxy(x, y);

    /* Parte preenchida: blocos solidos em verde */
    set_color(C_BAR_FILL);
    for (int i = 0; i < filled; i++) printf("%s", BAR_FULL);

    /* Parte vazia: blocos ocos em cinza */
    set_color(C_BAR_EMPTY);
    for (int i = filled; i < bar_w; i++) printf("%s", BAR_HALF);

    set_color(C_RESET);
    fflush(stdout);
}

/* ================================================================
 * ARENA: BORDA E FUNDO
 * ================================================================ */

/*
 * draw_board_border: desenha a borda externa da arena.
 *
 * A arena usa BLOCK (▓) nas laterais para dar espessura visual.
 * A cor muda conforme o nivel (border_color_for_level).
 */
void draw_board_border(void) {
    int bc = border_color_for_level();   /* Cor do nivel atual */
    set_color(bc);

    /* Linha superior: canto + 2*WIDTH horizontais + canto */
    gotoxy(ARENA_LEFT, TOP);
    printf("%s", BOX_TL);
    for (int x = 0; x < WIDTH * 2; x++) printf("%s", BOX_H);
    printf("%s", BOX_TR);

    /* Laterais: blocos cheios para parecer mais espesso */
    for (int y = TOP + 1; y < HEIGHT; y++) {
        gotoxy(ARENA_LEFT,  y); printf("%s", BLOCK);   /* Parede esquerda */
        gotoxy(ARENA_RIGHT, y); printf("%s", BLOCK);   /* Parede direita */
    }

    /* Linha inferior */
    gotoxy(ARENA_LEFT, HEIGHT);
    printf("%s", BOX_BL);
    for (int x = 0; x < WIDTH * 2; x++) printf("%s", BOX_H);
    printf("%s", BOX_BR);

    set_color(C_RESET);
    fflush(stdout);
}

/*
 * draw_board_background: preenche o interior da arena com fundo pontilhado.
 *
 * Padrao de xadrez: celulas onde (x+y) % 4 == 0 recebem o ponto (·),
 * as demais ficam vazias. Isso cria uma textura sutil sem distrair.
 */
void draw_board_background(void) {
    for (int y = TOP + 1; y < HEIGHT; y++) {
        gotoxy(ARENA_LEFT + 1, y);       /* Primeira celula apos a borda esquerda */
        set_color(C_BG);                 /* Cinza escuro para o fundo */
        for (int x = 0; x < WIDTH; x++) {
            if ((x + y) % 4 == 0)
                printf("%s ", DOT_BG);   /* Ponto medio + espaco (2 chars por celula) */
            else
                printf("  ");            /* Dois espacos vazios */
        }
    }
    set_color(C_RESET);
    fflush(stdout);
}

/* ================================================================
 * OBSTACULOS
 * ================================================================ */

/*
 * draw_obstacle: renderiza um obstaculo em posicao logica (x,y).
 * Usa arena_col() para converter para coordenada de console.
 */
void draw_obstacle(int x, int y) {
    int col = arena_col(x);   /* Converte x logico para coluna do console */
    gotoxy(col, y);
    set_color(C_OBSTACLE);    /* Vermelho escuro */
    printf("%s ", SYM_OBSTACLE);  /* Quadrado + espaco (para ocupar 2 colunas) */
    set_color(C_RESET);
    fflush(stdout);
}

/*
 * draw_all_obstacles: redesenha todos os obstaculos de uma vez.
 * Chamado apos animacoes ou transicoes que possam ter apagado a arena.
 */
void draw_all_obstacles(void) {
    for (int i = 0; i < obstacleCount; i++)
        draw_obstacle(obstacles[i].x, obstacles[i].y);
}

/*
 * erase_cell: apaga uma celula da arena e restaura o fundo pontilhado.
 * Usada para "apagar" a antiga cauda da cobra a cada frame.
 */
void erase_cell(int x, int y) {
    int col = arena_col(x);
    gotoxy(col, y);
    set_color(C_BG);
    /* Restaura o padrao de fundo original da celula */
    if ((x + y) % 4 == 0)
        printf("%s ", DOT_BG);   /* Ponto se celula era pontilhada */
    else
        printf("  ");             /* Vazio se era espaco em branco */
    set_color(C_RESET);
    fflush(stdout);
}

/* ================================================================
 * COBRA
 * ================================================================ */

/*
 * draw_head: renderiza a cabeca da cobra.
 * Emoji verde em modo manual, robo em modo automatico.
 * A diferenca visual ajuda o jogador a identificar o modo ativo.
 */
void draw_head(int x, int y) {
    gotoxy(arena_col(x), y);
    set_color(C_RESET);
    printf("%s", autoMode ? SYM_HEAD_AUTO : SYM_HEAD);   /* Escolhe emoji conforme modo */
    fflush(stdout);
}

/*
 * draw_body: renderiza um segmento do corpo.
 * Em alta velocidade (speedMs <= 65), usa emoji azul para indicar perigo.
 */
void draw_body(int x, int y) {
    gotoxy(arena_col(x), y);
    set_color(C_RESET);
    printf("%s", speedMs <= 65 ? SYM_BODY_FAST : SYM_BODY);   /* Verde normal ou azul rapido */
    fflush(stdout);
}

/*
 * draw_tail_seg: renderiza o ultimo segmento (cauda).
 * Amarelo para diferenciar visualmente da cabeca e do corpo.
 */
void draw_tail_seg(int x, int y) {
    gotoxy(arena_col(x), y);
    set_color(C_RESET);
    printf("%s", SYM_TAIL);   /* Emoji amarelo */
    fflush(stdout);
}

/* ================================================================
 * FRUTAS
 * ================================================================ */

/*
 * draw_fruit: renderiza a fruta de indice 'i' em sua posicao.
 * Usa os campos symbol e color da struct Fruit para o emoji correto.
 */
void draw_fruit(int i) {
    int col = arena_col(fruits[i].x);   /* Converte x logico para console */
    gotoxy(col, fruits[i].y);
    set_color(fruits[i].color);          /* Cor especifica da fruta */
    printf("%s", fruits[i].symbol);      /* Emoji da fruta */
    set_color(C_RESET);
    fflush(stdout);
}

/*
 * spawn_fruit_anim: animacao visual quando uma fruta aparece.
 * Pisca "*" 2 vezes antes de exibir o emoji definitivo.
 * Duração total: ~140ms (2 x 40+30ms) - rapido mas notavel.
 */
void spawn_fruit_anim(int i) {
    int col = arena_col(fruits[i].x);
    int y   = fruits[i].y;

    for (int f = 0; f < 2; f++) {     /* 2 piscadas */
        gotoxy(col, y);
        set_color(C_FLASH);
        printf("* ");                  /* Asterisco como flash de spawn */
        fflush(stdout);
        Sleep(40);                     /* Visivel por 40ms */
        erase_cell(fruits[i].x, y);   /* Apaga o asterisco */
        Sleep(30);                     /* Pausa antes da proxima piscada */
    }

    draw_fruit(i);   /* Exibe a fruta definitiva */
}

/* ================================================================
 * OVERLAYS BFS
 * ================================================================ */

/*
 * draw_bfs_visited_overlay: exibe as celulas visitadas pelo BFS como pontos verdes.
 *
 * Mostra a "area de busca" explorada pelo algoritmo, visualizando
 * quantas celulas foram avaliadas antes de encontrar o caminho.
 * Pula celulas que ja tem cobra, fruta ou obstaculo (eles tem prioridade visual).
 */
void draw_bfs_visited_overlay(void) {
    if (!showBfsVisit) return;   /* Overlay desativado pelo usuario */

    for (int i = 0; i < bfs_visited_count; i++) {
        int x = bfs_visited_cells[i].x;
        int y = bfs_visited_cells[i].y;

        /* Nao sobrescreve elementos mais importantes */
        if (is_on_snake(x, y))    continue;   /* Cobra tem prioridade */
        if (fruit_at(x, y) >= 0)  continue;   /* Fruta tem prioridade */
        if (is_obstacle(x, y))    continue;   /* Obstaculo tem prioridade */
        if (is_on_bfs_path(x, y)) continue;   /* Caminho tem cor propria (azul) */

        /* Desenha ponto verde para indicar celula explorada */
        int col = arena_col(x);
        gotoxy(col, y);
        set_color(C_BFS_VISIT);    /* Verde escuro */
        printf(". ");              /* Ponto simples + espaco */
        set_color(C_RESET);
    }
    fflush(stdout);
}

/*
 * draw_bfs_path_overlay: exibe o caminho calculado como pontos azuis.
 *
 * Este e o caminho que a cobra seguira no modo automatico,
 * ou a sugestao visual no modo manual.
 */
void draw_bfs_path_overlay(void) {
    if (!showBfsPath) return;   /* Overlay desativado pelo usuario */

    for (int i = 0; i < bfs_path_len; i++) {
        int x = bfs_path[i].x;
        int y = bfs_path[i].y;

        /* Nao sobrescreve cobra, fruta ou obstaculo */
        if (is_on_snake(x, y))   continue;
        if (fruit_at(x, y) >= 0) continue;
        if (is_obstacle(x, y))   continue;

        /* Desenha ponto de meio (·) em azul claro */
        int col = arena_col(x);
        gotoxy(col, y);
        set_color(C_BFS_PATH);    /* Azul claro */
        printf("%s ", SYM_PATH);  /* Ponto medio Unicode */
        set_color(C_RESET);
    }
    fflush(stdout);
}

/*
 * clear_bfs_overlay: apaga os overlays do frame anterior.
 *
 * Chamado no inicio de update_game() antes de calcular o novo BFS.
 * Restaura o fundo original em cada celula que tinha um overlay.
 */
void clear_bfs_overlay(void) {
    for (int i = 0; i < bfs_visited_count; i++) {
        int x = bfs_visited_cells[i].x;
        int y = bfs_visited_cells[i].y;

        /* So apaga se nao ha elemento mais importante */
        if (is_on_snake(x, y))   continue;
        if (fruit_at(x, y) >= 0) continue;
        if (is_obstacle(x, y))   continue;

        /* Restaura fundo pontilhado */
        int col = arena_col(x);
        gotoxy(col, y);
        set_color(C_BG);
        if ((x + y) % 4 == 0)
            printf("%s ", DOT_BG);   /* Ponto se era celula pontilhada */
        else
            printf("  ");            /* Espaco se era celula vazia */
    }
    set_color(C_RESET);
    fflush(stdout);
}

/* ================================================================
 * PAINEL LATERAL (HUD)
 * ================================================================ */

/*
 * draw_side_panel_frame: desenha a estrutura estatica do painel.
 *
 * Chamado UMA VEZ em init_game().
 * Inclui bordas duplas, separadores e labels fixos.
 * Os VALORES (score, nivel, etc.) sao atualizados separadamente
 * por update_side_panel() para evitar redesenho desnecessario.
 */
void draw_side_panel_frame(void) {
    int L = PANEL_X;         /* Coluna esquerda do painel */
    int R = PANEL_X + PANEL_W;  /* Coluna direita do painel */

    /* Caixa principal com bordas duplas */
    draw_box_double(L, TOP, R, HEIGHT, C_PANEL);

    /* Titulo do painel com emoji de cerebro */
    char title[48];
    snprintf(title, sizeof(title), "%s SNAKE BFS", SYM_BRAIN);
    print_centered_range(L, R, TOP + 1, C_TITLE, title);
    draw_separator_double(L, TOP + 2, R, C_PANEL);

    /* Labels fixos das metricas (os valores sao atualizados separadamente) */
    write_at(L + 2, TOP + 3,  C_PANEL_HDR, "SCORE");
    write_at(L + 2, TOP + 5,  C_PANEL_HDR, "NIVEL");
    write_at(L + 2, TOP + 7,  C_PANEL_HDR, "SPEED");
    write_at(L + 2, TOP + 9,  C_PANEL_HDR, "TEMPO");
    write_at(L + 2, TOP + 11, C_PANEL_HDR, "RECORDE");

    draw_separator_double(L, TOP + 13, R, C_PANEL);

    /* Secao de modo de jogo */
    write_at(L + 2, TOP + 14, C_PANEL_HDR, "MODO");
    draw_separator_double(L, TOP + 16, R, C_PANEL);

    /* Secao de informacoes BFS */
    char bfsline[32];
    snprintf(bfsline, sizeof(bfsline), "%s BFS INFO", SYM_BRAIN);
    write_at(L + 2, TOP + 17, C_PANEL_HDR, bfsline);
    draw_separator_double(L, TOP + 21, R, C_PANEL);

    /* Secao de controles */
    char ctrlline[32];
    snprintf(ctrlline, sizeof(ctrlline), "%s CONTROLES", SYM_CTRL);
    write_at(L + 2, TOP + 22, C_PANEL_HDR, ctrlline);
}

/*
 * update_side_panel: atualiza os valores dinamicos do painel.
 *
 * Usa variaveis de cache (last*) para redesenhar APENAS o que mudou.
 * Isso reduz drasticamente as escritas no console e elimina flickering.
 *
 * Chamado ao final de cada update_game() e em eventos especiais.
 */
void update_side_panel(void) {
    int L = PANEL_X;
    char buf[64];

    /* --- Score: so redesenha se mudou --- */
    if (lastScore != score) {
        snprintf(buf, sizeof(buf), "%06d", score);        /* 6 digitos com zeros a esquerda */
        write_at(L + 2, TOP + 4, C_SCORE, buf);
        lastScore = score;   /* Atualiza cache */
    }

    /* --- Nivel e barra de progresso --- */
    if (lastLevel != level) {
        snprintf(buf, sizeof(buf), "%d  ", level);
        write_at(L + 2, TOP + 6, C_LEVEL, buf);
        lastLevel = level;
    }
    /* A barra e sempre redesenhada pois o progresso muda com o score */
    {
        int levelBase = (level - 1) * LEVEL_STEP;   /* Score base do nivel atual */
        int levelNext = level       * LEVEL_STEP;   /* Score necessario para o proximo */
        int progress  = score - levelBase;           /* Progresso dentro do nivel */
        int needed    = levelNext - levelBase;       /* Pontos necessarios para subir */
        if (needed > 0)
            draw_progress_bar(L + 2, TOP + 7, PANEL_W - 3, progress, needed);
    }

    /* --- Velocidade: sempre atualiza (muda com nivel) --- */
    snprintf(buf, sizeof(buf), "%s %s", speed_icon(), speed_label());
    write_at(L + 2, TOP + 8, C_SPEED, buf);

    /* --- Tempo: atualiza apenas quando o segundo muda --- */
    int elapsed = (int)(time(NULL) - startTime) - pausedSeconds;   /* Segundos jogados */
    if (paused) elapsed = (int)(pauseStart - startTime) - pausedSeconds;  /* Congela em pausa */
    if (elapsed < 0) elapsed = 0;   /* Previne valor negativo por sincronizacao */
    if (elapsed != lastElapsed) {
        snprintf(buf, sizeof(buf), "%02d:%02d", elapsed / 60, elapsed % 60);   /* MM:SS */
        write_at(L + 2, TOP + 10, C_TIME, buf);
        lastElapsed = elapsed;
    }

    /* --- Recorde: destaca com coroa se jogador esta batendo o recorde --- */
    int rec = best_score();
    snprintf(buf, sizeof(buf), "%06d", rec);
    write_at(L + 2, TOP + 12,
             (score > 0 && score >= rec) ? C_BEST : C_PANEL_VAL, buf);

    /* Coroa aparece se esta batendo o recorde */
    if (score > 0 && score >= rec)
        write_at(L + 10, TOP + 12, C_BEST, SYM_CROWN);
    else
        write_at(L + 10, TOP + 12, C_RESET, "  ");  /* Apaga a coroa se nao esta batendo */

    /* --- Modo de jogo --- */
    if (autoMode) {
        snprintf(buf, sizeof(buf), "%s AUTOMATICO  ", SYM_ROBOT);
        write_at(L + 2, TOP + 15, C_MODE_AUTO, buf);
    } else {
        snprintf(buf, sizeof(buf), "%s MANUAL      ", SYM_CTRL);
        write_at(L + 2, TOP + 15, C_MODE_MAN, buf);
    }

    /* --- Informacoes do BFS --- */
    snprintf(buf, sizeof(buf), "Caminho: %3d    ", bfs_path_len);       /* Passos no caminho */
    write_at(L + 2, TOP + 18, C_BFS_INFO, buf);

    snprintf(buf, sizeof(buf), "Visitados: %3d  ", bfs_visited_count);  /* Nos explorados */
    write_at(L + 2, TOP + 19, C_BFS_INFO, buf);

    snprintf(buf, sizeof(buf), "Frutas: %d  ", fruitsEaten);
    write_at(L + 2, TOP + 20, C_PANEL_VAL, buf);

    /* --- Controles adaptativos (mudam conforme o modo) --- */
    if (autoMode)
        write_at(L + 2, TOP + 23, C_NORMAL, "T  Modo Manual  ");
    else
        write_at(L + 2, TOP + 23, C_NORMAL, "WASD  Mover     ");

    write_at(L + 2, TOP + 24, C_NORMAL, "V  Caminho BFS  ");
    write_at(L + 2, TOP + 25, C_NORMAL, "G  Visitados BFS");
    write_at(L + 2, TOP + 26, C_NORMAL, "T  Trocar Modo  ");
    write_at(L + 2, TOP + 27, C_NORMAL, "P  Pausar       ");

    /* --- Combo (so exibe se combo >= 3) --- */
    if (lastCombo != comboCount) {
        if (comboCount >= 3) {
            snprintf(buf, sizeof(buf), "%s x%d COMBO!  ", SYM_FIRE, comboCount);
            write_at(L + 2, TOP + 28, C_COMBO, buf);
        } else {
            fill_spaces(L + 2, TOP + 28, PANEL_W - 3);   /* Apaga combo antigo */
        }
        lastCombo = comboCount;
    }

    /* --- Status de pausa --- */
    if (paused) {
        snprintf(buf, sizeof(buf), "%s PAUSADO ", SYM_PAUSE);
        write_at(L + 2, TOP + 29, C_PAUSED, buf);
    } else {
        fill_spaces(L + 2, TOP + 29, 12);   /* Apaga o "PAUSADO" quando despausar */
    }
}

/* ================================================================
 * OVERLAY DE PAUSA
 * ================================================================ */

/*
 * draw_pause_overlay: exibe caixa de pausa centralizada na arena.
 * Calcula o centro da arena e posiciona uma caixa dupla com mensagem.
 */
void draw_pause_overlay(void) {
    int arenaW = WIDTH * 2;          /* Largura da arena em colunas de console */
    int boxW   = 20;                 /* Largura da caixa de pausa */
    int L = ARENA_LEFT + 1 + (arenaW - boxW) / 2;  /* Coluna esquerda centralizada */
    int R = L + boxW;
    int T = (TOP + HEIGHT) / 2 - 2;  /* Linha superior (acima do centro) */
    int B = T + 4;                   /* Linha inferior (4 linhas de altura) */

    draw_box_double(L, T, R, B, C_PAUSED);  /* Borda vermelha */
    clear_inside_box(L, T, R, B);            /* Limpa interior */

    /* Titulo e instrucao */
    char pauseTitle[32];
    snprintf(pauseTitle, sizeof(pauseTitle), "%s PAUSADO", SYM_PAUSE);
    print_centered_range(L, R, T + 1, C_PAUSED, pauseTitle);
    print_centered_range(L, R, T + 2, C_NORMAL, "P para retomar");
}

/*
 * clear_pause_overlay: remove a caixa de pausa e restaura a arena.
 *
 * Este e o processo inverso mais complexo: precisa redesenhar tudo
 * que estava sob a caixa de pausa.
 * Passos:
 *   1. Apaga a area da caixa com espacos
 *   2. Redesenha a borda da arena (pode ter sido sobreposta)
 *   3. Restaura o fundo das celulas logicas afetadas
 *   4. Redesenha obstaculos, frutas e cobra na area afetada
 *   5. Redesenha overlays BFS
 */
void clear_pause_overlay(void) {
    int arenaW = WIDTH * 2;
    int boxW   = 20;
    int L = ARENA_LEFT + 1 + (arenaW - boxW) / 2;
    int R = L + boxW;
    int T = (TOP + HEIGHT) / 2 - 2;
    int B = T + 4;

    /* Passo 1: Limpa a area da caixa com espacos */
    for (int y = T; y <= B; y++) {
        gotoxy(L, y);
        for (int x = L; x <= R + 1; x++) putchar(' ');
    }
    fflush(stdout);

    /* Passo 2: Redesenha borda (caso tenha sido sobreposta) */
    draw_board_border();

    /* Passo 3: Restaura o fundo nas celulas logicas afetadas */
    for (int y = T; y <= B; y++) {
        for (int lx = 1; lx <= WIDTH; lx++) {
            int col = arena_col(lx);
            if (col >= L && col <= R + 1)   /* Celula esta dentro da area da caixa */
                erase_cell(lx, y);           /* Restaura fundo pontilhado */
        }
    }

    /* Passo 4: Redesenha elementos da arena na area */
    draw_all_obstacles();
    for (int i = 0; i < FRUIT_COUNT; i++) draw_fruit(i);

    /* Redesenha cobra (do fim para o inicio: cabeca fica por cima) */
    for (int i = snakeLen - 1; i >= 0; i--) {
        if (i == 0) draw_head(snake[i].x, snake[i].y);
        else        draw_body(snake[i].x, snake[i].y);
    }

    /* Passo 5: Redesenha overlays BFS */
    draw_bfs_visited_overlay();
    draw_bfs_path_overlay();
}
