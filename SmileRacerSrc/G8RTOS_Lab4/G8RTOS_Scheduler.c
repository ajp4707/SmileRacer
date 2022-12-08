/**
 * G8RTOS_Scheduler.c
 * uP2 - Fall 2022
 */
#include <stdint.h>
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "inc/hw_nvic.h"
#include "driverlib/systick.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "BoardSupport/inc/RGBLedDriver.h"
#include "G8RTOS_Scheduler.h"
#include "G8RTOS_Structures.h"
#include "G8RTOS_CriticalSection.h"

/*
 * G8RTOS_Start exists in asm
 */
extern void G8RTOS_Start();

/* System Core Clock From system_msp432p401r.c */
extern uint32_t SystemCoreClock;

/*
 * Pointer to the currently running Thread Control Block
 */
//extern tcb_t * CurrentlyRunningThread;

/*********************************************** Dependencies and Externs *************************************************************/


/*********************************************** Defines ******************************************************************************/

/* Status Register with the Thumb-bit Set */
#define THUMBBIT 0x01000000

/*********************************************** Defines ******************************************************************************/


/*********************************************** Data Structures Used *****************************************************************/

/* Thread Control Blocks
 *  - An array of thread control blocks to hold pertinent information for each thread
 */
static tcb_t threadControlBlocks[MAX_THREADS];

/* Thread Stacks
 *  - An array of arrays that will act as individual stacks for each thread
 */
static int32_t threadStacks[MAX_THREADS][STACKSIZE];

/* Periodic Event Threads
 * - An array of periodic events to hold pertinent information for each thread
 */
static ptcb_t Pthread[MAXPTHREADS];

/*********************************************** Data Structures Used *****************************************************************/


/*********************************************** Private Variables ********************************************************************/

/*
 * Current Number of Threads currently in the scheduler
 */
static uint32_t NumberOfThreads;

/*
 * Current Number of Periodic Threads currently in the scheduler
 */
static uint32_t NumberOfPthreads;

/*********************************************** Private Variables ********************************************************************/


/*********************************************** Private Functions ********************************************************************/

/*
 * Initializes the Systick and Systick Interrupt
 * The Systick interrupt will be responsible for starting a context switch between threads
 * Param "numCycles": Number of cycles for each systick interrupt
 */
static void InitSysTick(uint32_t numCycles)
{
    SysTickPeriodSet(numCycles);
    SysTickEnable();
}

/*
 * Chooses the next thread to run.
 * Lab 2 Scheduling Algorithm:
 *  - Simple Round Robin: Choose the next running thread by selecting the currently running thread's next pointer
 *  - Check for sleeping and blocked threads
 */
void G8RTOS_Scheduler()
{
    currentMaxPriority = 256;                                               //Sets a max priority
    tempNextThread = CurrentlyRunningThread->nextTCB;                       //Sets a temp next thread
    for(uint8_t i = 0;i < NumberOfThreads;i++)                              //Interates through the threads
    {
        if(tempNextThread->asleep == 0 && tempNextThread->blocked == 0)     //Checks if alive or blocked
        {
            if(tempNextThread->priority < currentMaxPriority)               //Checks priority
            {
                CurrentlyRunningThread = tempNextThread;                    //Assigns max priority
                currentMaxPriority = CurrentlyRunningThread->priority;      //New max priority
            }
        }
        tempNextThread = tempNextThread->nextTCB;
    }
}


/*
 * SysTick Handler
 * The Systick Handler now will increment the system time,
 * set the PendSV flag to start the scheduler,
 * and be responsible for handling sleeping and periodic threads
 */
void SysTick_Handler()
{
    SystemTime++;
    tcb_t *ptr = CurrentlyRunningThread;

    ptcb_t *Pptr = &Pthread[0]; //Checks for periodic threads, and executes them appropriately
    for(uint8_t i = 0;i < NumberOfPthreads;i++)
    {
        if(Pptr->executeTime == SystemTime)
        {
            Pptr->executeTime = Pptr->period + SystemTime;
            Pptr->handler();
        }
        Pptr = Pptr->nextPTCB;
    }

    //Checks sleeping threads and wakes them up appropriately
    for(uint8_t i = 0;i < NumberOfThreads;i++)
    {
        if(ptr->asleep == 1)
        {
            if(ptr->sleepCount == SystemTime)
            {
                ptr->asleep = 0;
                ptr->sleepCount = 0;
            }
        }
        ptr = ptr->nextTCB;
    }

    HWREG(NVIC_INT_CTRL) |= NVIC_INT_CTRL_PEND_SV;
}

/*********************************************** Private Functions ********************************************************************/


/*********************************************** Public Variables *********************************************************************/

/* Holds the current time for the whole System */
uint32_t SystemTime;

/*********************************************** Public Variables *********************************************************************/


/*********************************************** Public Functions *********************************************************************/

/*
 * Sets variables to an initial state (system time and number of threads)
 * Enables board for highest speed clock and disables watchdog
 */
void G8RTOS_Init()
{
    SystemTime = 0;
    NumberOfThreads = 0;
    NumberOfPthreads = 0;
    uint32_t newVTORTable = 0x20000000;

    uint32_t * newTable = (uint32_t *)newVTORTable;
    uint32_t * oldTable = (uint32_t *)0;

    // In place of "memcpy"
    for (int i = 0; i < 155; i++)
    {
        newTable[i] = oldTable[i];
    }

    HWREG(NVIC_VTABLE) = newVTORTable;
}

/*
 * Starts G8RTOS Scheduler
 *  - Initializes the Systick
 *  - Sets Context to first thread
 * Returns: Error Code for starting scheduler. This will only return if the scheduler fails
 */
int G8RTOS_Launch()
{
    CurrentlyRunningThread = &threadControlBlocks[0];

    //Sets the thread with the max priority as the first thread to run
    for(uint8_t i = 1;i < NumberOfThreads;i++)
    {
        if(threadControlBlocks[i].priority < CurrentlyRunningThread->priority)
        {
            CurrentlyRunningThread = &threadControlBlocks[i];
        }
    }

    InitSysTick(SysCtlClockGet() / 1000); // 1 ms tick (1Hz / 1000)
    IntPrioritySet(FAULT_PENDSV, 0xE0);
    IntPrioritySet(FAULT_SYSTICK, 0xE0);
    SysTickIntEnable();
    IntMasterEnable();

    G8RTOS_Start();
    return 0;
}


/*
 * Adds threads to G8RTOS Scheduler
 *  - Checks if there are stil available threads to insert to scheduler
 *  - Initializes the thread control block for the provided thread
 *  - Initializes the stack for the provided thread to hold a "fake context"
 *  - Sets stack tcb stack pointer to top of thread stack
 *  - Sets up the next and previous tcb pointers in a round robin fashion
 * Param "threadToAdd": Void-Void Function to add as preemptable main thread
 * Returns: Error code for adding threads
 */
sched_ErrCode_t G8RTOS_AddThread(void (*threadToAdd)(void), uint8_t priority, char *name)
{
    IBit_State = StartCriticalSection();

    //Maximum amount of threads
    if(NumberOfThreads >= MAX_THREADS)
    {
        EndCriticalSection(IBit_State);
        return THREAD_LIMIT_REACHED;   //Returns -1 if max threads reached
    }
    else
    {
        uint8_t newThreadIndex = 0;

        if(NumberOfThreads == 0)
        {
            threadControlBlocks[0].nextTCB = &threadControlBlocks[0];     //Sets the first thread
            threadControlBlocks[0].previousTCB = &threadControlBlocks[0];
            CurrentlyRunningThread = &threadControlBlocks[0];
        }

        else
        {
            //New thread fills the first empty block in the linked list
            for(uint8_t i = 0;i < MAX_THREADS;i++)
            {
                if(threadControlBlocks[i].isAlive == 0)
                {
                    newThreadIndex = i;
                    break;
                }
            }

            CurrentlyRunningThread->nextTCB->previousTCB = &threadControlBlocks[newThreadIndex];
            threadControlBlocks[newThreadIndex].nextTCB =  CurrentlyRunningThread->nextTCB;
            CurrentlyRunningThread->nextTCB = &threadControlBlocks[newThreadIndex]; //set next tcb
            threadControlBlocks[newThreadIndex].previousTCB = CurrentlyRunningThread; //set previous tcb
        }

        //Assigns parameters to the threads
        threadControlBlocks[newThreadIndex].ThreadID = ((IDCounter++) << 16) | (uint32_t)(&threadControlBlocks[newThreadIndex]);
        uint8_t nameIndex = 0;
        while(nameIndex < MAX_NAME_LENGTH && name[nameIndex] != 0)
        {
            threadControlBlocks[newThreadIndex].Threadname[nameIndex] = name[nameIndex];
            nameIndex++;
        }
        threadControlBlocks[newThreadIndex].asleep = false;
        threadControlBlocks[newThreadIndex].priority = priority;
        threadControlBlocks[newThreadIndex].isAlive = 1;
        threadControlBlocks[newThreadIndex].stackPointer = &threadStacks[newThreadIndex][STACKSIZE-16];   //Sets the stack pointer to the thread
        threadStacks[newThreadIndex][STACKSIZE-1] = THUMBBIT;                //xPSR
        threadStacks[newThreadIndex][STACKSIZE-2] = (uint32_t)threadToAdd;   //PC
        NumberOfThreads++;  //Increases the thread count
    }
    EndCriticalSection(IBit_State);
    return NO_ERROR;
}


/*
 * Adds periodic threads to G8RTOS Scheduler
 * Function will initialize a periodic event struct to represent event.
 * The struct will be added to a linked list of periodic events
 * Param Pthread To Add: void-void function for P thread handler
 * Param period: period of P thread to add
 * Returns: Error code for adding threads
 */
int G8RTOS_AddPeriodicEvent(void (*PthreadToAdd)(void), uint32_t period, uint32_t execution)
{
    IBit_State = StartCriticalSection();

    //Maximum amount of P threads
    if(NumberOfPthreads > MAXPTHREADS)
    {
        EndCriticalSection(IBit_State);
        return -1;  //Return -1 if at max
    }
    else
    {
        if(NumberOfPthreads == 0)                                       //Sets the first thread
        {
            Pthread[NumberOfPthreads].nextPTCB = &Pthread[0];
            Pthread[NumberOfPthreads].previousPTCB = &Pthread[0];
        }
        else                                                            //Inserts the new thread into the linked list
        {
            Pthread[NumberOfPthreads].nextPTCB = &Pthread[0];
            Pthread[NumberOfPthreads-1].nextPTCB = &Pthread[NumberOfPthreads];
            Pthread[0].previousPTCB = &Pthread[NumberOfPthreads];
            Pthread[NumberOfPthreads].previousPTCB = &Pthread[NumberOfPthreads-1];
        }
        Pthread[NumberOfPthreads].period = period;          //Stores period
        Pthread[NumberOfPthreads].executeTime = execution;  //Stores execution
        Pthread[NumberOfPthreads].handler = PthreadToAdd;   //Stores handler
        NumberOfPthreads++; //Increases thread count
    }
    EndCriticalSection(IBit_State);
    return 1;
}

sched_ErrCode_t G8RTOS_AddAPeriodicEvent(void (*AthreadToAdd)(void), uint8_t priority, int32_t IRQn)
{
    IBit_State = StartCriticalSection();            //Disable interrupts
    if(IRQn < 0 || IRQn > 154)                      //Checks to see if priority is in range
    {
        EndCriticalSection(IBit_State);             //Enables interrupts
        return IRQn_INVALID;
    }
    if(priority > 6)                                //Checks if priority too low
    {
        EndCriticalSection(IBit_State);             //Enables interrupts
        return HWI_PRIORITY_INVALID;
    }

    // NVIC_SetVector(IRQn, (uint32_t)AthreadToAdd);   //Adds aperiodic thread

    // The code below is to emulate what the line above would be on the MSP432
    uint32_t *vectors = (uint32_t *)HWREG(NVIC_VTABLE);
    vectors[IRQn] = (uint32_t)AthreadToAdd;

    IntPrioritySet(IRQn, priority);
    IntEnable(IRQn);
    EndCriticalSection(IBit_State);
    return NO_ERROR;
}


/*
 * Puts the current thread into a sleep state.
 *  param durationMS: Duration of sleep time in ms
 */
void sleep(uint32_t durationMS)
{
    CurrentlyRunningThread->sleepCount = durationMS + SystemTime;   //Sets sleep count
    CurrentlyRunningThread->asleep = 1;                             //Puts the thread to sleep
    HWREG(NVIC_INT_CTRL) |= NVIC_INT_CTRL_PEND_SV;                  //Start context switch
}

threadId_t G8RTOS_GetThreadId()
{
    return CurrentlyRunningThread->ThreadID;        //Returns the thread ID
}

sched_ErrCode_t G8RTOS_KillThread(threadId_t threadID)
{
    IBit_State = StartCriticalSection();            //Disables interrupts
    tcb_t *tempThread = CurrentlyRunningThread;
    if(NumberOfThreads == 1)                        //Can't kill the last thread
    {
        EndCriticalSection(IBit_State);
        return CANNOT_KILL_LAST_THREAD;
    }
    for(uint8_t i = 0;i < NumberOfThreads;i++)      //Find the thread in the list of threads
    {
        if(tempThread->ThreadID == threadID)
        {
            NumberOfThreads--;                      //Decrements number of threads
            tempThread->isAlive = 0;                //Deletes thread parameters
            for(uint8_t i = 0;i < MAX_NAME_LENGTH;i++)
            {
                tempThread->Threadname[i] = 0;
            }
            tempThread->previousTCB->nextTCB = tempThread->nextTCB;     //Revises linked list
            tempThread->nextTCB->previousTCB = tempThread->previousTCB;
            if(tempThread == CurrentlyRunningThread)                    //If currently running thread, initiate context switch
            {
                EndCriticalSection(IBit_State);
                HWREG(NVIC_INT_CTRL) |= NVIC_INT_CTRL_PEND_SV;
            }
            else
            {
                EndCriticalSection(IBit_State);
                return NO_ERROR;
            }
        }

    }

    EndCriticalSection(IBit_State);
    return THREAD_DOES_NOT_EXIST;
}

//Thread kills itself
sched_ErrCode_t G8RTOS_KillSelf()
{
    IBit_State = StartCriticalSection();
    if(NumberOfThreads == 1)                //Can't kill the last thread
    {
        EndCriticalSection(IBit_State);
        return CANNOT_KILL_LAST_THREAD;
    }
    NumberOfThreads--;                      //Decrements number of threads
    CurrentlyRunningThread->isAlive = 0;    //Deletes thread parameters
    for(uint8_t i = 0;i < MAX_NAME_LENGTH;i++)
    {
        CurrentlyRunningThread->Threadname[i] = 0;
    }
    CurrentlyRunningThread->previousTCB->nextTCB = CurrentlyRunningThread->nextTCB; //Revises linked list
    CurrentlyRunningThread->nextTCB->previousTCB = CurrentlyRunningThread->previousTCB;
    EndCriticalSection(IBit_State);

    HWREG(NVIC_INT_CTRL) |= NVIC_INT_CTRL_PEND_SV;    //Initiates context switch
    while(1);
}

uint32_t GetNumberOfThreads(void)
{
    return NumberOfThreads;         //Returns the number of threads
}

void G8RTOS_KillAllThreads()
{
    IBit_State = StartCriticalSection();

    tcb_t * temp = CurrentlyRunningThread->nextTCB;

    do          //Kills all threads except for the currently running thread, which in this case is the EndOfGameHost
    {
        for(uint8_t i = 0;i < MAX_NAME_LENGTH;i++)
            temp->Threadname[i] = 0;

        temp->isAlive = false;
        NumberOfThreads--;
        temp = temp->nextTCB;
    } while(temp != CurrentlyRunningThread);

    CurrentlyRunningThread->nextTCB = CurrentlyRunningThread;
    CurrentlyRunningThread->previousTCB = CurrentlyRunningThread;

    EndCriticalSection(IBit_State);
}






/*********************************************** Public Functions *********************************************************************/
