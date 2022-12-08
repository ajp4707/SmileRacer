/**
 * thread.c
 * @author: Aidan Persaud
 * uP2 - Fall 2022
 */
#include "threads.h"
#include "G8RTOS.h"
#include "BoardSupport/inc/demo_sysctl.h"
#include "BoardSupport/inc/bmi160_support.h"
#include "BoardSupport/inc/opt3001.h"
#include "BoardSupport/inc/bme280.h"
#include "BoardSupport/inc/Joystick.h"
#include "driverlib/sysctl.h"
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"

#include "inc/tm4c123gh6pm.h"
#include "ILI9341_Lib.h"
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "driverlib/timer.h"
#include "driverlib/uart.h"

#define DEBOUNCE_S          2/10
#define UPDATE_S            8/10
#define BUTTON1_MASK        0x10
#define SLEEP_TICKS         30

#define FIFO_INPUT          0


/**** UART Messages ****/
#define SMILE           0x73
#define FACE            0x55
#define NOTHING         0x11

#define NUM_LANES       5  // max is 7

#define LANE_WIDTH      MAX_SCREEN_Y / NUM_LANES


/******* STRUCTS **************************************/
typedef struct Lane_t {
    uint32_t color;
    uint16_t top;
} Lane_t;

struct Ball {
    uint8_t  lane;
    int32_t xpos;
    int32_t ypos;
    uint16_t ID;
    uint16_t width;
    uint16_t color;
    uint16_t velocity;
} Ball;


struct Ball game_ball;

static uint32_t lane_colors[] = {LCD_RED, LCD_ORANGE, LCD_YELLOW, LCD_GREEN, LCD_BLUE, LCD_PURPLE, LCD_PINK};
static Lane_t Lanes[NUM_LANES];

static uint16_t num_temp_thrds = 0;     // number of temporary threads, ie. ball, wall, star, wall_generator

volatile bool kill_thrds;               // Flag to start killing temporary threads
volatile bool restart = false;          // Flag to reboot the game cycle
volatile bool update_ready = true;      // Flag to update the game_ball's onscreen position
volatile bool new_buffer = false;       // Flag when new UART message received


static uint16_t score;
static bool score_flag;                 // Flag when score has been updated

static uint8_t movement = 0;            // The number of lanes to move on next game_ball update
static uint8_t move_buffer;             // Buffer that stores last UART transmission (smile, face, or nothing)

static int32_t coords[2];               // Used for joystick input. Debug only

/* Helper functions ***********************************/
void seedRandom(void)
{
    srand(time(NULL));
}

void initLanes(void)
{
    for (int i = 0; i < NUM_LANES; i++)
    {
        Lanes[i].color = lane_colors[i];
        Lanes[i].top = i * LANE_WIDTH;
    }
}

void drawLanes(void)
{
    // make sure you call LCD Mutex before calling this.
    for (int i = 0; i < NUM_LANES; i++)
    {
        LCD_DrawRectangle(0, Lanes[i].top, MAX_SCREEN_X, LANE_WIDTH + i, lane_colors[i]);
    }
}

void clearLanes(uint32_t color)
{
    for (int i = 0; i < NUM_LANES; i++)
    {
        LCD_DrawRectangle(0, Lanes[i].top, MAX_SCREEN_X, LANE_WIDTH + i, color);
    }
}

uint16_t get_ball_ypos(uint8_t lane, uint16_t ball_width)
{
    return Lanes[lane].top + LANE_WIDTH/2 - ball_width/2;
}

/*
 * Function: UpdateGameBall
 * ----------------------------
 *   Modifies the size and position of the primary game_ball.
 *   Size change occurs at every UART transmission from Beagle.
 *   - Small ball means NO FACE detected
 *   - Medium ball means NO SMILE detected
 *   - Large ball means SMILE detected
 *
 *   Position change occurs only when smile is received AND
 *   0.8 seconds after last position change.
 *   The game_ball may only move UPWARDS onscreen.
 */
void UpdateGameBall(void)
{
    if (new_buffer == false)
        return;
    new_buffer = false;
    /* For use with joysticks
    int32_t movement = getMovement(FIFO_INPUT);

    if (movement != 0)
    {
        // erase ball
        G8RTOS_WaitSemaphore(&LCD_mutex);
        LCD_DrawRectangle(game_ball.xpos, game_ball.ypos, game_ball.width, game_ball.width, Lanes[game_ball.lane].color);
        G8RTOS_SignalSemaphore(&LCD_mutex);

        game_ball.lane = (NUM_LANES + game_ball.lane + movement) % NUM_LANES;
        game_ball.ypos = get_ball_ypos(game_ball.lane, game_ball.width);

        // plot ball
        G8RTOS_WaitSemaphore(&LCD_mutex);
        LCD_DrawRectangle(game_ball.xpos, game_ball.ypos, game_ball.width, game_ball.width, game_ball.color);
        G8RTOS_SignalSemaphore(&LCD_mutex);

        update_ready = false;
        TimerLoadSet(TIMER1_BASE, TIMER_A, SysCtlClockGet() * UPDATE_S);
        TimerEnable(TIMER1_BASE, TIMER_A);
    }
    */

    // erase ball
    G8RTOS_WaitSemaphore(&LCD_mutex);
    LCD_DrawRectangle(game_ball.xpos, game_ball.ypos, game_ball.width, game_ball.width, Lanes[game_ball.lane].color);
    G8RTOS_SignalSemaphore(&LCD_mutex);

    if (move_buffer == SMILE)
    {
        game_ball.width = 10;
        movement = 1;
    }
    else if (move_buffer == FACE)
    {
        game_ball.width = 7;
        movement = 0;
    }
    else
    {
        game_ball.width = 4;
    }

    if (update_ready == true)
    {
        game_ball.lane = (NUM_LANES + game_ball.lane - movement) % NUM_LANES; //move up not down
        update_ready = false;
        TimerLoadSet(TIMER1_BASE, TIMER_A, SysCtlClockGet() * UPDATE_S);
        TimerEnable(TIMER1_BASE, TIMER_A);
    }

    game_ball.ypos = get_ball_ypos(game_ball.lane, game_ball.width);

    // plot ball
    G8RTOS_WaitSemaphore(&LCD_mutex);
    LCD_DrawRectangle(game_ball.xpos, game_ball.ypos, game_ball.width, game_ball.width, game_ball.color);
    G8RTOS_SignalSemaphore(&LCD_mutex);
}

/*
 * Function: UpdateWall
 * ----------------------------
 *   Moves a wall to the left by pixels = its velocity
 */
void UpdateWall(struct Ball* ball_ptr)
{
    ball_ptr->xpos = ball_ptr->xpos - ball_ptr->velocity;
}

/*
 * Function: start_temp_thread
 * ----------------------------
 *   Increments the temporary thread count. Must be called
 *   at the beginning of each temporary thread for proper
 *   game function.
 *
 *   Temporary threads include: Game_ball thread, the star,
 *   the wall_generator, and all walls
 */
void start_temp_thread(void)
{
    num_temp_thrds++;
}

/*
 * Function: kill_temp_thread
 * ----------------------------
 *   Decrements the temporary thread count and kills
 *   the current thread.
 */
void kill_temp_thread(void)
{
    num_temp_thrds--;
    G8RTOS_KillSelf();
}

void up_score(void)
{
    score++;
    score_flag = true;
}

void print_score(void)
{
    start_temp_thread();

    char str[12];
    while(1)
    {
        if(kill_thrds)
            kill_temp_thread();

    if (score_flag)
    {
        score_flag = false;
        sprintf(str, "Score: %d", score);
        G8RTOS_WaitSemaphore(&LCD_mutex);
        LCD_DrawRectangle(3, 3, 100, 15, Lanes[0].color);
        LCD_Text(3, 3, (uint8_t*)str, LCD_WHITE);
        G8RTOS_SignalSemaphore(&LCD_mutex);
    }

    sleep(200);
    }
}

/********* THREADS *************************/

/*
 * Thread: background0
 * ----------------------------
 *   Background thread just in case.
 */
void background0(void)
{
    while(1);
}

/*
 * Thread: game_over
 * ----------------------------
 *   Main game controlling thread.
 *
 *   Initializes game_ball, star and wall_generator threads.
 *   Lets the game run and waits for the game_over_sem semaphore
 *   to be triggered by a wall.
 *
 *   After the end condition is met, this thread waits for all
 *   temporary threads (balls, star, walls) to kill themselves
 *   moving on.
 *
 *   Displays "game over" splash screen and final score.
 */
void game_over(void)
{
    seedRandom();
    initLanes();

    while(1)
    {
        score = 0;
        score_flag = true;
        kill_thrds = false;
        G8RTOS_InitSemaphore(&game_over_sem, 0);
        G8RTOS_InitSemaphore(&ball_ready, 0);

        // Pregame clear screen
        G8RTOS_WaitSemaphore(&LCD_mutex);
        drawLanes();
        G8RTOS_SignalSemaphore(&LCD_mutex);

        //score thread
        G8RTOS_AddThread(print_score, 254, "ball");

        // ball thread
        G8RTOS_AddThread(ball_thread, 251, "ball");
        G8RTOS_WaitSemaphore(&ball_ready);

        // star thread
        G8RTOS_AddThread(star_thread, 251, "star");

        // Add wall generator thread
        G8RTOS_AddThread(wall_generator, 250, "wall_gen");
        // code here: walls are responsible for triggering game_over_sem

        // wait for game over
        G8RTOS_WaitSemaphore(&game_over_sem);
        kill_thrds = true;

        // wait for all temp threads to die
        while (num_temp_thrds > 0)
            sleep(200);

        G8RTOS_WaitSemaphore(&LCD_mutex);
        clearLanes(LCD_RED);
        LCD_Text(120, 100, "Game Over!", LCD_WHITE);
        char str[18];
        sprintf(str, "Final score: %d", score);
        LCD_Text(105, 120, (uint8_t*)str, LCD_WHITE);
        G8RTOS_SignalSemaphore(&LCD_mutex);

        restart = true;

        // wait on interrupt
        while(restart);
    }
}

/*
 * Thread: ball_thread
 * ----------------------------
 *   Controls the game_ball (the character!)
 *
 *   Creates the ball object. Plots ball onscreen.
 *   Responsible for killing itself.
 */
void ball_thread(void)
{
    start_temp_thread();

    game_ball.width = 7;
    game_ball.lane = NUM_LANES/2;
    game_ball.xpos = (MAX_SCREEN_X - game_ball.width)/2;
    game_ball.ypos = get_ball_ypos(game_ball.lane, game_ball.width);
    game_ball.color = LCD_WHITE;

    // plot ball
    G8RTOS_WaitSemaphore(&LCD_mutex);
    LCD_DrawRectangle(game_ball.xpos, game_ball.ypos, game_ball.width, game_ball.width, game_ball.color);
    G8RTOS_SignalSemaphore(&LCD_mutex);

    G8RTOS_SignalSemaphore(&ball_ready);

    while(1)
    {
        if (kill_thrds)
            kill_temp_thread();

        UpdateGameBall();
        sleep(SLEEP_TICKS);
    }
}

/*
 * Thread: star_thread
 * ----------------------------
 *   Controls the purple square (the star!)
 *
 *   Initializes and plots the star. Checks collision with the
 *   game ball. If collision, increment score and randomly relocate
 *   star to another lane.
 *   Responsible for killing itself.
 */
void star_thread(void)
{
    start_temp_thread();

    struct Ball star;

    star.color = LCD_PURPLE;
    star.width = 10;
    star.lane = (game_ball.lane - 2) % NUM_LANES;
    star.ypos = get_ball_ypos(star.lane, star.width);
    star.xpos = MAX_SCREEN_X/2 - star.width/2;

    while(1)
    {
        if (kill_thrds)
            kill_temp_thread();
        // check collision
        if(game_ball.lane == star.lane)
        {
            up_score();

            //erase star, redraw ball
            G8RTOS_WaitSemaphore(&LCD_mutex);
            LCD_DrawRectangle(star.xpos, star.ypos, star.width, star.width, Lanes[star.lane].color);
            LCD_DrawRectangle(game_ball.xpos, game_ball.ypos, game_ball.width, game_ball.width, game_ball.color);
            G8RTOS_SignalSemaphore(&LCD_mutex);

            // generate a new lane number that is DIFFERENT than game_ball's lane
            do {
            star.lane = rand() % NUM_LANES;
            }
            while(star.lane == game_ball.lane);
            star.ypos = get_ball_ypos(star.lane, star.width);
        }

        // plot star
        G8RTOS_WaitSemaphore(&LCD_mutex);
        LCD_DrawRectangle(star.xpos, star.ypos, star.width, star.width, star.color);
        G8RTOS_SignalSemaphore(&LCD_mutex);
        sleep(SLEEP_TICKS);
    }
}

/*
 * Thread: wall_generator
 * ----------------------------
 *   Generates new wall threads at a semi-random interval (1-2 seconds).
 *   Responsible for killing itself.
 */
void wall_generator(void)
{
    start_temp_thread();
    uint32_t sleepcount;
    while(1)
    {
        if (kill_thrds)
            kill_temp_thread();


        G8RTOS_AddThread(wall_thread, 252, "wall");
        sleepcount = 1000 + rand() % 1000;
        sleep(sleepcount);
    }
}

/*
 * Thread: wall_thread
 * ----------------------------
 *   Generates a new wall object in a random lane.
 *   Plots wall object onscreen.
 *
 *   Kills itself when reaches the left side of screen.
 *   Responsible for killing itself.
 */
void wall_thread(void)
{
    start_temp_thread();

    // Init walls
    struct Ball wall;

    wall.color = LCD_WHITE;
    wall.velocity = 2;
    wall.width = 10;
    wall.lane = rand() % NUM_LANES;
    wall.xpos = MAX_SCREEN_X - wall.width;
    wall.ypos = get_ball_ypos(wall.lane, wall.width);

    while(1)
    {
        if (kill_thrds)
            kill_temp_thread();


        // plot ball
        G8RTOS_WaitSemaphore(&LCD_mutex);
        LCD_DrawRectangle(wall.xpos, wall.ypos, wall.width, wall.width, wall.color);
        G8RTOS_SignalSemaphore(&LCD_mutex);

        // check collision
        if (game_ball.xpos + (game_ball.width-1) >= wall.xpos  &&
            game_ball.xpos <= wall.xpos + (wall.width-1) &&
            game_ball.lane == wall.lane)
        {
            // game over!
            G8RTOS_SignalSemaphore(&game_over_sem);
            kill_temp_thread();
        }

        sleep(SLEEP_TICKS);

        // erase ball
        G8RTOS_WaitSemaphore(&LCD_mutex);
        LCD_DrawRectangle(wall.xpos, wall.ypos, wall.width, wall.width, Lanes[wall.lane].color);
        G8RTOS_SignalSemaphore(&LCD_mutex);

        UpdateWall(&wall);

        if (wall.xpos < 0)
        {
            kill_temp_thread();
        }
    }
}

/*
 * Aperiodic thread: UART_int_handler
 * ----------------------------
 *   Handles UART transmission from beaglebone and
 *   moves data into buffer for later.
 *
 *   Sets the new_buffer flag to signal to the
 *   game_ball thread.
 */
void UART_int_handler(void)
{
    int32_t val = 0;

    uint32_t ui32Status;
    ui32Status = UARTIntStatus(UART1_BASE, true);
    UARTIntClear(UART1_BASE, ui32Status);

    // Loop while there are characters in the receive FIFO.
    while(UARTCharsAvail(UART1_BASE))
    {
          val = UARTCharGetNonBlocking(UART1_BASE);
    }

    move_buffer = (uint8_t)(val & 0xFF);
    new_buffer = true;
}

/*
 * Thread: wait_for_tap
 * ----------------------------
 *   Clears the "restart" flag, which allows the game_over
 *   thread to restart the game.
 *
 *   A little silly to have a separate function for this.
 *   Remnant from lab 4.
 */
void wait_for_tap(void)
{
    while(1)
    {
        G8RTOS_WaitSemaphore(&tap_flag);
        restart = false;
        sleep(200);
    }
}

/*
 * Aperiodic thread: SwitchDebounce
 * Interrupt handler for Timer0A.
 * ----------------------------
 *  Software debouncing of switch 2 input.
 *  If the button is still pressed, unblock wait_for_tap,
 *  which unblocks game_over thread.
 */
void SwitchDebounce(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    // verify Port F pin 4 is still low
    if(!(GPIO_PORTF_DATA_R & BUTTON1_MASK))
        G8RTOS_SignalSemaphore(&tap_flag);
}

/*
 * Aperiodic thread: LCDtap
 * Interrupt handler for PORTF GPIO.
 * ----------------------------
 *  Really should be called SWITCHtap.
 *  Triggered when the restart button (switch 2) is pressed.
 *  Loads Timer0A so that we can debounce the button press (with SwitchDebounce)
 */
void LCDtap(void)
{
    GPIOIntClear(GPIO_PORTF_BASE, GPIO_INT_PIN_4);

    // Defer to timer to debounce the button
    // Load timer and run it
    TimerLoadSet(TIMER0_BASE, TIMER_A, SysCtlClockGet() * DEBOUNCE_S);
    TimerEnable(TIMER0_BASE, TIMER_A);
}

/*
 * Aperiodic thread: UpdateDebounce
 * Interrupt handler for Timer1A.
 * ----------------------------
 *  Timer 1A is set in UpdateGameBall.
 *  This callback is triggered after 0.8 seconds.
 *
 *  Sets flag to allow the game_ball to be updated once again
 */
void UpdateDebounce(void)
{
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    update_ready = true;
}

/*
 * Periodic thread: P_read_joys
 * ----------------------------
 *  Reads joystick values into coords buffer.
 *
 *  Used for debug only.
 */
void P_read_joys(void)
{
     GetJoystickCoordinates(coords);
     if ((coords[1] - 2048)/500 >= 1) // positive movement
         writeFIFO(FIFO_INPUT, 1);
     else if ((coords[1] - 2048)/500 <= -1)
         writeFIFO(FIFO_INPUT, -1);
     else
         writeFIFO(FIFO_INPUT, 0);
}
