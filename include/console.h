/*
 * console.h
 * ---------
 * Interface publica das funcoes utilitarias de console Windows.
 * Encapsula toda interacao com a API Win32 de console (HANDLE, COORD, etc.)
 * para que os demais modulos nao precisem incluir <windows.h> diretamente
 * alem do necessario.
 */

#ifndef CONSOLE_H
#define CONSOLE_H

#include <windows.h>   /* HANDLE - necessario para o tipo do handle global */

/* ----------------------------------------------------------------
 * Handle global do console de saida.
 * Declarado aqui como 'extern' para que todos os modulos possam acessar
 * o mesmo handle inicializado em console.c (definicao real esta la).
 * ---------------------------------------------------------------- */
extern HANDLE hConsole;

/* ----------------------------------------------------------------
 * Posiciona o cursor do console na coluna x, linha y (0-indexed).
 * Toda renderizacao do jogo usa esta funcao em vez de printf("\033[...").
 * ---------------------------------------------------------------- */
void gotoxy(int x, int y);

/* ----------------------------------------------------------------
 * Define a cor de texto do console usando os atributos Win32.
 * 'color' e um valor 0-15 conforme as constantes C_* em constants.h.
 * ---------------------------------------------------------------- */
void set_color(int color);

/* ----------------------------------------------------------------
 * Oculta o cursor piscante do console para visual limpo durante o jogo.
 * Chame no inicio da partida.
 * ---------------------------------------------------------------- */
void hide_cursor(void);

/* ----------------------------------------------------------------
 * Exibe o cursor do console. Chamado quando o jogador precisa digitar
 * seu nome no game over, ou ao encerrar o programa.
 * ---------------------------------------------------------------- */
void show_cursor(void);

/* ----------------------------------------------------------------
 * Limpa a tela completa usando system("cls").
 * Usado apenas nas transicoes entre telas (menu -> jogo, etc.)
 * para evitar artefatos visuais.
 * ---------------------------------------------------------------- */
void clear_screen(void);

/* ----------------------------------------------------------------
 * Combinacao de gotoxy + set_color + printf + reset.
 * Funcao de conveniencia para escrever texto colorido em uma posicao.
 * ---------------------------------------------------------------- */
void write_at(int x, int y, int color, const char *text);

/* ----------------------------------------------------------------
 * Preenche 'n' espacos em branco a partir de (x, y).
 * Usado para apagar texto antigo antes de redesenhar valores atualizados.
 * ---------------------------------------------------------------- */
void fill_spaces(int x, int y, int n);

/* ----------------------------------------------------------------
 * Remove '\n' e '\r' do final de uma string (resultado de fgets).
 * Necessario para salvar nomes no ranking sem quebras de linha.
 * ---------------------------------------------------------------- */
void trim_newline(char *s);

/* ----------------------------------------------------------------
 * Inicializa o handle do console, configura tamanho do buffer e janela,
 * define o titulo da janela e ativa UTF-8.
 * Deve ser a PRIMEIRA funcao chamada em main().
 * Retorna 0 em sucesso, 1 em erro (handle invalido).
 * ---------------------------------------------------------------- */
int console_init(void);

#endif /* CONSOLE_H */
