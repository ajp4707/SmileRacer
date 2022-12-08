/**
 * thread.h
 * @author:
 * uP2 - Fall 2022
 */

#ifndef THREADS_H_
#define THREADS_H_
#include "G8RTOS.h"

semaphore_t tap_flag;
semaphore_t sensor_mutex;
semaphore_t LCD_mutex;
semaphore_t modify_ball_count_mutex;

semaphore_t ball_ready;
semaphore_t game_over_sem;

semaphore_t all_threads_dead;

void seedRandom(void);


void background0(void);
void wall_thread(void);
void P_read_joys(void);
void print_score(void);
void ball_thread(void);
void star_thread(void);
void wait_for_tap(void);
void SwitchDebounce(void);
void game_over(void);
void wall_generator(void);


/* helpers */
void initLanes(void);
void drawLanes(void);


// define semaphore

// Struct object for a 'ball' thread 

// Define array of ball threads

// define your functions

#endif /* THREADS_H_ */
