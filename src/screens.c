/*
 * screens.c
 * ---------
 * Implementacao das telas de UI: menu, ranking e game over.
 *
 * Cada tela usa o padrao de "double buffering logico":
 *   - Flag *NeedsRedraw controla se o frame estatico deve ser redesenhado
 *   - Variaveis de cache (last*) controlam se opcoes dinamicas mudaram
 *   - Apenas o necessario e redesenhado a cada chamada
 */

#include <stdio.h>    /* printf, fflush, snprintf */
#include <string.h>   /* strlen */
#include <windows.h>  /* GetTickCount, Sleep */
#include <time.h>     /* time */

#include "../include/screens.h"    /* Propria interface */
#include "../include/constants.h"  /* Constantes, tipos */
#include "../include/console.h"    /* gotoxy, set_color, write_at, fill_spaces */
#include "../include/ranking.h"    /* load_ranking, best_score */
#include "../include/game.h"       /* score, level, fruitsEaten, etc. */

/* ================================================================
 * VARIAVEIS DE ESTADO DAS TELAS
 * ================================================================ */

/* Flags de redesenho: 1 = precisa redesenhar a estrutura estatica */
int menuNeedsRedraw     = 1;   /* Menu precisa de redesenho completo inicialmente */
int gameoverNeedsRedraw = 1;
int rankingNeedsRedraw  = 1;


/* Indices de selecao */
int menuIndex     = 0;   /* Opcao do menu (0=Jogar Manual, 1=Auto, 2=Ranking, 3=Sair) */
int gameOverIndex = 0;   /* Opcao do game over (0=Jogar Novamente, 1=Menu) */
int diffIndex = 0; // indice para navegação no submenu de dificuldade (0=Facil, 1=Medio, 2=Dificil)

/* Cache: rastreiam o ultimo estado exibido para redesenho incremental */
static int menuLastIndex      = -1;
static int gameoverLastIndex  = -1;


/* ================================================================
 * ARTE ASCII DO TITULO
 * ================================================================ */

/* Arte do titulo "SNAKE" em 6 linhas de ASCII art */
static const char *TITLE_ART[6] = {
    "  _____ _   _          _  _______",
    " / ____| \\ | |   /\\   | |/ /  ___|",
    "| (___ |  \\| |  /  \\  | ' /| |__  ",
    " \\___ \\| . ` | / /\\ \\ |  < |  __| ",
    " ____) | |\\  |/ ____ \\| . \\| |___ ",
    "|_____/|_| \\_/_/    \\_\\_|\\_\\_____|",
};

/* ================================================================
 * FUNCOES AUXILIARES INTERNAS
 * ================================================================ */

/*
 * screen_box: desenha uma caixa de tela com borda dupla e separador central.
 * 'frame_color' e para as bordas, 'accent_color' para o separador central.
 * O separador fica no meio vertical (my = (T+B)/2) para dividir a caixa.
 */
static void screen_box(int L, int T, int R, int B,
                        int frame_color, int accent_color) {
    int my = (T + B) / 2;   /* Linha central para o separador */

    set_color(frame_color);

    /* Linha superior */
    gotoxy(L, T);
    printf("%s", BOX_TLH);
    for (int x = L + 1; x < R; x++) printf("%s", BOX_DH);
    printf("%s", BOX_TRH);

    /* Linha inferior */
    gotoxy(L, B);
    printf("%s", BOX_BLH);
    for (int x = L + 1; x < R; x++) printf("%s", BOX_DH);
    printf("%s", BOX_BRH);

    /* Linhas laterais + separador central */
    for (int y = T + 1; y < B; y++) {
        if (y == my) {
            /* Linha central: separador com cor de destaque */
            set_color(accent_color);
            gotoxy(L, y); printf("%s", BOX_LM);              /* ╠ */
            for (int x = L + 1; x < R; x++) printf("%s", BOX_DH);  /* ═ */
            gotoxy(R, y); printf("%s", BOX_RM);              /* ╣ */
        } else {
            /* Linhas normais: apenas vertical */
            set_color(frame_color);
            gotoxy(L, y); printf("%s", BOX_DV);
            gotoxy(R, y); printf("%s", BOX_DV);
        }
    }
    set_color(C_RESET);
    fflush(stdout);
}

/*
 * screen_sep: linha separadora horizontal dentro de uma caixa.
 */
static void screen_sep(int L, int y, int R, int color) {
    set_color(color);
    gotoxy(L + 1, y);
    printf("%s", BOX_LM);                              /* ╠ juncao esquerda */
    for (int x = L + 2; x < R; x++) printf("%s", BOX_DH);  /* ═ horizontal */
    printf("%s", BOX_RM);                              /* ╣ juncao direita */
    set_color(C_RESET);
    fflush(stdout);
}

/*
 * screen_clear: preenche o interior da caixa com espacos (limpa conteudo anterior).
 */
static void screen_clear(int L, int T, int R, int B) {
    for (int y = T + 1; y < B; y++) {
        gotoxy(L + 1, y);
        for (int x = L + 1; x < R; x++) putchar(' ');
    }
    fflush(stdout);
}

/*
 * screen_center: centraliza texto entre L e R em uma linha y.
 */
static void screen_center(int L, int R, int y, int color, const char *text) {
    int w       = R - L - 1;             /* Largura util */
    int len     = (int)strlen(text);     /* Comprimento em bytes */
    int x       = L + 1 + (w - len) / 2;  /* Posicao centralizada */
    if (x < L + 1) x = L + 1;           /* Garante limite esquerdo */
    gotoxy(x, y);
    set_color(color);
    printf("%s", text);
    set_color(C_RESET);
    fflush(stdout);
}

/*
 * screen_cursor: cursor piscante (> ou espaco) baseado em timer.
 * Usa GetTickCount() para piscar independentemente do frame rate do jogo.
 * 'selected': 1 = esta linha esta selecionada, 0 = nao.
 */
static void screen_cursor(int x, int y, int selected) {
    static DWORD last = 0;    /* Timestamp da ultima troca de estado */
    static int   on   = 1;    /* Estado atual: 1=visivel, 0=oculto */
    DWORD now = GetTickCount();
    if (now - last > 450) {   /* Pisca a cada 450ms */
        on   = !on;
        last = now;
    }
    gotoxy(x, y);
    if (selected && on) { set_color(14); printf(">"); }   /* Cursor visivel: ">" amarelo */
    else                { set_color(0);  printf(" "); }   /* Cursor oculto: espaco preto */
    set_color(C_RESET);
    fflush(stdout);
}

/*
 * screen_dots: linha de pontos decorativos (. . . . .).
 */
static void screen_dots(int L, int y, int R, int color) {
    set_color(color);
    gotoxy(L + 2, y);
    for (int x = L + 2; x < R - 1; x++) printf((x % 2 == 0) ? "." : " ");
    set_color(C_RESET);
    fflush(stdout);
}

/*
 * draw_title: renderiza a arte ASCII do titulo em posicao centralizada.
 * 'cx' e o centro horizontal, 'ty' e a linha do topo.
 * Alterna cores entre linhas para efeito visual degradee.
 */
static void draw_title(int cx, int ty) {
    int cols[6] = { 10, 10, 11, 11, 10, 10 };   /* Cores por linha: verde/ciano */
    for (int r = 0; r < 6; r++) {
        int len = (int)strlen(TITLE_ART[r]);
        gotoxy(cx - len / 2, ty + r);             /* Centraliza cada linha */
        set_color(cols[r]);
        printf("%s", TITLE_ART[r]);
    }
    set_color(C_RESET);
    fflush(stdout);
}

/* ================================================================
 * TELA: MENU PRINCIPAL
 * ================================================================ */

/*
 * render_menu_screen: menu principal com redesenho incremental.
 *
 * O bloco 'if (menuNeedsRedraw)' so executa quando a tela e exibida
 * pela primeira vez ou apos uma transicao de volta ao menu.
 * O cursor e os itens sao atualizados a cada frame (rapido).
 */
void render_menu_screen(void) {

    /* --- Redesenho completo (uma vez por visita ao menu) --- */
    if (menuNeedsRedraw) {
        clear_screen();   /* Limpa tudo antes de redesenhar */

        int L = 12, R = 67, T = 1, B = 30;   /* Dimensoes da caixa do menu */
        int cx = (L + R) / 2;                  /* Centro horizontal */

        screen_box(L, T, R, B, 10, 14);    /* Caixa verde com separador amarelo */
        screen_clear(L, T, R, B);           /* Limpa interior */

        /* Arte do titulo "SNAKE" */
        draw_title(cx, T + 2);

        /* Linha de pontos decorativa e subtitulo */
        screen_dots(L, T + 9, R, 2);
        screen_center(L, R, T + 10, 8, "B F S   E D I T I O N");

        /* Subtitulo com emoji de cerebro */
        char sub[64];
        snprintf(sub, sizeof(sub), "%s Cobra Inteligente com BFS %s", SYM_BRAIN, SYM_BRAIN);
        screen_center(L, R, T + 11, 3, sub);

        screen_sep(L, T + 12, R, 10);   /* Separador apos subtitulo */

        /* Rodape com instrucoes de controle */
        screen_sep(L, B - 2, R, 2);
        screen_center(L, R, B - 1, 8, "W / S   MOVER        ENTER   CONFIRMAR");

        menuNeedsRedraw = 1;     /* Permite redesenho incremental dos itens */
        menuNeedsRedraw = 0;     /* Marca que a estrutura foi desenhada */
        menuLastIndex   = -1;    /* Forca redesenho dos itens na proxima vez */
    }

    /* --- Cursor piscante (atualizado a cada frame) --- */
    {
        int L = 12, R = 67, T = 1;
        int cx = (L + R) / 2;
        /* Um cursor para cada uma das 4 opcoes */
        for (int i = 0; i < 4; i++)
            screen_cursor(cx - 15, T + 14 + i * 3, i == menuIndex);
    }

    /* --- Itens do menu (apenas quando a selecao muda) --- */
    if (menuLastIndex != menuIndex) {
        int L = 12, R = 67, T = 1;

        /* Labels e cores de cada opcao */
        const char *labels[] = { "JOGAR MANUAL", "JOGAR AUTO (BFS)", "RANKING", "SAIR" };
        int hiCol[]          = {    10,                 9,               14,       12   };

        for (int i = 0; i < 4; i++) {
            int oy = T + 14 + i * 3;   /* Linha Y desta opcao */

            /* Limpa a linha antes de redesenhar */
            gotoxy(L + 2, oy);
            for (int x = L + 2; x < R - 1; x++) putchar(' ');

            if (i == menuIndex) {
                /* Opcao selecionada: destaque com << >> e cor propria */
                char line[40];
                snprintf(line, sizeof(line), "[ %s ]", labels[i]);
                gotoxy(L + 3, oy); set_color(hiCol[i]); printf("<<");  /* Seta esquerda */
                gotoxy(R - 4, oy); set_color(hiCol[i]); printf(">>");  /* Seta direita */
                screen_center(L, R, oy, hiCol[i], line);
            } else {
                /* Opcao normal: texto cinza */
                screen_center(L, R, oy, 8, labels[i]);
            }
        }

        /* High score na parte inferior do menu */
        {
            int rs = best_score();
            char rec[40];
            snprintf(rec, sizeof(rec), "HIGH  SCORE     %06d", rs);
            gotoxy(L + 2, T + 27);
            for (int x = L + 2; x < R - 1; x++) putchar(' ');  /* Limpa linha */
            screen_center(L, R, T + 27, rs > 0 ? 14 : 8, rec);  /* Amarelo se tem score */
        }

        menuLastIndex = menuIndex;   /* Atualiza cache */
    }
}

/*=======================================================
    sub menu de dificuldade
=========================================================*/
void render_difficulty_screen(void) {
    if (menuNeedsRedraw) {
        int L = 12, R = 67, T = 5, B = 20;// Dimensoes da caixa do submenu de dificuldade
        screen_box(L, T, R, B, 11, 14); // Caixa Ciano
        screen_center(L, R, T + 2, 14, "SELECIONE A DIFICULDADE");
        menuNeedsRedraw = 0;
    }
    
    const char *opts[] = { "FACIL (Sem Obstaculos)", "MEDIO (Poucos Obstaculos)", "DIFICIL (Muitos Obstaculos)" };
    for (int i = 0; i < 3; i++) {
        int color = (i == diffIndex) ? 14 : 8; // Amarelo se selecionado
        screen_center(12, 67, 10 + (i * 2), color, opts[i]);
    }
}
/* ================================================================
 * TELA: RANKING
 * ================================================================ */

/*
 * render_ranking_screen: exibe a tabela de recordes.
 *
 * Redesenhada apenas uma vez (rankingNeedsRedraw) pois e estatica:
 * nao tem cursor nem elementos que mudam enquanto e exibida.
 */
void render_ranking_screen(void) {
    if (!rankingNeedsRedraw) return;   /* Sem mudancas: pula o redesenho */

    clear_screen();

    int L = 12, R = 67, T = 2, B = 28;

    screen_box(L, T, R, B, 14, 11);   /* Caixa amarela com separador ciano */
    screen_clear(L, T, R, B);

    screen_center(L, R, T + 1, 14, "H A L L   O F   F A M E");
    screen_sep(L, T + 2, R, 14);
    screen_center(L, R, T + 3, 8, "#      NOME                     SCORE");
    screen_sep(L, T + 4, R, 8);

    int count = 0;
    load_ranking(&count);   /* Le o arquivo de ranking */

    if (count == 0) {
        /* Ranking vazio: mensagem centralizada */
        screen_center(L, R, T + 13, 8, "- - -  SEM SCORES  - - -");
    } else {
        /* Posicoes com labels e cores diferentes */
        const char *pos[]  = { "1ST", "2ND", "3RD", "4TH", "5TH" };
        int posCol[]       = {   14,    11,    13,     7,     7  };
        /* Cores: 1o=amarelo, 2o=ciano, 3o=magenta, 4o e 5o=branco normal */

        for (int i = 0; i < count; i++) {
            char line[64];
            int y = T + 6 + i * 3;   /* Cada entrada ocupa 3 linhas */

            /* Primeiro lugar tem linhas de destaque acima e abaixo */
            if (i == 0) {
                set_color(14);
                gotoxy(L + 2, y - 1);
                for (int x = L + 2; x < R - 1; x++) printf("%s", BOX_DH);  /* Linha acima */
                gotoxy(L + 2, y + 1);
                for (int x = L + 2; x < R - 1; x++) printf("%s", BOX_DH);  /* Linha abaixo */
                set_color(C_RESET);
                fflush(stdout);
            }

            /* Linha com posicao, nome e score */
            snprintf(line, sizeof(line), "[%s]   %-18s   %06d",
                     pos[i], ranking[i].name, ranking[i].score);
            screen_center(L, R, y, posCol[i], line);
        }
    }

    /* Rodape com instrucao de saida */
    screen_sep(L, B - 2, R, 14);
    screen_center(L, R, B - 1, 8, "ENTER / ESC     VOLTAR AO MENU");

    rankingNeedsRedraw = 0;   /* Nao redesenha ate proxima visita */
}

/* ================================================================
 * TELA: GAME OVER
 * ================================================================ */

/*
 * render_gameover_screen: exibe estatisticas da partida encerrada e opcoes.
 *
 * Dois niveis de redesenho:
 *   1. Estatisticas (gameoverNeedsRedraw): desenhadas uma vez
 *   2. Opcoes (gameoverLastIndex): atualizadas quando a selecao muda
 */
void render_gameover_screen(void) {

    /* --- Redesenho das estatisticas (uma vez por game over) --- */
    if (gameoverNeedsRedraw) {
        clear_screen();

        int L = 12, R = 67, T = 2, B = 28;

        screen_box(L, T, R, B, 12, 14);   /* Caixa vermelha com separador amarelo */
        screen_clear(L, T, R, B);

        screen_center(L, R, T + 1, 12, "G A M E    O V E R");
        screen_sep(L, T + 2, R, 12);

        /* Calcula tempo total jogado (descontando pausas) */
        int elapsed = (int)(pauseStart > startTime
                            ? pauseStart - startTime
                            : time(NULL) - startTime) - pausedSeconds;
        if (elapsed < 0) elapsed = 0;

        char buf[48];

        /* Exibe cada estatistica centralizada */
        snprintf(buf, sizeof(buf), "SCORE     >>   %06d", score);
        screen_center(L, R, T + 4, 14, buf);

        snprintf(buf, sizeof(buf), "NIVEL     >>   %d", level);
        screen_center(L, R, T + 6, 10, buf);

        snprintf(buf, sizeof(buf), "TEMPO     >>   %02d : %02d", elapsed/60, elapsed%60);
        screen_center(L, R, T + 8, 13, buf);

        snprintf(buf, sizeof(buf), "FRUTAS    >>   %d", fruitsEaten);
        screen_center(L, R, T + 10, 11, buf);

        snprintf(buf, sizeof(buf), "COMBO     >>   %d x", comboCount);
        screen_center(L, R, T + 12, 12, buf);

        snprintf(buf, sizeof(buf), "MODO      >>   %s", autoMode ? "BFS AUTO" : "MANUAL");
        screen_center(L, R, T + 14, autoMode ? 9 : 10, buf);

        /* Mensagem especial se bateu o recorde */
        if (score > 0 && score >= best_score()) {
            screen_sep(L, T + 15, R, 14);
            screen_center(L, R, T + 16, 14, "* *  N O V O   R E C O R D E  * *");
        }

        /* Instrucoes de controle */
        screen_sep(L, B - 3, R, 12);
        screen_center(L, R, B - 2, 8, "W / S   MOVER        ENTER   CONFIRMAR");

        gameoverNeedsRedraw = 0;
        gameoverLastIndex   = -1;   /* Forca redesenho das opcoes */
    }

    /* --- Redesenho das opcoes (quando a selecao muda) --- */
    if (gameoverLastIndex != gameOverIndex) {
        int L = 12, R = 67, T = 2, B = 28;

        const char *opts[] = { "JOGAR  NOVAMENTE", "VOLTAR  AO  MENU" };
        int hiCol[]        = {        10,                  8           };

        for (int i = 0; i < 2; i++) {
            int oy = B - 5 + i * 2;   /* Linha Y de cada opcao */

            /* Limpa a linha */
            gotoxy(L + 2, oy);
            for (int x = L + 2; x < R - 1; x++) putchar(' ');

            if (i == gameOverIndex) {
                /* Opcao selecionada com destaque */
                char line[40];
                snprintf(line, sizeof(line), "[ %s ]", opts[i]);
                gotoxy(L + 3, oy); set_color(hiCol[i]); printf("<<");
                gotoxy(R - 4, oy); set_color(hiCol[i]); printf(">>");
                screen_center(L, R, oy, hiCol[i], line);
            } else {
                screen_center(L, R, oy, 8, opts[i]);
            }
        }

        gameoverLastIndex = gameOverIndex;
    }
}
