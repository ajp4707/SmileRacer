/**
 * G8RTOS_Structure.h
 * uP2 - Fall 2022
 */

#ifndef G8RTOS_STRUCTURES_H_
#define G8RTOS_STRUCTURES_H_

#include "G8RTOS_Semaphores.h"
#include <stdbool.h>

#define UNBLOCKED   0

/*********************************************** Data Structure Definitions ***********************************************************/

/*
 *  Thread Control Block:
 *      - Every thread has a Thread Control Block
 *      - The Thread Control Block holds information about the Thread Such as the Stack Pointer, Priority Level, and Blocked Status
 *      - For Lab 2 the TCB will only hold the Stack Pointer, next TCB and the previous TCB (for Round Robin Scheduling)
 */

#define MAX_NAME_LENGTH 16

typedef int32_t threadId_t;

/* Create tcb struct here */

typedef struct tcb_t {          //TBC structure declaration
    int32_t *stackPointer;
    struct tcb_t *nextTCB;
    struct tcb_t *previousTCB;
    semaphore_t *blocked;
    uint32_t sleepCount;
    bool asleep;
    uint8_t priority;
    bool isAlive;
    char Threadname[MAX_NAME_LENGTH];
    threadId_t ThreadID;
} tcb_t;

/*
 *  Periodic Thread Control Block:
 *      - Holds a function pointer that points to the periodic thread to be executed
 *      - Has a period in us
 *      - Holds Current time
 *      - Contains pointer to the next periodic event - linked list
 */

/* Create periodic thread struct here */
typedef struct ptcb_t {         //TCBP structure
    void (*handler)(void);
    uint32_t period;
    uint32_t executeTime;
    uint32_t currentTime;
    struct ptcb_t *previousPTCB;
    struct ptcb_t *nextPTCB;
} ptcb_t;

/*********************************************** Data Structure Definitions ***********************************************************/


/*********************************************** Public Variables *********************************************************************/

tcb_t * CurrentlyRunningThread;

/*********************************************** Public Variables *********************************************************************/




#endif /* G8RTOS_STRUCTURES_H_ */
