/**
 * G8RTOS IPC header
 * @author:
 * uP2 - Fall 2022
 */

#ifndef G8RTOS_G8RTOS_IPC_H_
#define G8RTOS_G8RTOS_IPC_H_

/*
 * Initializes One to One FIFO Struct
 */
int G8RTOS_InitFIFO(uint32_t FIFOIndex);

/*
 * Reads FIFO
 *  - Waits until CurrentSize semaphore is greater than zero
 *  - Gets data and increments the head ptr (wraps if necessary)
 * Param "FIFOChoice": chooses which buffer we want to read from
 * Returns: uint32_t Data from FIFO
 */
int32_t readFIFO(uint32_t FIFO);

/*
 * Writes to FIFO
 *  Writes data to Tail of the buffer if the buffer is not full
 *  Increments tail (wraps if ncessary)
 *  Param "FIFOChoice": chooses which buffer we want to read from
 *        "Data': Data being put into FIFO
 *  Returns: error code for full buffer if unable to write
 */
int writeFIFO(uint32_t FIFO, uint32_t data);

int32_t getMovement(uint32_t FIFOChoice);

#endif /* G8RTOS_G8RTOS_IPC_H_ */
