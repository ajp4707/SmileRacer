/**
 * G8RTOS_Semaphores.c
 * uP2 - Fall 2022
 */

/*********************************************** Dependencies and Externs *************************************************************/

#include <stdint.h>
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "inc/hw_nvic.h"
#include "G8RTOS_Semaphores.h"
#include "G8RTOS_CriticalSection.h"
#include "G8RTOS_Scheduler.h"


/*********************************************** Dependencies and Externs *************************************************************/


/*********************************************** Public Functions *********************************************************************/

/*
 * Initializes a semaphore to a given value
 * Param "s": Pointer to semaphore
 * Param "value": Value to initialize semaphore to
 * THIS IS A CRITICAL SECTION
 */
void G8RTOS_InitSemaphore(semaphore_t *s, int32_t value)
{
    IBit_State = StartCriticalSection();
    *(s) = value;
    EndCriticalSection(IBit_State);
}

/*
 * No longer waits for semaphore
 *  - Decrements semaphore
 *  - Blocks thread if sempahore is unavailable
 * Param "s": Pointer to semaphore to wait on
 * THIS IS A CRITICAL SECTION
 */
void G8RTOS_WaitSemaphore(semaphore_t *s)
{
    // Strategy: begin and end critical section when dealing with semaphores
    // Turn off interrupts when dealing with I2C. This is a separate issue.
    IBit_State = StartCriticalSection();
    // Try to claim the semaphore.
    *(s) -= 1;
    if (*(s) >= 0) // successfully claimed!
    {
        EndCriticalSection(IBit_State);
        return;
    }
    else  // semaphore is negative
    {
        //currently running thread gets blocked.
        CurrentlyRunningThread->blocked = s;
        // Yield the CPU
        EndCriticalSection(IBit_State);
        //trigger scheduler switch
        HWREG(NVIC_INT_CTRL) |= NVIC_INT_CTRL_PEND_SV;
        return;
    }
}

/*
 * Signals the completion of the usage of a semaphore
 *  - Increments the semaphore value by 1
 *  - Unblocks any threads waiting on that semaphore
 * Param "s": Pointer to semaphore to be signaled
 * THIS IS A CRITICAL SECTION
 */
void G8RTOS_SignalSemaphore(semaphore_t *s)
{
    IBit_State = StartCriticalSection();
    // give back the semaphore
    *(s) += 1;
    // unblock one semaphore
    tcb_t* thr = CurrentlyRunningThread->nextTCB;
    // find a thread that is waiting on this semaphore
    while(thr != CurrentlyRunningThread)
    {
        if (thr->blocked == s)
        {
            thr->blocked = UNBLOCKED;
            break; // only do it once!
        }
        thr = thr->nextTCB;
    }
    EndCriticalSection(IBit_State);
}

void G8RTOS_Decrement(semaphore_t *s)
{
    IBit_State = StartCriticalSection();
    // give back the semaphore
    *(s) += 1;
    EndCriticalSection(IBit_State);
}
/*********************************************** Public Functions *********************************************************************/


