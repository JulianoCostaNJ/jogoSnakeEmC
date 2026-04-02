/*
 * game.h
 * ------
 * Interface do modulo de estado e logica do jogo.
 *
 * Este modulo e o "modelo" (no sentido MVC) do jogo:
 * armazena e modifica o estado de tudo que existe na arena
 * (cobra, frutas, obstaculos, pontuacao, etc.).
 *
 * Separado de render.h (que apenas le o estado para desenhar)
 * e de input.h (que apenas dispara mudancas de direcao).
 */

#ifndef GAME_H
#define GAME_H

#include "../include/constants.h"  /* Segment, Fruit, Direction, etc. */

/* ================================================================
 * ESTADO GLOBAL DA PARTIDA
 * Exportado como 'extern' para que render.c, bfs.c, effects.c
 * possam ler sem precisar de getters, mantendo o codigo simples.
 * ================================================================ */

/* --- Cobra --- */
extern Segment  snake[];      /* Array de segmentos: [0]=cabeca, [snakeLen-1]=cauda */
extern int      snakeLen;     /* Numero atual de segmentos da cobra */
extern Direction dir;         /* Direcao de movimento atual (aplicada no update) */
extern Direction nextDir;     /* Proxima direcao (aplicada no proximo update) */

/* --- Frutas --- */
extern Fruit  fruits[];       /* Array com todas as frutas ativas na arena */

/* --- Obstaculos --- */
extern Segment obstacles[];   /* Posicoes fixas dos obstaculos gerados aleatoriamente */
extern int     obstacleCount; /* Quantos obstaculos foram colocados */
extern int activeObstacleLimit; /* Limite de obstaculos ativos, ajustado pela dificuldade */

/* --- Pontuacao e progressao --- */
extern int score;         /* Pontuacao atual da partida */
extern int level;         /* Nivel atual (aumenta conforme score / LEVEL_STEP) */
extern int speedMs;       /* Delay em ms entre frames (menor = mais rapido) */
extern int fruitsEaten;   /* Total de frutas comidas nesta partida */
extern int comboCount;    /* Frutas comidas consecutivamente (sem morrer) */

/* --- Controle de tempo --- */
extern time_t startTime;      /* Timestamp Unix do inicio da partida */
extern time_t pauseStart;     /* Timestamp do inicio da pausa atual */
extern int    pausedSeconds;  /* Total de segundos ja pausados (acumulado) */

/* --- Flags de estado --- */
extern int paused;    /* 1 = jogo pausado, 0 = rodando */
extern int gameOver;  /* 1 = partida encerrada, 0 = em andamento */

/* --- Modo de jogo e visualizacao BFS --- */
extern int autoMode;     /* 0 = controle manual, 1 = BFS automatico */
extern int showBfsPath;  /* 1 = exibir caminho BFS na arena */
extern int showBfsVisit; /* 1 = exibir celulas visitadas pelo BFS na arena */

/* --- Cache do painel (evita redesenhar valores que nao mudaram) --- */
extern int lastElapsed;  /* Ultimo tempo exibido (segundos) */
extern int lastLevel;    /* Ultimo nivel exibido */
extern int lastScore;    /* Ultima pontuacao exibida */
extern int lastFruits;   /* Ultimo total de frutas exibido */
extern int lastCombo;    /* Ultimo combo exibido */

/* ================================================================
 * FUNCOES DE CONSULTA (podem ser chamadas por qualquer modulo)
 * ================================================================ */

/*
 * is_on_snake: verifica se a posicao (x,y) contem algum segmento da cobra.
 * Usado por spawn_fruit e spawn_obstacles para evitar sobreposicao.
 * Complexidade: O(snakeLen)
 */
int is_on_snake(int x, int y);

/*
 * is_obstacle: verifica se (x,y) e uma posicao de obstaculo.
 * Usado por BFS e deteccao de colisao.
 * Complexidade: O(obstacleCount)
 */
int is_obstacle(int x, int y);


/*
 * fruit_at: retorna o indice da fruta em (x,y), ou -1 se nao houver.
 * Usado na deteccao de colisao (update_game) e nos overlays BFS.
 * Complexidade: O(FRUIT_COUNT)
 */
int fruit_at(int x, int y);

/*
 * arena_col: converte coluna logica (1..WIDTH) em coluna de console.
 * Cada celula logica ocupa 2 colunas de console (para emojis de largura 2).
 * Formula: ARENA_LEFT + 1 + (logical_x - 1) * 2
 */
int arena_col(int logical_x);

/* ================================================================
 * FUNCOES DE LOGICA DO JOGO
 * ================================================================ */

/*
 * init_game: inicializa/reinicia todos os dados de uma nova partida.
 *   - Reseta cobra, pontuacao, nivel, velocidade
 *   - Gera obstaculos aleatorios
 *   - Posiciona frutas iniciais
 *   - Calcula o primeiro BFS
 *   - Desenha o estado inicial completo
 */
void init_game(void);
/* 
 * set_difficulty: define a dificuldade do jogo.
 * @param diff: nivel de dificuldade (Facil, Medio, Dificil)
 */
void set_difficulty(Difficulty diff);

/*
 * update_game: executa um frame do jogo.
 *   1. Se paused ou gameOver: retorna sem fazer nada
 *   2. No modo auto: usa BFS para decidir nextDir
 *   3. Aplica nextDir como dir
 *   4. Calcula nova posicao da cabeca
 *   5. Detecta colisoes (parede, obstaculo, corpo proprio)
 *   6. Se comeu fruta: aumenta cobra, spawna nova fruta, atualiza BFS
 *   7. Se nao comeu: move a cobra (apaga cauda, avanca corpo)
 *   8. Atualiza painel lateral
 */
void update_game(void);

/*
 * spawn_obstacles: gera OBSTACLE_COUNT obstaculos em posicoes aleatorias.
 * Garante que os obstaculos nao aparecem sobre a cobra ou perto do centro.
 */
void spawn_obstacles(void);

/*
 * spawn_fruit: posiciona a fruta de indice 'i' em local aleatorio livre.
 * Escolhe o tipo (value, symbol, rarity) com probabilidades ponderadas:
 *   10% de chance: Gema (valor 80)
 *   30% de chance: Estrela (valor 50)
 *   60% de chance: Maca/Uva/Cereja (valores 10-30)
 */
void spawn_fruit(int i);

#endif /* GAME_H */
