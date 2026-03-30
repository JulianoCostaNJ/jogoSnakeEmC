/*
 * bfs.c
 * -----
 * Implementacao do algoritmo Breadth-First Search (Busca em Largura).
 *
 * CONCEITO CENTRAL:
 *   BFS explora o grafo implicito da arena nivel a nivel:
 *   primeiro visita todos os vizinhos a distancia 1,
 *   depois distancia 2, e assim por diante.
 *   Isso garante que o PRIMEIRO caminho encontrado ate a fruta
 *   e sempre o MINIMO em numero de passos.
 *
 * ESTRUTURAS CONFORME AULA:
 *   Fila (FIFO)  -> bfs_queue[front..rear]
 *   Hash espacial -> bfs_visited[x][y]
 *   Parente      -> bfs_parent[x][y] (reconstroi caminho)
 */

#include <string.h>   /* memset, memcpy */
#include <stdlib.h>   /* abs */

#include "../include/bfs.h"        /* Propria interface */
#include "../include/constants.h"  /* BFS_MAX, WIDTH, HEIGHT, TOP */
#include "../include/game.h"       /* snake[], snakeLen, fruits[], is_obstacle, fruit_at */

/* ================================================================
 * DEFINICOES DAS VARIAVEIS EXTERNAS (declaradas como extern em bfs.h)
 * ================================================================ */

/* Fila circular: bfs_queue[bfs_front] e o proximo a sair,
   bfs_queue[bfs_rear] e a proxima posicao livre para inserir */
BfsNode bfs_queue[BFS_MAX];
int     bfs_front = 0;
int     bfs_rear  = 0;

/* Hash espacial: 1 = celula ja foi visitada nesta execucao do BFS */
int     bfs_visited[WIDTH + 1][HEIGHT + 1];

/* Grafo de parentes: para cada celula visitada, guarda de onde viemos.
   Isso forma uma "arvore de BFS" que permite reconstruir o caminho. */
BfsNode bfs_parent[WIDTH + 1][HEIGHT + 1];

/* Resultado: caminho da cabeca ate a fruta mais proxima */
BfsNode bfs_path[BFS_MAX];
int     bfs_path_len = 0;

/* Registro de todas as celulas visitadas para visualizacao educativa */
BfsNode bfs_visited_cells[BFS_MAX];
int     bfs_visited_count = 0;

/*
 * Vetores de direcao: 4 movimentos cardinais.
 * dx altera a coluna (x), dy altera a linha (y).
 * A ordem importa pois influencia qual caminho e descoberto primeiro
 * quando existem multiplos caminhos de mesmo comprimento.
 *
 *   i=0: ( 1, 0) = mover para baixo  (x+1, y)
 *   i=1: (-1, 0) = mover para cima   (x-1, y)
 *   i=2: ( 0, 1) = mover para direita (x, y+1)
 *   i=3: ( 0,-1) = mover para esquerda (x, y-1)
 */
int dx[4] = {  1, -1, 0,  0 };
int dy[4] = {  0,  0, 1, -1 };

/* ================================================================
 * FUNCOES AUXILIARES INTERNAS (static = visibilidade apenas neste .c)
 * ================================================================ */

/*
 * bfs_enqueue: insere um no na fila BFS.
 * Operacao O(1): simplesmente coloca na posicao bfs_rear e incrementa.
 * ATENCAO: nao verifica overflow (BFS_MAX deve ser suficiente para a arena).
 */
static void bfs_enqueue(int x, int y) {
    bfs_queue[bfs_rear].x = x;   /* Guarda coordenada x */
    bfs_queue[bfs_rear].y = y;   /* Guarda coordenada y */
    bfs_rear++;                   /* Avanca o indice de insercao */
}

/*
 * bfs_dequeue: remove e retorna o proximo no da fila.
 * Operacao O(1): retorna da posicao bfs_front e incrementa.
 * A fila e do tipo FIFO (First In, First Out), essencial para BFS.
 */
static BfsNode bfs_dequeue(void) {
    return bfs_queue[bfs_front++];   /* Retorna e avanca o indice de saida */
}

/*
 * is_free_for_bfs: verifica se uma celula pode ser visitada pelo BFS.
 *
 * Uma celula e "livre" se:
 *   1. Esta dentro dos limites da arena (x em [1..WIDTH], y em [TOP+1..HEIGHT-1])
 *   2. Nao e um obstaculo fixo
 *   3. Nao e um segmento do corpo da cobra (exceto a cabeca, que e o inicio)
 *
 * A cauda (ultimo segmento) sera liberada no proximo movimento,
 * mas por simplicidade tratamos todo o corpo como bloqueado.
 * Isso pode causar que o BFS evite caminhos que seriam validos
 * na pratica, mas e uma simplificacao aceitavel.
 */
static int is_free_for_bfs(int x, int y) {
    /* Verifica limites: x deve estar na arena, y deve estar entre as bordas */
    if (x < 1 || x > WIDTH || y <= TOP || y >= HEIGHT)
        return 0;  /* Fora dos limites -> bloqueado */

    /* Obstaculos fixos bloqueiam o caminho */
    if (is_obstacle(x, y))
        return 0;

    /* Corpo da cobra bloqueia (i=0 e a cabeca, ponto de partida, ja foi enfileirado) */
    for (int i = 1; i < snakeLen; i++) {   /* Comeca em 1 para pular a cabeca */
        if (snake[i].x == x && snake[i].y == y)
            return 0;  /* Segmento do corpo -> bloqueado */
    }

    return 1;  /* Celula livre para ser visitada */
}

/* ================================================================
 * ALGORITMO BFS PRINCIPAL
 * ================================================================ */

/*
 * run_bfs: Busca em Largura do ponto (sx,sy) ate (fx,fy).
 *
 * ALGORITMO (conforme slides da aula):
 *   1. Inicializa fila vazia, visited = falso para todos
 *   2. Enfileira o no inicial, marca como visitado
 *   3. Enquanto fila nao estiver vazia:
 *      a. Desenfileira no atual
 *      b. Se e o destino: reconstroi caminho e retorna 1
 *      c. Para cada vizinho valido nao visitado:
 *         - Marca como visitado
 *         - Guarda parente (de onde viemos)
 *         - Enfileira
 *   4. Se fila esvaziar sem encontrar destino: retorna 0
 *
 * Complexidade: O(V) onde V = numero de celulas livres na arena.
 */
int run_bfs(int sx, int sy, int fx, int fy) {
    /* --- Inicializacao: zera todas as estruturas --- */
    bfs_front = 0;            /* Reinicia ponteiro de saida da fila */
    bfs_rear  = 0;            /* Reinicia ponteiro de entrada da fila */
    bfs_path_len = 0;         /* Caminho anterior descartado */
    bfs_visited_count = 0;    /* Historico de visitados descartado */

    /* Zera o hash de visitados: memset e O(WIDTH*HEIGHT) mas rapido na pratica */
    memset(bfs_visited, 0, sizeof(bfs_visited));

    /* --- Passo 2: Enfileira o no inicial --- */
    bfs_enqueue(sx, sy);           /* Celula de partida entra na fila */
    bfs_visited[sx][sy] = 1;       /* Marca como visitada (evita revisitar) */

    /* Registra para visualizacao educativa */
    bfs_visited_cells[bfs_visited_count].x = sx;
    bfs_visited_cells[bfs_visited_count].y = sy;
    bfs_visited_count++;

    /* --- Passo 3: Loop principal da BFS --- */
    while (bfs_front != bfs_rear) {   /* Enquanto fila nao estiver vazia */

        BfsNode cur = bfs_dequeue();   /* Desenfileira o proximo no (FIFO) */

        /* --- Passo 3b: Verifica se chegamos ao destino --- */
        if (cur.x == fx && cur.y == fy) {

            /* RECONSTRUCAO DO CAMINHO usando a arvore de parentes:
             * Seguimos os parentes de volta do destino ate a origem.
             * O resultado fica em ordem inversa, entao invertemos no final. */

            BfsNode temp_path[BFS_MAX];  /* Buffer temporario para caminho invertido */
            int     temp_len = 0;

            BfsNode node;
            node.x = fx;   /* Comeca do destino (fruta) */
            node.y = fy;

            /* Sobe na arvore de parentes ate chegar na origem */
            while (!(node.x == sx && node.y == sy)) {
                temp_path[temp_len++] = node;                  /* Guarda no atual */
                node = bfs_parent[node.x][node.y];             /* Vai para o parente */
            }

            /* Inverte temp_path para obter a ordem correta (origem -> destino) */
            bfs_path_len = 0;
            for (int i = temp_len - 1; i >= 0; i--) {    /* Percorre ao contrario */
                bfs_path[bfs_path_len++] = temp_path[i]; /* Adiciona em ordem correta */
            }

            return 1;   /* Caminho encontrado! */
        }

        /* --- Passo 3c: Explora os 4 vizinhos --- */
        for (int i = 0; i < 4; i++) {
            int nx = cur.x + dx[i];   /* Coordenada x do vizinho */
            int ny = cur.y + dy[i];   /* Coordenada y do vizinho */

            /* Verificacoes em cascata (ordem importa por eficiencia):
             * 1o: limites -> mais barato checar antes de acessar arrays
             * 2o: visited -> evita processar duas vezes
             * 3o: is_free -> mais caro (itera corpo da cobra) */
            if (nx >= 1 && ny > TOP && nx <= WIDTH && ny < HEIGHT &&
                !bfs_visited[nx][ny] &&
                is_free_for_bfs(nx, ny)) {

                bfs_enqueue(nx, ny);          /* Coloca vizinho na fila */
                bfs_visited[nx][ny] = 1;      /* Marca como visitado imediatamente */
                bfs_parent[nx][ny] = cur;     /* Guarda de onde viemos */

                /* Registra para visualizacao da area explorada */
                if (bfs_visited_count < BFS_MAX) {
                    bfs_visited_cells[bfs_visited_count].x = nx;
                    bfs_visited_cells[bfs_visited_count].y = ny;
                    bfs_visited_count++;
                }
            }
        }
    }

    return 0;   /* Fila esvaziou sem encontrar destino: sem caminho possivel */
}

/* ================================================================
 * SELECAO DA FRUTA MAIS PROXIMA
 * ================================================================ */

/*
 * find_nearest_fruit_bfs: encontra a fruta alcancavel mais proxima.
 *
 * Executa BFS para cada uma das FRUIT_COUNT frutas e seleciona
 * a que resulta no menor bfs_path_len (menos passos).
 *
 * Salva o melhor caminho e os visitados correspondentes nos globais.
 * Retorna 1 se ao menos uma fruta e alcancavel, 0 caso contrario.
 *
 * Complexidade: O(FRUIT_COUNT * V) onde V = nos da arena.
 */
int find_nearest_fruit_bfs(void) {
    int     best_len           = 999999;      /* Melhor (menor) caminho encontrado */
    int     best_idx           = -1;          /* Indice da fruta mais proxima */
    BfsNode best_path[BFS_MAX];               /* Copia do melhor caminho */
    int     best_path_len      = 0;
    BfsNode best_visited[BFS_MAX];            /* Copia dos visitados do melhor BFS */
    int     best_visited_count = 0;

    /* Itera sobre todas as frutas ativas na arena */
    for (int i = 0; i < FRUIT_COUNT; i++) {
        /* Executa BFS da cabeca da cobra ate a fruta i */
        if (run_bfs(snake[0].x, snake[0].y, fruits[i].x, fruits[i].y)) {

            /* Se o caminho e menor que o melhor ate agora: salva */
            if (bfs_path_len < best_len) {
                best_len           = bfs_path_len;   /* Atualiza melhor comprimento */
                best_idx           = i;               /* Fruta alvo atual */
                best_path_len      = bfs_path_len;
                best_visited_count = bfs_visited_count;

                /* Copia o caminho e os visitados para nao perder ao proxima iteracao */
                memcpy(best_path,    bfs_path,          sizeof(BfsNode) * bfs_path_len);
                memcpy(best_visited, bfs_visited_cells, sizeof(BfsNode) * bfs_visited_count);
            }
        }
    }

    if (best_idx >= 0) {
        /* Restaura o melhor resultado encontrado nos globais */
        bfs_path_len      = best_path_len;
        bfs_visited_count = best_visited_count;
        memcpy(bfs_path,          best_path,    sizeof(BfsNode) * best_path_len);
        memcpy(bfs_visited_cells, best_visited, sizeof(BfsNode) * best_visited_count);
        return 1;   /* Fruta alcancavel encontrada */
    }

    /* Nenhuma fruta alcancavel: limpa resultados */
    bfs_path_len      = 0;
    bfs_visited_count = 0;
    return 0;
}

/* ================================================================
 * CONSULTAS (lidas por render.c e game.c)
 * ================================================================ */

/*
 * is_on_bfs_path: O(bfs_path_len) -> linear.
 * Poderia ser O(1) com uma matriz booleana, mas path_len e pequeno
 * e a simplicidade vale mais aqui.
 */
int is_on_bfs_path(int x, int y) {
    for (int i = 0; i < bfs_path_len; i++)       /* Percorre o caminho */
        if (bfs_path[i].x == x && bfs_path[i].y == y)
            return 1;   /* Celula encontrada no caminho */
    return 0;           /* Celula nao esta no caminho */
}

/*
 * is_bfs_visited: O(bfs_visited_count) -> linear.
 * Similar ao acima. Para grandes arenas considerar uma matriz booleana.
 */
int is_bfs_visited(int x, int y) {
    for (int i = 0; i < bfs_visited_count; i++)  /* Percorre visitados */
        if (bfs_visited_cells[i].x == x && bfs_visited_cells[i].y == y)
            return 1;   /* Celula foi visitada */
    return 0;           /* Celula nao foi visitada */
}
