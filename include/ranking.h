/*
 * ranking.h
 * ---------
 * Interface do sistema de ranking persistente.
 *
 * O ranking e salvo em arquivo texto (RANKING_FILE) com o formato:
 *   Nome;Pontuacao\n
 *
 * Maximo de MAX_RANKING entradas, ordenadas por pontuacao decrescente.
 * A persistencia permite que o recorde sobreviva entre sessoes.
 */

#ifndef RANKING_H
#define RANKING_H

#include "../include/constants.h"  /* RankEntry, MAX_RANKING, RANKING_FILE */

/* Array global do ranking (lido do arquivo) */
extern RankEntry ranking[MAX_RANKING];

/*
 * load_ranking: le o arquivo RANKING_FILE e preenche ranking[].
 * Define *count com o numero de entradas lidas.
 * Se o arquivo nao existir, *count = 0 e nao ha erro.
 */
void load_ranking(int *count);

/*
 * save_ranking: persiste as 'count' entradas de ranking[] no arquivo.
 * Sobrescreve o arquivo anterior (nao e append).
 */
void save_ranking(int count);

/*
 * insert_ranking: insere uma nova entrada (name, sc) no ranking.
 * So insere se:
 *   - Ainda ha espaco (count < MAX_RANKING), OU
 *   - A nova pontuacao e maior que a ultima do ranking atual
 * Reordena por pontuacao decrescente apos a insercao e salva o arquivo.
 */
void insert_ranking(const char *name, int sc);

/*
 * best_score: retorna a maior pontuacao do ranking (ranking[0].score),
 * ou 0 se o ranking estiver vazio.
 * Chamado frequentemente para exibir o recorde no painel lateral.
 */
int best_score(void);

#endif /* RANKING_H */
