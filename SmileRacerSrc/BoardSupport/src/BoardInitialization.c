/*
 * BoardInitialization.c
 */

#include <stdbool.h>
#include "BoardSupport/inc/BoardInitialization.h"
#include "BoardSupport/inc/I2CDriver.h"
#include "BoardSupport/inc/RGBLedDriver.h"
#include "BoardSupport/inc/opt3001.h"
#include "BoardSupport/inc/bmi160_support.h"
#include "BoardSupport/inc/tmp007.h"
#include "BoardSupport/inc/bme280_support.h"
#include "BoardSupport/inc/Joystick.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/pin_map.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "inc/tm4c123gh6pm.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"
#include "driverlib/fpu.h"



static void InitConsole(void) // configures UART2
{
    /*
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART2);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    GPIOPinConfigure(GPIO_PD6_U2RX);
    GPIOPinConfigure(GPIO_PD7_U2TX);
    GPIOPinTypeUART(GPIO_PORTD_BASE, GPIO_PIN_6 | GPIO_PIN_7);

    UARTClockSourceSet(UART2_BASE, UART_CLOCK_PIOSC);
    //UARTStdioConfig(2, 9600, SysCtlClockGet());
    UARTConfigSetExpClk(UART2_BASE, SysCtlClockGet(), 115200, UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);

    UARTEnable(UART2_BASE);
    UARTFIFOEnable(UART2_BASE);
    //configure UART interrupt
    IntEnable(INT_UART2);
    */

    // blelgkjsfdj;adkfj

    FPUEnable();
    FPULazyStackingEnable();

    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);

    GPIOPinConfigure(GPIO_PC4_U1RX);
    GPIOPinConfigure(GPIO_PC5_U1TX);
    GPIOPinTypeUART(GPIO_PORTC_BASE, GPIO_PIN_4 | GPIO_PIN_5);

    //
    // Configure the UART for 115,200, 8-N-1 operation.
    //
    UARTConfigSetExpClk(UART1_BASE, SysCtlClockGet(), 115200,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_PAR_NONE));
    IntEnable(INT_UART1);
    UARTIntEnable(UART1_BASE, UART_INT_RX | UART_INT_RT);
}

static void EnableSwitchInterrupt(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_4);
    GPIO_PORTF_PUR_R |= 0x10;

    // Initialize PB4 interrupt on falling edge
    GPIOIntClear(GPIO_PORTF_BASE, GPIO_INT_PIN_4);
    GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_FALLING_EDGE);
    GPIOIntEnable(GPIO_PORTF_BASE, GPIO_INT_PIN_4);
    IntEnable(INT_GPIOF);
}

static void ConfigureTimer(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    TimerConfigure(TIMER0_BASE, TIMER_CFG_A_ONE_SHOT);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    TimerPrescaleSet(TIMER0_BASE, TIMER_A, 1);
    IntEnable(INT_TIMER0A);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
    TimerConfigure(TIMER1_BASE, TIMER_CFG_A_ONE_SHOT);
    TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    TimerPrescaleSet(TIMER1_BASE, TIMER_A, 1);
    IntEnable(INT_TIMER1A);
}

bool InitializeBoard(void)
{
    // WatchDog timer is off by default (will generate errors in debug window unless clock gating is enabled to it)

    // Set clock speed higher using the PLL (50 MHz)
    SysCtlClockSet(SYSCTL_SYSDIV_4|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);

    // Initialize I2C for the LEDs and the Sensor BooserPack
    InitializeLEDI2C(I2C0_BASE);
    InitializeSensorI2C();

    // Initialize and Test OPT3001 Digital Ambient Light Sensor (ALS)
    //sensorOpt3001Enable(true);
    //bool isLightSensorFunctional = sensorOpt3001Test();

    // Check if ALS is functional
    //if (!isLightSensorFunctional)
    //    return false;

    // Initialize and Test TMP007 IR Temperature Sensor
    //sensorTmp007Enable(true);
    //bool isTempSensorFunctional = sensorTmp007Test();

    // Check if temperature sensor is functional
    //if (!isTempSensorFunctional)
    //    return false;

    // Initialize BMI160 Accelerometer and Gyroscope
    //bmi160_initialize_sensor();

    // Initialize BME280 Humidity Sensor
    //bme280_initialize_sensor();

    // Initialize Joystick
    //Joystick_Init_Without_Interrupt();
    //Joystick_Push_Button_Init_With_Interrupt();

    // Initialize Console UART from TIVA SDK
    InitConsole();

    // Initialize RGB LEDs
 //   InitializeRGBLEDs();

    // Initialize switch interrupt
    EnableSwitchInterrupt();

    //Configure timer used to debounce
    ConfigureTimer();

    return true;
}


