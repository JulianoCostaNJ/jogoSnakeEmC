/*
 * game.c
 * ------
 * Implementacao da logica central do jogo: estado, inicializacao e update.
 *
 * Este e o modulo mais importante: ele conecta BFS, render e input.
 * O fluxo de um frame e:
 *   input.c -> altera nextDir
 *   game.c  -> update_game() le nextDir, move cobra, detecta eventos
 *   render.c -> le o estado e redesenha apenas o que mudou
 */

#include <stdio.h>     /* printf, fflush */
#include <stdlib.h>    /* rand, abs */
#include <string.h>    /* memset */
#include <time.h>      /* time */
#include <windows.h>   /* Sleep */

#include "../include/game.h"       /* Propria interface */
#include "../include/constants.h"  /* Todas as constantes */
#include "../include/bfs.h"        /* find_nearest_fruit_bfs, bfs_path */
#include "../include/render.h"     /* draw_*, erase_cell, update_side_panel */
#include "../include/effects.h"    /* flash_eat, anim_level_up */
#include "../include/console.h"    /* console_init, hide_cursor, show_cursor, clear_screen */

/* ================================================================
 * TABELA DE FRUTAS
 * Define as propriedades de cada tipo de fruta possivel.
 * Separada aqui para facil balanceamento sem tocar na logica.
 * ================================================================ */
static const struct {
    int         value;    /* Pontos que a fruta concede */
    const char *symbol;   /* Emoji/simbolo UTF-8 */
    int         color;    /* Cor do console */
    int         rarity;   /* 0=comum, 1=raro, 2=epico */
} FRUIT_TABLE[] = {
    { 10, SYM_APPLE,  13, 0 },   /* Maca: mais comum, menos pontos */
    { 20, SYM_GRAPE,  13, 0 },   /* Uva: comum */
    { 30, SYM_CHERRY, 12, 0 },   /* Cereja: comum */
    { 50, SYM_STAR,   14, 1 },   /* Estrela: rara */
    { 80, SYM_GEM,    11, 2 },   /* Gema: epica, mais valiosa */
};
#define FRUIT_TABLE_LEN 5   /* Numero de tipos de fruta */

/* ================================================================
 * DEFINICOES DAS VARIAVEIS EXTERNAS (declaradas como extern em game.h)
 * ================================================================ */

Segment   snake[MAX_SNAKE];          /* Corpo da cobra */
int       snakeLen   = 0;            /* Comprimento atual */
Direction dir        = DIR_RIGHT;    /* Direcao corrente */
Direction nextDir    = DIR_RIGHT;    /* Direcao solicitada pelo input */

Fruit     fruits[FRUIT_COUNT];       /* Frutas ativas na arena */

Segment   obstacles[OBSTACLE_COUNT]; /* Posicoes dos obstaculos */
int       obstacleCount = 0;         /* Obstaculos gerados */
int activeObstacleLimit = 20; /* Limite de obstaculos ativos, ajustado pela dificuldade */ 

int score         = 0;    /* Pontuacao */
int level         = 1;    /* Nivel atual */
int speedMs       = INITIAL_SPEED;  /* Velocidade em ms */
int fruitsEaten   = 0;    /* Total de frutas comidas */
int comboCount    = 0;    /* Combo atual */

time_t startTime;         /* Inicio da partida */
time_t pauseStart;        /* Inicio da pausa atual */
int    pausedSeconds = 0; /* Segundos pausados acumulados */

int paused   = 0;   /* Flag de pausa */
int gameOver = 0;   /* Flag de game over */

int autoMode     = 0;   /* Modo de controle */
int showBfsPath  = 1;   /* Exibir caminho BFS */
int showBfsVisit = 0;   /* Exibir visitados BFS */

/* Cache do painel (para desenho incremental) */
int lastElapsed = -1;
int lastLevel   = -1;
int lastScore   = -1;
int lastFruits  = -1;
int lastCombo   = -1;

/* ================================================================
 * FUNCOES DE CONSULTA
 * ================================================================ */

/*
 * is_on_snake: percorre todo o array snake[] procurando (x,y).
 * Complexidade O(snakeLen): aceitavel pois snakeLen raramente passa de 100.
 */
int is_on_snake(int x, int y) {
    for (int i = 0; i < snakeLen; i++)         /* Itera cada segmento */
        if (snake[i].x == x && snake[i].y == y)
            return 1;   /* Posicao ocupada pela cobra */
    return 0;           /* Posicao livre */
}

/*
 * is_obstacle: percorre obstacles[] procurando (x,y).
 * Complexidade O(OBSTACLE_COUNT): sempre pequeno (20 obstaculos).
 */

void set_difficulty(Difficulty diff) {
    if (diff == DIFF_EASY)   activeObstacleLimit = 0;
    else if (diff == DIFF_MEDIUM) activeObstacleLimit = 8;
    else                          activeObstacleLimit = 20; // Hard (macro original)
}

int is_obstacle(int x, int y) {
    for (int i = 0; i < obstacleCount; i++)
        if (obstacles[i].x == x && obstacles[i].y == y)
            return 1;   /* Ha obstaculo nesta posicao */
    return 0;
}

/*
 * fruit_at: retorna o indice [0..FRUIT_COUNT-1] da fruta em (x,y).
 * Retorna -1 se nao houver fruta.
 */
int fruit_at(int x, int y) {
    for (int i = 0; i < FRUIT_COUNT; i++)
        if (fruits[i].x == x && fruits[i].y == y)
            return i;   /* Fruta encontrada: retorna seu indice */
    return -1;          /* Sem fruta nesta posicao */
}

/*
 * arena_col: converte coordenada logica x em coluna real do console.
 *
 * Layout do console (cada celula logica ocupa 2 colunas):
 *   ARENA_LEFT = borda esquerda
 *   ARENA_LEFT+1 = primeira celula logica x=1 (col console 2)
 *   x=2 fica na coluna console 4, x=3 na 6, etc.
 *
 * Formula: (x-1)*2 desloca para a celula certa, +ARENA_LEFT+1 adiciona offset da borda.
 */
int arena_col(int logical_x) {
    return ARENA_LEFT + 1 + (logical_x - 1) * 2;
}

/* ================================================================
 * SPAWN DE OBSTACULOS
 * ================================================================ */

/*
 * spawn_obstacles: gera os obstaculos da partida.
 *
 * Estrategia:
 *   - Tenta posicionar OBSTACLE_COUNT obstaculos em locais aleatorios
 *   - Rejeita posicoes: sobre a cobra, sobre outro obstaculo, perto do centro
 *   - O centro (mx, my) e onde a cobra inicia, entao reserva uma area segura
 *   - Maximo de 200 tentativas por obstaculo para evitar loop infinito
 */
void spawn_obstacles(void) {
    obstacleCount = 0;
    // ... lógica de sorteio ...
    /* Centro da arena: onde a cobra comeca */
    int mx = WIDTH / 2;
    int my = (TOP + HEIGHT) / 2;
       
    for (int i = 0; i < activeObstacleLimit; i++) { 
        // Lógica de geração atual...
         int x, y, ok;
        int attempts = 0;

        do {
            attempts++;
            if (attempts > 200) break;   /* Evita loop infinito se arena lotada */
            ok = 1;   /* Assume posicao valida ate provar contrario */

            /* Posicao aleatoria dentro dos limites jogaveis */
            x = rand() % WIDTH + 1; 
            y = rand() % (HEIGHT - TOP - 1) + TOP + 1;

            /* Zona de segurança ao redor do spawn da cobra */
            if (abs(x - mx) <= 5 && abs(y - my) <= 2) { ok = 0; continue; }

            /* Nao pode sobrepor a cobra */
            if (is_on_snake(x, y)) { ok = 0; continue; }

            /* Nao pode sobrepor outro obstaculo ja colocado */
            if (is_obstacle(x, y)) { ok = 0; continue; }

        } while (!ok);

        if (attempts <= 200) {   /* Conseguiu posicionar dentro do limite */
            obstacles[obstacleCount].x = x;
            obstacles[obstacleCount].y = y;
            obstacleCount++;
        }
    }
    
}

/* ================================================================
 * SPAWN DE FRUTAS
 * ================================================================ */

/*
 * spawn_fruit: posiciona a fruta de indice 'i' em local aleatorio livre.
 *
 * A animacao de spawn (piscada) e feita em render.c (spawn_fruit_anim).
 *
 * Sistema de raridade (rolagem de dado 0-99):
 *   0- 9 (10%): Gema     (epico,  valor 80)
 *  10-39 (30%): Estrela  (raro,   valor 50)
 *  40-99 (60%): Maca/Uva/Cereja (comum, valores 10-30)
 */
void spawn_fruit(int i) {
    int x, y, ok;

    /* Loop ate encontrar posicao livre */
    do {
        ok = 1;
        x = rand() % WIDTH + 1;                       /* Coluna aleatoria [1..WIDTH] */
        y = rand() % (HEIGHT - TOP - 1) + TOP + 1;    /* Linha aleatoria [TOP+1..HEIGHT-1] */

        if (is_on_snake(x, y))  { ok = 0; continue; }   /* Sobre a cobra */
        if (is_obstacle(x, y))  { ok = 0; continue; }   /* Sobre obstaculo */

        /* Verifica conflito com outras frutas ja posicionadas */
        for (int j = 0; j < FRUIT_COUNT; j++) {
            if (j != i && fruits[j].x == x && fruits[j].y == y) {
                ok = 0; break;   /* Ja tem fruta aqui */
            }
        }
    } while (!ok);

    /* Determina o tipo da fruta por probabilidade */
    int roll = rand() % 100;   /* Numero aleatorio [0..99] */
    int fi;                     /* Indice na FRUIT_TABLE */
    if      (roll < 10) fi = 4;          /* 10%: Gema */
    else if (roll < 40) fi = 3;          /* 30%: Estrela */
    else                fi = rand() % 3; /* 60%: Maca/Uva/Cereja (escolha aleatoria entre as 3) */

    /* Aplica as propriedades do tipo escolhido */
    fruits[i].x      = x;
    fruits[i].y      = y;
    fruits[i].value  = FRUIT_TABLE[fi].value;
    fruits[i].symbol = FRUIT_TABLE[fi].symbol;
    fruits[i].color  = FRUIT_TABLE[fi].color;
    fruits[i].rarity = FRUIT_TABLE[fi].rarity;

    /* Chama animacao visual de spawn (render.c) */
    spawn_fruit_anim(i);
}

/* ================================================================
 * INICIALIZACAO DA PARTIDA
 * ================================================================ */

/*
 * init_game: prepara uma partida nova do zero.
 *
 * Ordem de operacoes importa:
 * 1. Reseta variaveis de estado
 * 2. Posiciona cobra no centro
 * 3. Limpa tela e redesenha estrutura estatica
 * 4. Gera obstaculos (antes das frutas para evitar conflito)
 * 5. Posiciona frutas (depois da cobra e obstaculos)
 * 6. Calcula BFS inicial
 * 7. Registra timestamp de inicio
 */
void init_game(void) {
    /* --- Reset de todos os contadores e flags --- */
    snakeLen      = 5;            /* Cobra inicia com 5 segmentos */
    score         = 0;
    level         = 1;
    speedMs       = INITIAL_SPEED;
    paused        = 0;
    gameOver      = 0;
    pausedSeconds = 0;
    fruitsEaten   = 0;
    comboCount    = 0;
    bfs_path_len  = 0;
    bfs_visited_count = 0;

    /* Invalida cache do painel para forcar redesenho completo */
    lastElapsed = -1;
    lastLevel   = -1;
    lastScore   = -1;
    lastFruits  = -1;
    lastCombo   = -1;

    /* --- Posiciona cobra no centro da arena --- */
    int mx = WIDTH / 2;           /* Coluna central */
    int my = (TOP + HEIGHT) / 2;  /* Linha central */

    /* Cobra vai da cabeca (mx, my) ate a cauda (mx - snakeLen + 1, my)
     * movendo-se para a direita inicialmente */
    for (int i = 0; i < snakeLen; i++) {
        snake[i].x = mx - i;   /* Cada segmento um passo para a esquerda */
        snake[i].y = my;        /* Todos na mesma linha */
    }

    dir     = DIR_RIGHT;   /* Direcao inicial: direita */
    nextDir = DIR_RIGHT;

    /* --- Limpeza e desenho inicial --- */
    clear_screen();        /* Apaga conteudo anterior (system cls) */
    spawn_obstacles();     /* Gera novos obstaculos aleatorios */

    /* Desenha elementos estaticos da arena */
    draw_board_border();      /* Borda da arena com cor do nivel */
    draw_board_background();  /* Fundo pontilhado */
    draw_all_obstacles();     /* Obstaculos vermelhos */
    draw_side_panel_frame();  /* Estrutura do painel lateral (nao os valores) */

    /* Desenha cobra inicial (do final para o inicio para sobreposicao correta) */
    for (int i = snakeLen - 1; i >= 0; i--) {
        if (i == 0)              draw_head(snake[i].x, snake[i].y);       /* Cabeca */
        else if (i == snakeLen-1) draw_tail_seg(snake[i].x, snake[i].y); /* Cauda */
        else                     draw_body(snake[i].x, snake[i].y);      /* Corpo */
    }

    /* Spawna as frutas (com animacao visual) */
    for (int i = 0; i < FRUIT_COUNT; i++)
        spawn_fruit(i);

    /* Calcula e exibe o primeiro caminho BFS */
    find_nearest_fruit_bfs();
    draw_bfs_visited_overlay();  /* Celulas visitadas (se showBfsVisit=1) */
    draw_bfs_path_overlay();     /* Caminho sugerido (se showBfsPath=1) */

    /* Registra o momento de inicio da partida */
    startTime = time(NULL);

    /* Atualiza painel com valores iniciais */
    update_side_panel();
}

/* ================================================================
 * LOOP DE ATUALIZACAO (um frame por chamada)
 * ================================================================ */

/*
 * update_game: processa um tick do jogo.
 *
 * Chamado no loop principal de STATE_PLAYING com delay de speedMs ms.
 * Nao e chamado durante pause ou game over.
 */
void update_game(void) {
    /* Verifica se devemos processar este frame */
    if (paused || gameOver) return;

    /* --- Limpa overlay BFS anterior para redesenho limpo --- */
    clear_bfs_overlay();   /* Apaga pontos azuis/verdes do frame anterior */

    /* ========================================================
     * MODO AUTOMATICO: BFS decide a proxima direcao
     * ======================================================== */
    if (autoMode) {
        find_nearest_fruit_bfs();   /* Recalcula melhor caminho atual */

        if (bfs_path_len > 0) {
            /* Proximo passo do caminho: bfs_path[0] e o primeiro no apos a cabeca */
            int nx = bfs_path[0].x;   /* Coluna do proximo passo */
            int ny = bfs_path[0].y;   /* Linha do proximo passo */
            int hx = snake[0].x;      /* Coluna atual da cabeca */
            int hy = snake[0].y;      /* Linha atual da cabeca */

            /* Calcula o delta e converte para direcao */
            int ddx = nx - hx;   /* +1 = direita, -1 = esquerda */
            int ddy = ny - hy;   /* +1 = baixo,   -1 = cima */

            if      (ddy == -1) nextDir = DIR_UP;
            else if (ddx ==  1) nextDir = DIR_RIGHT;
            else if (ddy ==  1) nextDir = DIR_DOWN;
            else if (ddx == -1) nextDir = DIR_LEFT;
            /* Se delta invalido: mantem direcao atual (segurança) */
        }
        /* Se bfs_path_len == 0: sem caminho, cobra continua em frente e provavelmente morre */
    }

    /* ========================================================
     * APLICA DIRECAO E CALCULA NOVA POSICAO DA CABECA
     * ======================================================== */
    dir = nextDir;   /* Efetiva a direcao (pode ter sido mudada pelo input ou BFS) */

    int nx = snake[0].x;   /* Nova coluna da cabeca (começa na posicao atual) */
    int ny = snake[0].y;   /* Nova linha da cabeca */

    /* Aplica o movimento conforme a direcao */
    if      (dir == DIR_UP)    ny--;   /* Sobe: y diminui */
    else if (dir == DIR_RIGHT) nx++;   /* Direita: x aumenta */
    else if (dir == DIR_DOWN)  ny++;   /* Desce: y aumenta */
    else if (dir == DIR_LEFT)  nx--;   /* Esquerda: x diminui */

    /* ========================================================
     * DETECCAO DE COLISOES
     * ======================================================== */

    /* Colisao com bordas da arena */
    if (nx < 1 || nx > WIDTH || ny <= TOP || ny >= HEIGHT) {
        gameOver = 1;   /* Bateu na parede -> fim de jogo */
        return;
    }

    /* Colisao com obstaculo fixo */
    if (is_obstacle(nx, ny)) {
        gameOver = 1;   /* Bateu em obstaculo -> fim de jogo */
        return;
    }

    /* Verifica se ha fruta na nova posicao da cabeca */
    int fruitIndex = fruit_at(nx, ny);

    if (fruitIndex == -1) {
        /* Sem fruta: verifica colisao com corpo proprio (exceto a cauda,
         * pois ela vai se mover e liberar a posicao) */
        for (int i = 0; i < snakeLen - 1; i++) {
            if (snake[i].x == nx && snake[i].y == ny) {
                gameOver = 1;   /* Bateu no proprio corpo */
                return;
            }
        }
    } else {
        /* Com fruta: o corpo vai crescer (cauda nao se move),
         * entao verifica colisao com TODO o corpo */
        for (int i = 0; i < snakeLen; i++) {
            if (snake[i].x == nx && snake[i].y == ny) {
                gameOver = 1;   /* Nao deveria acontecer, mas previne bugs */
                return;
            }
        }
    }

    /* ========================================================
     * MOVIMENTO DA COBRA (sem colisao: pode mover)
     * ======================================================== */

    /* Desenha o antigo local da cabeca como corpo antes de mover */
    draw_body(snake[0].x, snake[0].y);

    /* Salva a posicao da cauda ANTES de deslocar o array
     * (necessario para apagar a cauda depois, no caso sem fruta) */
    Segment oldTail = snake[snakeLen - 1];

    /* Desloca todos os segmentos uma posicao para tras no array:
     * snake[1] = snake[0], snake[2] = snake[1], ...
     * Isso "empurra" o corpo seguindo a cabeca */
    for (int i = snakeLen; i > 0; i--)
        snake[i] = snake[i - 1];

    /* Nova posicao da cabeca */
    snake[0].x = nx;
    snake[0].y = ny;

    /* ========================================================
     * PROCESSAMENTO DE EVENTO: COMEU FRUTA
     * ======================================================== */
    if (fruitIndex != -1) {
        int rarity = fruits[fruitIndex].rarity;   /* Para o efeito visual */

        score     += fruits[fruitIndex].value;    /* Adiciona pontos */
        snakeLen++;                                /* Cobra cresce (cauda nao e apagada) */
        if (snakeLen >= MAX_SNAKE) snakeLen = MAX_SNAKE - 1;  /* Limite de segurança */
        fruitsEaten++;   /* Estatistica */
        comboCount++;    /* Incrementa combo (reset apenas na morte) */

        /* Verifica subida de nivel */
        int newLevel = score / LEVEL_STEP + 1;   /* Nivel calculado pelo score */
        int didLevel = (newLevel > level);         /* Subiu de nivel neste frame? */
        if (didLevel) {
            level   = newLevel;
            speedMs -= 8;   /* Reduz delay -> aumenta velocidade */
            if (speedMs < MIN_SPEED) speedMs = MIN_SPEED;   /* Nao ultrapassar limite */
        }

        /* Gera nova fruta no lugar da que foi comida */
        spawn_fruit(fruitIndex);

        /* Redesenha cauda (o ultimo segmento e o novo, pois cobra cresceu) */
        draw_tail_seg(snake[snakeLen - 1].x, snake[snakeLen - 1].y);
        draw_head(nx, ny);   /* Desenha nova cabeca */

        /* Recalcula BFS com o novo estado (cobra maior, nova fruta) */
        find_nearest_fruit_bfs();
        draw_bfs_visited_overlay();
        draw_bfs_path_overlay();

        /* Atualiza painel lateral com nova pontuacao */
        update_side_panel();

        /* Efeitos visuais conforme raridade da fruta */
        flash_eat(rarity);
        if (didLevel) {
            anim_level_up();       /* Animacao de subida de nivel */
            draw_board_border();   /* Redesenha borda com nova cor de nivel */
        }

    /* ========================================================
     * PROCESSAMENTO DE EVENTO: MOVIMENTO NORMAL (sem comer)
     * ======================================================== */
    } else {
        /* Apaga a posicao da antiga cauda (ela "saiu" do corpo) */
        erase_cell(oldTail.x, oldTail.y);

        /* Redesenha o novo ultimo segmento como cauda */
        draw_tail_seg(snake[snakeLen - 1].x, snake[snakeLen - 1].y);

        /* Desenha nova posicao da cabeca */
        draw_head(nx, ny);

        /* Recalcula BFS a cada passo (cobra se moveu, caminho mudou) */
        find_nearest_fruit_bfs();
        draw_bfs_visited_overlay();
        draw_bfs_path_overlay();

        /* Atualiza painel apenas se algo mudou (economiza CPU) */
        int elapsed = (int)(time(NULL) - startTime) - pausedSeconds;
        if (elapsed != lastElapsed || lastScore != score)
            update_side_panel();
    }
}
