/*
 * console.c
 * ---------
 * Implementacao das utilidades de console Windows.
 * Isola completamente a API Win32 neste modulo, facilitando
 * uma eventual portabilidade para outros sistemas operacionais.
 */

#include <stdio.h>      /* printf, putchar, fflush */
#include <string.h>     /* strlen */
#include <windows.h>    /* HANDLE, COORD, SMALL_RECT, SetConsole* */

#include "../include/console.h"     /* Propria interface */
#include "../include/constants.h"   /* C_RESET, CON_COLS, CON_ROWS */

/* ----------------------------------------------------------------
 * Definicao real do handle global (declarado como 'extern' em console.h).
 * GetStdHandle(STD_OUTPUT_HANDLE) retorna o handle do console de saida.
 * Todos os modulos que incluem console.h enxergam este mesmo handle.
 * ---------------------------------------------------------------- */
HANDLE hConsole = INVALID_HANDLE_VALUE;

/* ----------------------------------------------------------------
 * gotoxy: move o cursor para a coluna x, linha y do console.
 *
 * COORD e uma struct Win32 com campos X e Y do tipo SHORT.
 * SetConsoleCursorPosition aplica a posicao ao handle do console.
 * Sem esta funcao precisariamos de sequencias ANSI "\033[y;xH"
 * que nem sempre funcionam no Windows CMD classico.
 * ---------------------------------------------------------------- */
void gotoxy(int x, int y) {
    COORD c;
    c.X = (SHORT)x;    /* Cast necessario: COORD usa SHORT, nao int */
    c.Y = (SHORT)y;
    SetConsoleCursorPosition(hConsole, c);  /* Aplica a posicao */
}

/* ----------------------------------------------------------------
 * set_color: define a cor de texto para proximas escritas no console.
 *
 * SetConsoleTextAttribute aceita um WORD com bits de cor:
 *   bits 0-3: cor do texto (foreground)
 *   bits 4-7: cor do fundo (background)
 * Valores 0-15 correspondem a combinacoes RGB + intensidade.
 * ---------------------------------------------------------------- */
void set_color(int color) {
    SetConsoleTextAttribute(hConsole, (WORD)color);
}

/* ----------------------------------------------------------------
 * hide_cursor: oculta o cursor piscante para visual mais limpo.
 *
 * CONSOLE_CURSOR_INFO tem dois campos:
 *   dwSize:   tamanho percentual do cursor (1-100), ignorado quando oculto
 *   bVisible: FALSE = oculto, TRUE = visivel
 * ---------------------------------------------------------------- */
void hide_cursor(void) {
    CONSOLE_CURSOR_INFO ci;
    ci.dwSize   = 1;       /* Tamanho minimo (irrelevante quando oculto) */
    ci.bVisible = FALSE;   /* Oculta o cursor */
    SetConsoleCursorInfo(hConsole, &ci);
}

/* ----------------------------------------------------------------
 * show_cursor: exibe o cursor quando o jogador precisa digitar.
 *
 * dwSize = 20 corresponde a um cursor normal (20% da altura da celula).
 * ---------------------------------------------------------------- */
void show_cursor(void) {
    CONSOLE_CURSOR_INFO ci;
    ci.dwSize   = 20;    /* Tamanho padrao do cursor */
    ci.bVisible = TRUE;  /* Torna o cursor visivel */
    SetConsoleCursorInfo(hConsole, &ci);
}

/* ----------------------------------------------------------------
 * clear_screen: apaga todo o conteudo visivel do console.
 *
 * system("cls") e simples mas bloqueia e causa flicker.
 * Aceitavel apenas em transicoes de tela (nao no loop de jogo).
 * ---------------------------------------------------------------- */
void clear_screen(void) {
    system("cls");  /* Limpa o console via comando do sistema */
}

/* ----------------------------------------------------------------
 * write_at: escreve texto colorido em uma posicao especifica.
 *
 * Sequencia: posiciona -> coloriza -> imprime -> reset de cor.
 * O reset ao final garante que codigo posterior nao herde a cor.
 * ---------------------------------------------------------------- */
void write_at(int x, int y, int color, const char *text) {
    gotoxy(x, y);           /* Move cursor para posicao */
    set_color(color);        /* Aplica cor desejada */
    printf("%s", text);      /* Imprime o texto */
    set_color(C_RESET);      /* Restaura cor padrao */
    fflush(stdout);          /* Forca saida imediata (sem buffering) */
}

/* ----------------------------------------------------------------
 * fill_spaces: sobrescreve 'n' posicoes com espacos brancos.
 *
 * Tecnica de "apagar sem redesenhar tudo": muito mais rapido do
 * que chamar clear_screen(). Usado para apagar valores desatualizados
 * no painel lateral antes de escrever os novos.
 * ---------------------------------------------------------------- */
void fill_spaces(int x, int y, int n) {
    gotoxy(x, y);                  /* Posiciona no inicio da area */
    for (int i = 0; i < n; i++)   /* Imprime n espacos consecutivos */
        putchar(' ');
    fflush(stdout);                /* Garante saida imediata */
}

/* ----------------------------------------------------------------
 * trim_newline: remove '\n' e '\r' do fim de uma string.
 *
 * fgets() preserva o '\n' final da entrada do usuario.
 * Sem remover, o nome salvo no ranking ficaria "Joao\n".
 * Itera de traz para frente ate encontrar char que nao e newline.
 * ---------------------------------------------------------------- */
void trim_newline(char *s) {
    size_t n = strlen(s);                                   /* Comprimento atual */
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r'))   /* Enquanto termina com newline */
        s[--n] = '\0';                                       /* Substitui por null terminator */
}

/* ----------------------------------------------------------------
 * console_init: configura o ambiente do console para o jogo.
 *
 * Passos:
 * 1. Ativa codepage UTF-8 para exibir emojis e box-drawing chars.
 * 2. Obtem handle do console de saida.
 * 3. Redimensiona o buffer do console (scroll area).
 * 4. Ajusta o tamanho da janela visivel.
 * 5. Define o titulo da janela.
 *
 * Retorna 0 em sucesso, 1 se o handle for invalido.
 * ---------------------------------------------------------------- */
int console_init(void) {
    /* Ativa UTF-8 para saida e entrada do console */
    SetConsoleOutputCP(CP_UTF8);  /* CP_UTF8 = 65001 */
    SetConsoleCP(CP_UTF8);

    /* Obtem handle do console de saida (stdout do console) */
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE) {
        printf("Erro ao obter o console.\n");  /* Mensagem de erro basica */
        return 1;  /* Sinal de falha para main() */
    }

    /* Define o tamanho do buffer de texto (area de scroll interno) */
    COORD bufSize;
    bufSize.X = (SHORT)CON_COLS;   /* Largura total necessaria */
    bufSize.Y = (SHORT)CON_ROWS;   /* Altura total necessaria */
    SetConsoleScreenBufferSize(hConsole, bufSize);

    /* Define a area visivel da janela do console (sem scroll bars) */
    SMALL_RECT winRect;
    winRect.Left   = 0;
    winRect.Top    = 0;
    winRect.Right  = (SHORT)(CON_COLS - 1);   /* Coluna final */
    winRect.Bottom = (SHORT)(CON_ROWS - 1);   /* Linha final */
    SetConsoleWindowInfo(hConsole, TRUE, &winRect);

    /* Titulo que aparece na barra da janela */
    SetConsoleTitleA("SNAKE BFS - Cobra Inteligente");

    return 0;  /* Sucesso */
}
