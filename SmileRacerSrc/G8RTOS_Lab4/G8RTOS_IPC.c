/**
 * G8RTOS IPC - Inter-Process Communication
 * @author:
 * uP2 - Fall 2022
 */
#include <stdint.h>
#include "G8RTOS_IPC.h"
#include "G8RTOS_Semaphores.h"

#define FIFOSIZE 16
#define MAX_NUMBER_OF_FIFOS 4

/*
 * FIFO struct will hold
 *  - buffer
 *  - head
 *  - tail
 *  - lost data
 *  - current size
 *  - mutex
 */

/* Create FIFO struct here */

typedef struct FIFO_t {
    int32_t buffer[FIFOSIZE];
    int32_t *head;
    int32_t *tail;
    uint32_t lostData;
    semaphore_t currentSize;
    semaphore_t mutex;
} FIFO_t;


/* Array of FIFOS */
static FIFO_t FIFOs[MAX_NUMBER_OF_FIFOS];


/*
 * Initializes FIFO Struct
 */
int G8RTOS_InitFIFO(uint32_t FIFOIndex)
{
    if(FIFOIndex >= MAX_NUMBER_OF_FIFOS)
        return -1;
    FIFO_t* ffptr = &FIFOs[FIFOIndex];
    // Initialize buffer pointers
    ffptr->head = &(ffptr->buffer[0]);
    ffptr->tail = ffptr->head;

    ffptr->lostData=0;
    ffptr->mutex = 1;
    ffptr->currentSize = 0;

    return 0;
}

/*
 * Reads FIFO
 *  - Waits until CurrentSize semaphore is greater than zero
 *  - Gets data and increments the head ptr (wraps if necessary)
 * Param: "FIFOChoice": chooses which buffer we want to read from
 * Returns: uint32_t Data from FIFO
 */
int32_t readFIFO(uint32_t FIFOChoice)
{
    // wait for data to be present
    G8RTOS_WaitSemaphore(&(FIFOs[FIFOChoice].currentSize));
    // wait for mutex semaphore
    G8RTOS_WaitSemaphore(&(FIFOs[FIFOChoice].mutex));

    // read the data.
    int32_t ret = *(FIFOs[FIFOChoice].head);
    // increment head to the next int, wrap if necessary
    if(FIFOs[FIFOChoice].head >= &(FIFOs[FIFOChoice].buffer[FIFOSIZE-1]))
    {
        FIFOs[FIFOChoice].head = &(FIFOs[FIFOChoice].buffer[0]);
    }
    else
    {
        FIFOs[FIFOChoice].head++;
    }

    // release the semaphore
    G8RTOS_SignalSemaphore(&(FIFOs[FIFOChoice].mutex));
    return ret;
}

/*
 * Writes to FIFO
 *  Writes data to Tail of the buffer if the buffer is not full
 *  Increments tail (wraps if necessary)
 *  Param "FIFOChoice": chooses which buffer we want to read from
 *        "Data': Data being put into FIFO
 *  Returns: error code for full buffer if unable to write
 */
int writeFIFO(uint32_t FIFOChoice, uint32_t Data)
{
    // wait for semaphore
    //G8RTOS_WaitSemaphore(&(FIFOs[FIFOChoice].mutex));
    *(FIFOs[FIFOChoice].tail) = Data;

    // Increment the tail pointer.
    if(FIFOs[FIFOChoice].tail >= &(FIFOs[FIFOChoice].buffer[FIFOSIZE-1]))
    {
        FIFOs[FIFOChoice].tail = &(FIFOs[FIFOChoice].buffer[0]);
    }
    else
    {
        FIFOs[FIFOChoice].tail++;
    }

    // check to see if buffer is full
    if(FIFOs[FIFOChoice].currentSize >= FIFOSIZE-1)
    {
        // Buffer is full, we need to move the head pointer
        FIFOs[FIFOChoice].lostData++;
        // increment head to the next int, wrap if necessary
        if(FIFOs[FIFOChoice].head >= &(FIFOs[FIFOChoice].buffer[FIFOSIZE-1]))
        {
            FIFOs[FIFOChoice].head = &(FIFOs[FIFOChoice].buffer[0]);
        }
        else
        {
            FIFOs[FIFOChoice].head++;
        }
        return -1;
    }

    G8RTOS_SignalSemaphore(&(FIFOs[FIFOChoice].currentSize));
    //G8RTOS_SignalSemaphore(&(FIFOs[FIFOChoice].mutex));
    return 0;
}

int32_t getMovement(uint32_t FIFOChoice)        // returns 0, 1, or -1
{
    if(FIFOChoice >= MAX_NUMBER_OF_FIFOS)
        return 0;

    // ~average result based
    int32_t sum = 0;
    for (int i = 0; i < FIFOSIZE; i++)
    {
        sum += FIFOs[FIFOChoice].buffer[i];
    }

    if (sum > 8) return 1;
    else if (sum < -8) return -1;
    else return 0;
}
