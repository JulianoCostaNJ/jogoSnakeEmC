/*
 * render.h
 * --------
 * Interface do modulo de renderizacao.
 *
 * Responsabilidade unica: LER o estado do jogo e DESENHAR no console.
 * Nunca modifica o estado do jogo (sem efeitos colaterais no modelo).
 *
 * Estrategia de desenho incremental:
 *   - Apenas redesenha o que mudou (evita flickering e lentidao)
 *   - Cada celula e redesenhada individualmente quando necessario
 *   - O painel lateral compara com cache (lastScore, lastLevel, etc.)
 */

#ifndef RENDER_H
#define RENDER_H

/* ================================================================
 * ARENA / TABULEIRO
 * ================================================================ */

/* Desenha a borda da arena com a cor do nivel atual */
void draw_board_border(void);

/* Preenche o interior da arena com o padrao de fundo pontilhado */
void draw_board_background(void);

/* ================================================================
 * ELEMENTOS DA ARENA
 * ================================================================ */

/* Desenha todos os obstaculos (usa cor vermelha C_OBSTACLE) */
void draw_all_obstacles(void);

/* Desenha um obstaculo especifico em (x, y) logico */
void draw_obstacle(int x, int y);

/* Apaga uma celula e restaura o fundo pontilhado */
void erase_cell(int x, int y);

/* ================================================================
 * COBRA
 * ================================================================ */

/* Desenha a cabeca da cobra em (x, y): emoji verde (manual) ou robo (auto) */
void draw_head(int x, int y);

/* Desenha um segmento de corpo em (x, y) */
void draw_body(int x, int y);

/* Desenha o ultimo segmento (cauda) em (x, y) */
void draw_tail_seg(int x, int y);

/* ================================================================
 * FRUTAS
 * ================================================================ */

/* Desenha a fruta de indice 'i' na sua posicao atual */
void draw_fruit(int i);

/* Animacao de spawn: pisca antes de exibir a fruta definitivamente */
void spawn_fruit_anim(int i);

/* ================================================================
 * OVERLAYS DO BFS
 * ================================================================ */

/* Desenha as celulas visitadas pelo BFS (pontos verdes, se showBfsVisit=1) */
void draw_bfs_visited_overlay(void);

/* Desenha o caminho BFS encontrado (pontos azuis, se showBfsPath=1) */
void draw_bfs_path_overlay(void);

/* Apaga os overlays BFS antigos antes do proximo frame */
void clear_bfs_overlay(void);

/* ================================================================
 * PAINEL LATERAL (HUD)
 * ================================================================ */

/* Desenha a estrutura estatica do painel (bordas, cabecalhos) - chamado uma vez */
void draw_side_panel_frame(void);

/* Atualiza os valores do painel (score, nivel, tempo, etc.) - chamado todo frame */
void update_side_panel(void);

/* Desenha barra de progresso horizontal no painel */
void draw_progress_bar(int x, int y, int bar_w, int value, int maxval);

/* ================================================================
 * CAIXAS E UTILIDADES DE LAYOUT
 * ================================================================ */

/* Desenha caixa com bordas simples (BOX_TL, BOX_H, etc.) */
void draw_box_unicode(int left, int top, int right, int bottom, int color);

/* Desenha caixa com bordas duplas (BOX_TLH, BOX_DH, etc.) */
void draw_box_double(int left, int top, int right, int bottom, int color);

/* Desenha separador horizontal duplo (linha ╠═══╣) */
void draw_separator_double(int left, int y, int right, int color);

/* Apaga o interior de uma caixa (preenche com espacos) */
void clear_inside_box(int left, int top, int right, int bottom);

/* Centraliza texto dentro de uma faixa horizontal [left..right] */
void print_centered_range(int left, int right, int y, int color, const char *text);

/* ================================================================
 * OVERLAY DE PAUSA
 * ================================================================ */

/* Exibe a caixa de pausa no centro da arena */
void draw_pause_overlay(void);

/* Remove a caixa de pausa e restaura o estado da arena */
void clear_pause_overlay(void);

/* ================================================================
 * CONSULTA AUXILIAR
 * ================================================================ */

/* Retorna a cor de borda correspondente ao nivel atual (ciclica a cada 5) */
int border_color_for_level(void);

/* Retorna o icone emoji da velocidade atual */
const char *speed_icon(void);

/* Retorna o label de texto da velocidade atual */
const char *speed_label(void);

#endif /* RENDER_H */
