/*
 * effects.h
 * ---------
 * Interface dos efeitos visuais especiais do jogo.
 *
 * Efeitos sao animacoes bloqueantes (usam Sleep internamente)
 * que ocorrem em eventos especificos: comer fruta, subir nivel, morrer.
 * Por serem bloqueantes, o jogo "pausa" brevemente durante eles,
 * o que e intencional para dar feedback dramatico ao jogador.
 */

#ifndef EFFECTS_H
#define EFFECTS_H

/*
 * flash_eat: pisca a borda da arena ao comer uma fruta.
 *
 * O numero de piscadas e a cor dependem da raridade da fruta:
 *   rarity=0 (comum): 1 piscada, verde
 *   rarity=1 (raro):  2 piscadas, amarelo
 *   rarity=2 (epico): 3 piscadas, ciano
 *
 * Duracao total: flashes * 50ms
 */
void flash_eat(int rarity);

/*
 * anim_death: animacao de game over.
 * Pisca a borda em vermelho e exibe "GAME OVER" no centro da arena 4 vezes.
 * Duracao total: ~1200ms (4 x 150ms por ciclo de piscada/texto)
 */
void anim_death(void);

/*
 * anim_level_up: animacao de subida de nivel.
 * Exibe "LEVEL N!" piscando no centro da arena 3 vezes.
 * Restaura os elementos da linha central apos a animacao.
 * Duracao total: ~600ms (3 x 200ms por ciclo)
 */
void anim_level_up(void);

#endif /* EFFECTS_H */
