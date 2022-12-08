/**
 * thread.h
 * @author: Aidan Persaud
 * uP2 - Fall 2022
 */

#ifndef THREADS_H_
#define THREADS_H_
#include "G8RTOS.h"

semaphore_t tap_flag;
semaphore_t LCD_mutex;

semaphore_t ball_ready;
semaphore_t game_over_sem;

void background0(void);
void game_over(void);
void ball_thread(void);
void star_thread(void);
void wall_generator(void);
void wall_thread(void);
void print_score(void);

void wait_for_tap(void);
void SwitchDebounce(void);

/* helpers */
void seedRandom(void);
void initLanes(void);
void drawLanes(void);
void P_read_joys(void);

#endif /* THREADS_H_ */
