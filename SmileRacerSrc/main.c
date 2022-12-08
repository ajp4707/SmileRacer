/**
 * main.c
 * @author:
 * uP2 - Fall 2022
 */

// Include all headers you might need
#include "G8RTOS.h"
#include "threads.h"
#include "driverlib/interrupt.h"
#include "driverlib/watchdog.h"
#include "inc/hw_memmap.h"
#include "inc/tm4c123gh6pm.h"
#include "driverlib/sysctl.h"
#include "BoardInitialization.h"
#include "ILI9341_Lib.h"

void main(void)
{
    IntMasterDisable();
    G8RTOS_Init();
    InitializeBoard();
    LCD_Init(false);
    LCD_Clear(LCD_BLACK);

    seedRandom();

    G8RTOS_InitSemaphore(&LCD_mutex, 1);
    G8RTOS_InitSemaphore(&tap_flag, 0);

    G8RTOS_AddThread(background0, 255, "t0");
    G8RTOS_AddThread(game_over, 250, "t2"); // high priority
    G8RTOS_AddThread(wait_for_tap, 249, "t2"); // high priority

    //G8RTOS_InitFIFO(0);     // Fifo controller input. Used for debugging.

    G8RTOS_Launch();

    while(1);
}
