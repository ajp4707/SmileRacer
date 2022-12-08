/*
 * Joystick.c
 *
 *  Created on: March 20, 2022
 *      Author: Ghanshyam
 */
/*********************************************** Dependencies and Externs *************************************************************/
#include <stdint.h>
#include "BoardSupport/inc/Joystick.h"
#include "driverlib/sysctl.h"
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "inc/hw_memmap.h"
#include "inc/tm4c123gh6pm.h"
#include "driverlib/interrupt.h"
/*********************************************** Dependencies and Externs *************************************************************/


/*********************************************** Defines *********************************************************************/


/*********************************************** Public Variables ********************************************************************/

/* Function Pointer for calling user defined function for joystick button press */
void (*ButtonFunction)(void);

/*********************************************** Public Variables ******************************* *************************************/


/*********************************************** Public Functions *********************************************************************/
/*
 * Initializes internal ADC
 * ADC input from PE3 and PE2 (AIN0 and AIN1)
 */
void Joystick_Init_Without_Interrupt(void)
{
    // MIGHT NEED TO CHANGE THE CLOCK FREQUENCY... UNSTABLE BEHAVIOR AT 50MHZ POSSIBLE

    // Enable ADC Clock
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

    // Enable clock to the GPIO port being used (E)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);

    // Set the alternate function bits for the ADC input pins (PE3 and PE2)
    // Change pins to be Analog Pins
    // Disable the analog isolation circuit
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_2 | GPIO_PIN_3);

    // Enable sample sequence 2 with 2 steps for x and y coordinates
    ADCSequenceConfigure(ADC0_BASE, 2, ADC_TRIGGER_PROCESSOR, 0);
    ADCSequenceStepConfigure(ADC0_BASE, 2, 0, ADC_CTL_CH0 | ADC_CTL_IE);                // X-Coordinate - Step 0
    ADCSequenceStepConfigure(ADC0_BASE, 2, 1, ADC_CTL_CH1 | ADC_CTL_IE | ADC_CTL_END);  // Y-Coordinate - Step 1
    ADCSequenceEnable(ADC0_BASE, 2);

    // Clear any interrupt flags
    ADCIntClear(ADC0_BASE, 2);
}

/*
 * Init's GPIO interrupt for joystick push button (PE1)
 */
void Joystick_Push_Button_Init_With_Interrupt(void)
{
    // Initialize PE1 as input
    GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_1);
    GPIO_PORTE_PUR_R |= 0x02;

    // Initialize PE1 interrupt on falling edge
    GPIOIntClear(GPIO_PORTE_BASE, GPIO_INT_PIN_1);
    GPIOIntTypeSet(GPIO_PORTE_BASE, GPIO_PIN_1, GPIO_FALLING_EDGE);
    GPIOIntEnable(GPIO_PORTE_BASE, GPIO_INT_PIN_1);
    IntEnable(20);
}

/*
 * Function to disable joystick interrupt
 */
void Disable_Joystick_Interrupt(void)
{
    GPIOIntClear(GPIO_PORTE_BASE, GPIO_INT_PIN_1);
    GPIOIntDisable(GPIO_PORTE_BASE, GPIO_INT_PIN_1);
}

/*
 * Functions returns X and Y coordinates
 */
void GetJoystickCoordinates(uint32_t * coordinates)
{
    // Start conversion
    ADCProcessorTrigger(ADC0_BASE, 2);

    // Wait until conversion is complete
    while(!ADCIntStatus(ADC0_BASE, 2, false));

    // Clear ADC interrupt flag
    ADCIntClear(ADC0_BASE, 2);

    // Read from x and y coordinates
    ADCSequenceDataGet(ADC0_BASE, 2, coordinates);
}

void PortEIntHandler(void)
{
    SysCtlDelay(600000);
    GPIOIntClear(GPIO_PORTE_BASE, GPIO_INT_PIN_1);
    //ButtonFunction();
}

/*********************************************** Public Functions *********************************************************************/

