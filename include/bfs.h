/*
 * bfs.h
 * -----
 * Interface do modulo de Busca em Largura (Breadth-First Search).
 *
 * O BFS e o "cerebro" do modo automatico e tambem gera a visualizacao
 * de caminho no modo manual. A abordagem usa um GRAFO IMPLICITO onde:
 *   - Nos    = celulas (x, y) da arena
 *   - Arestas = adjacencias nas 4 direcoes (cima, baixo, esq, dir)
 *   - Peso   = todos iguais (BFS garante caminho minimo em saltos)
 *
 * Estruturas de dados utilizadas (conforme aula):
 *   - Fila (bfs_queue[])     = Motor da BFS (FIFO via front/rear)
 *   - visited[][]            = Hash espacial (evita revisitar nos)
 *   - parent[][]             = Lista encadeada implicita (reconstroi caminho)
 *   - bfs_path[]             = Caminho minimo calculado (cabeca -> fruta)
 *   - bfs_visited_cells[]    = Todos nos visitados (para visualizacao)
 */

#ifndef BFS_H
#define BFS_H

#include "../include/constants.h"  /* BfsNode, BFS_MAX, WIDTH, HEIGHT */

/* ----------------------------------------------------------------
 * Estado interno do BFS exposto para leitura pelos modulos de render.
 *
 * Sao 'extern' porque bfs.c e quem aloca e preenche estas estruturas;
 * render.c e game.c apenas leem para exibir ou tomar decisoes.
 * ---------------------------------------------------------------- */

/* Fila da BFS: array circular com indice de inicio (front) e fim (rear) */
extern BfsNode bfs_queue[];
extern int     bfs_front;   /* Indice do proximo elemento a desenfileirar */
extern int     bfs_rear;    /* Indice onde o proximo elemento sera inserido */

/* Hash espacial: bfs_visited[x][y] = 1 se a celula ja foi visitada */
extern int     bfs_visited[WIDTH + 1][HEIGHT + 1];

/* Lista encadeada implicita: bfs_parent[x][y] = celula de onde viemos */
extern BfsNode bfs_parent[WIDTH + 1][HEIGHT + 1];

/* Caminho resultante: sequencia ordenada de celulas da cabeca ate a fruta */
extern BfsNode bfs_path[];
extern int     bfs_path_len;  /* Numero de celulas no caminho (0 = sem caminho) */

/* Todas as celulas visitadas durante a ultima execucao do BFS */
extern BfsNode bfs_visited_cells[];
extern int     bfs_visited_count;  /* Quantas celulas foram visitadas */

/* ----------------------------------------------------------------
 * Direcoes de adjacencia (4 direcoes cardinais).
 * dx[i] e o delta na coluna, dy[i] o delta na linha.
 * Pares: {+1,0}=baixo, {-1,0}=cima, {0,+1}=direita, {0,-1}=esquerda
 * ---------------------------------------------------------------- */
extern int dx[4];
extern int dy[4];

/* ----------------------------------------------------------------
 * run_bfs: executa o algoritmo BFS do ponto (sx,sy) ate (fx,fy).
 *
 * Preenche bfs_path[] com o caminho minimo e bfs_visited_cells[]
 * com todas as celulas exploradas.
 *
 * Retorna: 1 se encontrou caminho, 0 se nao existe caminho possivel.
 *          (pode nao existir se a cobra bloquear todos os caminhos)
 * ---------------------------------------------------------------- */
int run_bfs(int sx, int sy, int fx, int fy);

/* ----------------------------------------------------------------
 * find_nearest_fruit_bfs: executa BFS para cada fruta e escolhe
 * a que tem o caminho mais curto (menos passos ate a cobra).
 *
 * Atualiza bfs_path[] e bfs_visited_cells[] com o resultado otimo.
 * Retorna: 1 se encontrou ao menos uma fruta alcancavel, 0 caso contrario.
 * ---------------------------------------------------------------- */
int find_nearest_fruit_bfs(void);

/* ----------------------------------------------------------------
 * is_on_bfs_path: verifica se a celula (x,y) faz parte do caminho
 * atual calculado pelo BFS (bfs_path[]).
 * Usado por render.c para colorir as celulas do caminho.
 * ---------------------------------------------------------------- */
int is_on_bfs_path(int x, int y);

/* ----------------------------------------------------------------
 * is_bfs_visited: verifica se a celula (x,y) foi visitada na ultima
 * execucao do BFS (bfs_visited_cells[]).
 * Usado por render.c para exibir a area de busca explorada.
 * ---------------------------------------------------------------- */
int is_bfs_visited(int x, int y);

#endif /* BFS_H */
