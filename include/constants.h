/*
 * constants.h
 * -----------
 * Centraliza TODAS as constantes, macros, enums e structs do jogo.
 * Separar aqui evita "magic numbers" espalhados pelo codigo e
 * facilita ajustes de balanceamento sem mexer em logica.
 *
 * Compilacao (exemplo):
 *   gcc src/main.c src/*.c -o snake_bfs.exe -std=c11
 */

#ifndef CONSTANTS_H   /* Guard de inclusao multipla */
#define CONSTANTS_H

#include <windows.h>  /* HANDLE, COORD, SHORT - API do console Windows */

/* ================================================================
 * DIMENSOES DO TABULEIRO / LAYOUT
 * ================================================================ */

#define WIDTH        40   /* Colunas logicas da arena (celulas de jogo) */
#define HEIGHT       26   /* Linhas totais da arena (inclui bordas) */
#define TOP           3   /* Linha Y onde a borda superior da arena fica */

/* Coluna X esquerda da arena no console */
#define ARENA_LEFT    1
/* Coluna X direita da arena: cada celula logica ocupa 2 chars + bordas */
#define ARENA_RIGHT  (ARENA_LEFT + WIDTH * 2 + 1)

/* Posicao X onde o painel lateral começa */
#define PANEL_X      (ARENA_RIGHT + 3)
/* Largura do painel lateral em caracteres */
#define PANEL_W       24

/* Tamanho total do buffer do console em colunas e linhas */
#define CON_COLS     (PANEL_X + PANEL_W + 2)
#define CON_ROWS      35

/* ================================================================
 * CONFIGURACAO DO JOGO
 * ================================================================ */

#define FRUIT_COUNT    6    /* Quantas frutas existem simultaneamente na arena */
#define MAX_SNAKE    800    /* Tamanho maximo que a cobra pode atingir */
#define MAX_RANKING    5    /* Posicoes salvas no ranking */
#define RANKING_FILE  "ranking.txt"  /* Arquivo de persistencia do ranking */
#define OBSTACLE_COUNT 20   /* Quantos obstaculos sao gerados por partida */

#define INITIAL_SPEED 140   /* Delay inicial em ms entre cada frame */
#define MIN_SPEED      50   /* Delay minimo (velocidade maxima) em ms */
#define LEVEL_STEP     80   /* Pontos necessarios para subir de nivel */

/* ================================================================
 * TAMANHO MAXIMO DA FILA BFS
 * ================================================================ */

/* A arena tem WIDTH*HEIGHT celulas no maximo, entao a fila nunca
 * precisara de mais que isso durante a busca em largura */
#define BFS_MAX (WIDTH * HEIGHT)

/* ================================================================
 * SIMBOLOS UNICODE / EMOJI (UTF-8 encoded)
 * ================================================================
 * Cada sequencia \xNN representa um byte UTF-8.
 * Ex.: SYM_HEAD = U+1F7E2 (circulo verde) = F0 9F 9F A2
 */

/* --- Cobra --- */
#define SYM_HEAD       "\xF0\x9F\x9F\xA2"  /* 🟢 Cabeca da cobra (modo manual) */
#define SYM_HEAD_AUTO  "\xF0\x9F\xA4\x96"  /* 🤖 Cabeca da cobra (modo automatico) */
#define SYM_BODY       "\xF0\x9F\x9F\xA9"  /* 🟩 Segmento do corpo */
#define SYM_TAIL       "\xF0\x9F\x9F\xA8"  /* 🟨 Segmento da cauda */
#define SYM_BODY_FAST  "\xF0\x9F\x94\xB5"  /* 🔵 Corpo em alta velocidade */

/* --- Frutas --- */
#define SYM_APPLE      "\xF0\x9F\x8D\x8E"  /* 🍎 Maca (valor 10) */
#define SYM_GRAPE      "\xF0\x9F\x8D\x87"  /* 🍇 Uva (valor 20) */
#define SYM_CHERRY     "\xF0\x9F\x8D\x92"  /* 🍒 Cereja (valor 30) */
#define SYM_STAR       "\xE2\xAD\x90"       /* ⭐ Estrela (valor 50) */
#define SYM_GEM        "\xF0\x9F\x92\x8E"  /* 💎 Gema (valor 80) */

/* --- HUD / Efeitos visuais --- */
#define SYM_PAUSE      "\xE2\x8F\xB8"       /* ⏸ Icone de pausa */
#define SYM_SKULL      "\xF0\x9F\x92\x80"  /* 💀 Caveira (game over) */
#define SYM_TROPHY     "\xF0\x9F\x8F\x86"  /* 🏆 Trofeu */
#define SYM_SPEED_SLOW "\xF0\x9F\x90\xA2"  /* 🐢 Velocidade lenta */
#define SYM_SPEED_MED  "\xF0\x9F\x9A\xB6"  /* 🚶 Velocidade media */
#define SYM_SPEED_FAST "\xF0\x9F\x9A\x80"  /* 🚀 Velocidade rapida */
#define SYM_SPEED_INS  "\xE2\x9A\xA1"       /* ⚡ Velocidade insana */
#define SYM_SNAKE_TITLE "\xF0\x9F\x90\x8D" /* 🐍 Cobra decorativa */
#define SYM_CROWN      "\xF0\x9F\x91\x91"  /* 👑 Coroa de recorde */
#define SYM_CLOCK      "\xE2\x8F\xB1"       /* ⏱ Relogio */
#define SYM_CTRL       "\xF0\x9F\x8E\xAE"  /* 🎮 Controle */
#define SYM_FIRE       "\xF0\x9F\x94\xA5"  /* 🔥 Fogo (combo) */
#define SYM_HEART      "\xE2\x9D\xA4"       /* ❤ Coracao */
#define SYM_ROBOT      "\xF0\x9F\xA4\x96"  /* 🤖 Robo (modo auto) */
#define SYM_BRAIN      "\xF0\x9F\xA7\xA0"  /* 🧠 Cerebro (BFS) */
#define SYM_PATH       "\xC2\xB7"           /* · Ponto medio (caminho BFS) */
#define SYM_OBSTACLE   "\xE2\x96\xA0"       /* ■ Quadrado cheio (obstaculo) */

/* --- Caracteres de borda (box-drawing UTF-8) --- */
#define BOX_TL   "\xE2\x94\x8C"  /* ┌ canto superior esquerdo simples */
#define BOX_TR   "\xE2\x94\x90"  /* ┐ canto superior direito simples */
#define BOX_BL   "\xE2\x94\x94"  /* └ canto inferior esquerdo simples */
#define BOX_BR   "\xE2\x94\x98"  /* ┘ canto inferior direito simples */
#define BOX_H    "\xE2\x94\x80"  /* ─ horizontal simples */
#define BOX_V    "\xE2\x94\x82"  /* │ vertical simples */
#define BOX_TLH  "\xE2\x95\x94"  /* ╔ canto superior esquerdo duplo */
#define BOX_TRH  "\xE2\x95\x97"  /* ╗ canto superior direito duplo */
#define BOX_BLH  "\xE2\x95\x9A"  /* ╚ canto inferior esquerdo duplo */
#define BOX_BRH  "\xE2\x95\x9D"  /* ╝ canto inferior direito duplo */
#define BOX_DH   "\xE2\x95\x90"  /* ═ horizontal duplo */
#define BOX_DV   "\xE2\x95\x91"  /* ║ vertical duplo */
#define BOX_LM   "\xE2\x95\xA0"  /* ╠ juncao esquerda dupla */
#define BOX_RM   "\xE2\x95\xA3"  /* ╣ juncao direita dupla */
#define BLOCK    "\xE2\x96\x93"   /* ▓ bloco cheio (parede lateral) */
#define DOT_BG   "\xC2\xB7"       /* · ponto de fundo do tabuleiro */

/* --- Barra de progresso --- */
#define BAR_FULL "\xE2\x96\x88"   /* █ bloco completo */
#define BAR_HALF "\xE2\x96\x91"   /* ░ bloco vazio */

/* ================================================================
 * CORES DO CONSOLE WINDOWS (atributos SetConsoleTextAttribute)
 * ================================================================
 * Valores 0-15: combinacao de bits FOREGROUND_RED/GREEN/BLUE/INTENSITY
 */

#define C_RESET      7   /* Branco normal (padrao do console) */
#define C_BORDER    11   /* Ciano claro (bordas) */
#define C_PANEL     11   /* Ciano claro (painel lateral) */
#define C_PANEL_HDR 14   /* Amarelo (cabecalhos do painel) */
#define C_PANEL_VAL 15   /* Branco brilhante (valores do painel) */
#define C_SCORE     14   /* Amarelo (pontuacao) */
#define C_LEVEL     10   /* Verde brilhante (nivel) */
#define C_TIME      13   /* Magenta claro (tempo) */
#define C_SPEED     12   /* Vermelho claro (velocidade) */
#define C_RECORD    10   /* Verde brilhante (recorde) */
#define C_RUNNING   10   /* Verde brilhante (status jogando) */
#define C_PAUSED    12   /* Vermelho claro (status pausado) */
#define C_BG         8   /* Cinza escuro (fundo da arena) */
#define C_TITLE     14   /* Amarelo (titulos) */
#define C_SELECT    14   /* Amarelo (item selecionado) */
#define C_NORMAL     7   /* Branco normal (texto normal) */
#define C_FLASH     15   /* Branco brilhante (efeitos) */
#define C_FLASH2    14   /* Amarelo (efeitos secundarios) */
#define C_GAMEOVER  12   /* Vermelho claro (game over) */
#define C_BEST      10   /* Verde brilhante (melhor score) */
#define C_WALL       8   /* Cinza (parede) */
#define C_COMBO     12   /* Vermelho claro (combo) */
#define C_BAR_FILL  10   /* Verde (barra de progresso preenchida) */
#define C_BAR_EMPTY  8   /* Cinza (barra de progresso vazia) */

/* Cores de borda por nivel (ciclam a cada 5 niveis) */
#define C_BORDER_L1 11   /* Nivel 1: ciano claro */
#define C_BORDER_L2 10   /* Nivel 2: verde brilhante */
#define C_BORDER_L3 14   /* Nivel 3: amarelo */
#define C_BORDER_L4 12   /* Nivel 4: vermelho claro */
#define C_BORDER_L5 13   /* Nivel 5: magenta claro */

/* Cores especificas da BFS */
#define C_BFS_PATH   9   /* Azul claro (celulas do caminho encontrado) */
#define C_BFS_VISIT  2   /* Verde escuro (celulas visitadas) */
#define C_OBSTACLE   4   /* Vermelho escuro (obstaculos) */
#define C_MODE_MAN  10   /* Verde (indicador modo manual) */
#define C_MODE_AUTO  9   /* Azul claro (indicador modo automatico) */
#define C_BFS_INFO   3   /* Ciano escuro (informacoes BFS no painel) */

/* ================================================================
 * TIPOS DE DADOS
 * ================================================================ */

/*
 * Segment: representa uma posicao logica (coluna x, linha y) no tabuleiro.
 * Usado para celulas da cobra, obstaculos e no algoritmo BFS.
 * Coordenadas logicas: x em [1..WIDTH], y em [TOP+1..HEIGHT-1]
 */
typedef struct {
    int x;  /* Coluna logica (1 = primeira coluna jogavel) */
    int y;  /* Linha logica (TOP+1 = primeira linha jogavel) */
} Segment;

/*
 * Fruit: representa uma fruta posicionada na arena.
 * Cada fruta tem posicao, pontuacao, simbolo visual, cor e raridade.
 */
typedef struct {
    int         x;       /* Coluna logica da fruta */
    int         y;       /* Linha logica da fruta */
    int         value;   /* Pontos concedidos ao comer */
    const char *symbol;  /* Ponteiro para o emoji/simbolo UTF-8 */
    int         color;   /* Cor do console para renderizar */
    int         rarity;  /* 0=comum, 1=raro, 2=epico (afeta flash ao comer) */
} Fruit;

/*
 * RankEntry: um registro no ranking (nome do jogador + pontuacao).
 */
typedef struct {
    char name[32];  /* Nome do jogador (max 31 chars + null terminator) */
    int  score;     /* Pontuacao final da partida */
} RankEntry;

/*
 * GameState: maquina de estados principal do jogo.
 * O loop em main.c comuta entre esses estados baseado em input/eventos.
 */
typedef enum {
    STATE_MENU = 0,   /* Exibindo menu principal */
    STATE_DIFFICULTY, /* Exibindo selecao de dificuldade */
    STATE_PLAYING,    /* Partida em andamento */
    STATE_RANKING,    /* Exibindo tabela de recordes */
    STATE_GAMEOVER,   /* Partida encerrada, aguardando acao */
    STATE_EXIT        /* Sair do programa */
} GameState;

/*
 * Difficulty: niveis de dificuldade do jogo, pode em implementações futuras
 * afetar a velocidade inicial e crescimento da cobra.
 * Usados para balanceamento e desafio progressivo.
 */

typedef enum {
    DIFF_EASY = 0,
    DIFF_MEDIUM,
    DIFF_HARD
} Difficulty;
/*
 * Direction: direcao de movimento da cobra.
 * Importante: direcoes opostas (UP<->DOWN, LEFT<->RIGHT) nao podem ser
 * selecionadas consecutivamente para evitar colisao instantanea.
 */
typedef enum {
    DIR_UP = 0,   /* Mover para cima (y diminui) */
    DIR_RIGHT,    /* Mover para direita (x aumenta) */
    DIR_DOWN,     /* Mover para baixo (y aumenta) */
    DIR_LEFT      /* Mover para esquerda (x diminui) */
} Direction;

/*
 * BfsNode: celula usada na fila do algoritmo BFS e no caminho resultante.
 * Equivalente a Segment, mas semanticamente separado para clareza.
 */
typedef struct {
    int x;  /* Coluna logica da celula */
    int y;  /* Linha logica da celula */
} BfsNode;

#endif /* CONSTANTS_H */
