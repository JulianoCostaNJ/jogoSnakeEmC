/*
 * screens.h
 * ---------
 * Interface das telas de UI: menu, ranking e game over.
 *
 * Cada tela tem uma funcao de renderizacao que:
 *   1. Usa um flag *NeedsRedraw para desenhar a estrutura estatica apenas uma vez
 *   2. Atualiza apenas os elementos dinamicos (cursor piscante, opcao selecionada)
 *
 * Este padrao de "redraw incremental" evita flickering ao redesenhar
 * a tela inteira a cada frame mesmo quando so o cursor mudou.
 */

#ifndef SCREENS_H
#define SCREENS_H

/* ----------------------------------------------------------------
 * Flags de controle de redesenho (1 = precisa redesenhar tudo).
 * Sao setados para 1 nas transicoes de estado para forcar redesenho
 * completo quando a tela e exibida pela primeira vez.
 * ---------------------------------------------------------------- */
extern int menuNeedsRedraw;
extern int gameoverNeedsRedraw;
extern int rankingNeedsRedraw;

/* Indice da opcao selecionada em cada tela de selecao */
extern int menuIndex;      /* 0-3: opcoes do menu principal */
extern int gameOverIndex;  /* 0-1: "Jogar Novamente" ou "Voltar ao Menu" */
extern int diffIndex; // indice para navegação no submenu de dificuldade (0=Facil, 1=Medio, 2=Dificil)


/* ----------------------------------------------------------------
 * render_menu_screen: renderiza o menu principal.
 *
 * Componentes:
 *   - Arte ASCII do titulo "SNAKE"
 *   - Subtitulo "BFS Edition" com emoji de cerebro
 *   - 4 opcoes: Jogar Manual, Jogar Auto, Ranking, Sair
 *   - Cursor piscante na opcao atual (usando GetTickCount para timing)
 *   - High score atual
 *
 * Redesenho:
 *   - Estrutura completa: apenas quando menuNeedsRedraw = 1
 *   - Cursor: a cada chamada (pisca independente do redraw)
 *   - Opcoes: apenas quando menuIndex muda
 * ---------------------------------------------------------------- */
void render_menu_screen(void);
/*------------------------------------------------------------------
 * render_difficulty_screen: renderiza o submenu de dificuldade.
 *
 * Componentes:
 *   - Titulo "SELECIONE A DIFICULDADE"
 *   - 3 opcoes: Facil, Medio, Dificil
 *   - Opcao selecionada em amarelo, as outras em cinza
 *  - Cursor de selecao (setas << >>) ao redor da opcao selecionada
 * - Redesenho:
 *  - Estrutura completa: apenas quando menuNeedsRedraw = 1 (ao entrar no submenu)
 * - Opcoes: apenas quando diffIndex muda
 *------------------------------------------------------------------*/
void render_difficulty_screen(void);

/* ----------------------------------------------------------------
 * render_ranking_screen: exibe a tabela de recordes.
 *
 * Mostra ate MAX_RANKING entradas com posicao (1ST, 2ND, etc.),
 * nome e pontuacao. O primeiro lugar tem destaque especial.
 * Redesenhado apenas uma vez por visita (rankingNeedsRedraw).
 * ---------------------------------------------------------------- */
void render_ranking_screen(void);

/* ----------------------------------------------------------------
 * render_gameover_screen: exibe a tela de fim de jogo.
 *
 * Mostra:
 *   - Score, nivel, tempo, frutas, combo e modo da partida encerrada
 *   - Mensagem de novo recorde (se aplicavel)
 *   - Opcoes: "Jogar Novamente" ou "Voltar ao Menu"
 *
 * Redesenho:
 *   - Estatisticas: apenas quando gameoverNeedsRedraw = 1
 *   - Opcoes: quando gameOverIndex muda
 * ---------------------------------------------------------------- */
void render_gameover_screen(void);

#endif /* SCREENS_H */
