#include <stdint.h>
#include <BoardSupport/inc/RGBLedDriver.h>
#include <BoardSupport/inc/I2CDriver.h>
#include "inc/hw_memmap.h"


/* Algorithm to map the 16 bit LED data to 32 bits to send to the LS registers on the LP3943.
*  The algorithm zero extends the 16 bit LED data to 32 bits, then interleaves the 0's with the LED data
*  to give each LED 2 bits to match the format of the LS registers.
*/
static void ConvertData(uint16_t LED_DATA)
{
    REGISTER_LEDS = LED_DATA;
    LEDShiftTemp = (REGISTER_LEDS ^ (REGISTER_LEDS >> 8)) & 0x0000ff00;
    REGISTER_LEDS ^= (LEDShiftTemp ^ (LEDShiftTemp << 8));
    LEDShiftTemp = (REGISTER_LEDS ^ (REGISTER_LEDS >> 4)) & 0x00f000f0;
    REGISTER_LEDS ^= (LEDShiftTemp ^ (LEDShiftTemp << 4));
    LEDShiftTemp = (REGISTER_LEDS ^ (REGISTER_LEDS >> 2)) & 0x0c0c0c0c;
    REGISTER_LEDS ^= (LEDShiftTemp ^ (LEDShiftTemp << 2));
    LEDShiftTemp = (REGISTER_LEDS ^ (REGISTER_LEDS >> 1)) & 0x22222222;
    REGISTER_LEDS ^= (LEDShiftTemp ^ (LEDShiftTemp << 1));
}

void LP3943_PWMColorSet(uint32_t color, uint32_t PWM_DATA, uint32_t LED_DATA)
{
    switch (color)
    {
        case BLUE:
            SetSlaveAddress(I2C0_BASE, 0x60);
            break;
        case GREEN:
            SetSlaveAddress(I2C0_BASE, 0x61);
            break;
        case RED:
            SetSlaveAddress(I2C0_BASE, 0x62);
            break;
        default:
            SetSlaveAddress(I2C0_BASE, 0x00);
    }

    // Send out LS register address
    StartTransmission(I2C0_BASE, 0x12);

    // Send out frequency data
    ContinueTransmission(I2C0_BASE, PWM_DATA);
    ContinueTransmission(I2C0_BASE, PWM_DATA >> 8);
    ContinueTransmission(I2C0_BASE, PWM_DATA >> 16);
    ContinueTransmission(I2C0_BASE, PWM_DATA >> 24);

    // Send out LED data
    ContinueTransmission(I2C0_BASE, LED_DATA);
    ContinueTransmission(I2C0_BASE, LED_DATA >> 8);
    ContinueTransmission(I2C0_BASE, LED_DATA >> 16);
    ContinueTransmission(I2C0_BASE, LED_DATA >> 24);
    EndTransmission(I2C0_BASE);
}

void LP3943_LedModeSet(uint32_t color, uint16_t LED_DATA)
{
    switch (color)
    {
        case BLUE:
            SetSlaveAddress(I2C0_BASE, 0x60);
            break;
        case GREEN:
            SetSlaveAddress(I2C0_BASE, 0x61);
            break;
        case RED:
            SetSlaveAddress(I2C0_BASE, 0x62);
            break;
        default:
            SetSlaveAddress(I2C0_BASE, 0x00);
    }

    ConvertData(LED_DATA);

    // Send out LS register address
    StartTransmission(I2C0_BASE, 0x16);

    // Send out LED Data
    ContinueTransmission(I2C0_BASE, REGISTER_LEDS);
    REGISTER_LEDS >>= 8;
    ContinueTransmission(I2C0_BASE, REGISTER_LEDS);
    REGISTER_LEDS >>= 8;
    ContinueTransmission(I2C0_BASE, REGISTER_LEDS);
    REGISTER_LEDS >>= 8;
    ContinueTransmission(I2C0_BASE, REGISTER_LEDS);
    EndTransmission(I2C0_BASE);
}

void TurnOffLEDs(uint_desig color)
{
    LP3943_LedModeSet(color, 0x0000);
}

void InitializeRGBLEDs()
{
   // InitializeLEDI2C(I2C0_BASE);
  //  TurnOffLEDs(RED);
  //  TurnOffLEDs(GREEN);
  //  TurnOffLEDs(BLUE);
}
