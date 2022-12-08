#include <stdint.h>
#include <stdbool.h>
#include "driverlib/i2c.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "inc/hw_i2c.h"
#include "inc/hw_memmap.h"
#include "inc/tm4c123gh6pm.h"

static uint8_t I2CPeripheralOffset;

void InitializeLEDI2C(uint32_t moduleNumber)
{
    I2CPeripheralOffset = moduleNumber & 0x0000F000;

    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0 + I2CPeripheralOffset);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    GPIOPinConfigure(GPIO_PB2_I2C0SCL);
    GPIOPinConfigure(GPIO_PB3_I2C0SDA);

    GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);
    GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);

    I2CMasterInitExpClk(moduleNumber, SysCtlClockGet(), true);
}

void InitializeSensorI2C()
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    GPIOPinConfigure(GPIO_PA6_I2C1SCL);
    GPIOPinConfigure(GPIO_PA7_I2C1SDA);

    GPIOPinTypeI2CSCL(GPIO_PORTA_BASE, GPIO_PIN_6);
    GPIOPinTypeI2C(GPIO_PORTA_BASE, GPIO_PIN_7);

    I2CMasterInitExpClk(I2C1_BASE, SysCtlClockGet(), true);
}

void SetSlaveAddress(uint32_t moduleNumber, uint16_t address)
{
    I2CMasterSlaveAddrSet(moduleNumber, address, false);
}

void StartTransmission(uint32_t moduleNumber, uint16_t data)
{
    I2CMasterDataPut(moduleNumber, data);
    I2CMasterControl(moduleNumber, I2C_MASTER_CMD_BURST_SEND_START);
    while(I2CMasterBusy(moduleNumber));
}

void ContinueTransmission(uint32_t moduleNumber, uint16_t data)
{
    I2CMasterDataPut(moduleNumber, data);
    I2CMasterControl(moduleNumber, I2C_MASTER_CMD_BURST_SEND_CONT);
    while(I2CMasterBusy(moduleNumber));
}

void EndTransmission(uint32_t moduleNumber)
{
    I2CMasterControl(moduleNumber, I2C_MASTER_CMD_BURST_SEND_STOP);
}

void SignalMasterReceive(uint32_t moduleNumber, uint16_t registerAddress, uint16_t slaveAddress)
{
    I2CMasterDataPut(moduleNumber, registerAddress);
    I2CMasterControl(moduleNumber, I2C_MASTER_CMD_BURST_RECEIVE_START);
    SysCtlDelay(100);
    while(I2CMasterBusy(moduleNumber));

    I2CMasterSlaveAddrSet(moduleNumber, slaveAddress, true);
}

uint8_t StartReceive(uint32_t moduleNumber)
{
    I2CMasterControl(moduleNumber, I2C_MASTER_CMD_BURST_RECEIVE_START);
    SysCtlDelay(100);
    while(I2CMasterBusy(moduleNumber));
    uint8_t data = (I2CMasterDataGet(moduleNumber) & 0xFF);
    return data;
}

uint8_t EndReceive(uint32_t moduleNumber)
{
    I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
    SysCtlDelay(100);
    while(I2CMasterBusy(I2C1_BASE));
    uint8_t data = (I2CMasterDataGet(I2C1_BASE));
    return data;
}



