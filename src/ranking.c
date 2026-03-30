/*
 * ranking.c
 * ---------
 * Implementacao do sistema de ranking com persistencia em arquivo.
 *
 * Formato do arquivo ranking.txt:
 *   Linha 1: "NomeJogador;1234\n"
 *   Linha 2: "OutroJogador;987\n"
 *   ...
 *
 * O separador ';' foi escolhido por ser raro em nomes de jogadores.
 */

#include <stdio.h>    /* fopen, fclose, fscanf, fprintf */
#include <string.h>   /* strncpy, strlen */
#include <stdlib.h>   /* (nao necessario aqui, mas bom para portabilidade) */

#include "../include/ranking.h"    /* Propria interface */
#include "../include/constants.h"  /* MAX_RANKING, RANKING_FILE, RankEntry */

/* Definicao do array global (declarado como extern em ranking.h) */
RankEntry ranking[MAX_RANKING];

/*
 * load_ranking: le o arquivo e preenche ranking[].
 *
 * Usa fscanf com formato "%31[^;];%d\n":
 *   %31[^;] -> le ate 31 chars sem incluir o ';' (nome)
 *   ;       -> consume o separador
 *   %d      -> le o numero inteiro (pontuacao)
 *   \n      -> consume a quebra de linha
 *
 * Retorna via parametro *count quantas entradas foram lidas.
 */
void load_ranking(int *count) {
    FILE *f = fopen(RANKING_FILE, "r");   /* Abre para leitura */
    *count = 0;

    if (!f) return;   /* Arquivo nao existe ainda: ranking vazio, sem erro */

    /* Le entradas ate encher o array ou acabar o arquivo */
    while (*count < MAX_RANKING &&
           fscanf(f, "%31[^;];%d\n",
                  ranking[*count].name,
                  &ranking[*count].score) == 2) {
        (*count)++;   /* Incrementa contador somente se leitura bem-sucedida */
    }

    fclose(f);   /* Sempre fecha o arquivo */
}

/*
 * save_ranking: escreve ranking[] no arquivo.
 *
 * Sobrescreve completamente o arquivo anterior ("w" = write, nao append).
 * O formato e "Nome;Pontuacao\n" para cada entrada.
 */
void save_ranking(int count) {
    FILE *f = fopen(RANKING_FILE, "w");   /* Abre para escrita (cria/sobrescreve) */
    if (!f) return;   /* Sem permissao de escrita: ignora silenciosamente */

    for (int i = 0; i < count; i++)
        fprintf(f, "%s;%d\n", ranking[i].name, ranking[i].score);

    fclose(f);
}

/*
 * best_score: retorna o maior score registrado.
 * O ranking esta sempre ordenado por score decrescente,
 * entao [0] e sempre o melhor.
 */
int best_score(void) {
    int count = 0;
    load_ranking(&count);   /* Recarrega do arquivo para garantir atualidade */
    return count ? ranking[0].score : 0;   /* 0 se ranking vazio */
}

/*
 * insert_ranking: tenta inserir uma nova entrada no ranking.
 *
 * Logica:
 *   1. Recarrega ranking do arquivo
 *   2. Se ha espaco livre: adiciona ao final
 *   3. Se nao ha espaco mas o novo score e maior que o ultimo: substitui o ultimo
 *   4. Se nao ha espaco e o score nao qualifica: retorna sem fazer nada
 *   5. Ordena por score decrescente (bubble sort - pequeno, serve bem)
 *   6. Salva o arquivo atualizado
 */
void insert_ranking(const char *name, int sc) {
    int count = 0;
    load_ranking(&count);   /* Le estado atual do disco */

    if (count < MAX_RANKING) {
        /* Ha espaco: adiciona nova entrada ao final */
        strncpy(ranking[count].name, name, 31);
        ranking[count].name[31] = '\0';   /* Garante null terminator */
        ranking[count].score    = sc;
        count++;

    } else if (sc > ranking[count - 1].score) {
        /* Sem espaco mas novo score e maior que o pior: substitui o ultimo */
        strncpy(ranking[count - 1].name, name, 31);
        ranking[count - 1].name[31] = '\0';
        ranking[count - 1].score    = sc;

    } else {
        return;   /* Score nao qualifica: nao altera o ranking */
    }

    /* Ordenacao por score decrescente (bubble sort):
     * O(n^2) mas n <= MAX_RANKING = 5, entao e negligenciavel */
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (ranking[j].score > ranking[i].score) {
                /* Troca as entradas */
                RankEntry t  = ranking[i];
                ranking[i]   = ranking[j];
                ranking[j]   = t;
            }
        }
    }

    /* Limita ao maximo (caso count tenha ultrapassado por algum bug) */
    if (count > MAX_RANKING) count = MAX_RANKING;

    save_ranking(count);   /* Persiste no arquivo */
}
