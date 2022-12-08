#ifndef I2CDRIVER_H_
#define I2CDRIVER_H_

#include <stdint.h>

// Initializes I2C module for LEDs
void InitializeLEDI2C(uint32_t moduleNumber);

// Initializes I2C module for sensor BoosterPack
void InitializeSensorI2C();

// Set slave address
void SetSlaveAddress(uint32_t moduleNumber, uint16_t address);

// Start transmission
void StartTransmission(uint32_t moduleNumber, uint16_t data);

// Continue transmission
void ContinueTransmission(uint32_t moduleNumber, uint16_t data);

// End transmission
void EndTransmission(uint32_t moduleNumber);

// Signal master receive
void SignalMasterReceive(uint32_t moduleNumber, uint16_t registerAddress, uint16_t slaveAddress);

// Start a receive
uint8_t StartReceive(uint32_t moduleNumber);

// End a receive
uint8_t EndReceive(uint32_t moduleNumber);


#endif /* I2CDRIVER_H_ */
