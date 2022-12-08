/*
 * Joystick.h
 *
 *  Created on: March 20, 2022
 *      Author: Ghanshyam
 */

#ifndef BOARDSUPPORTPACKAGE_JOYSTICK_H_
#define BOARDSUPPORTPACKAGE_JOYSTICK_H_

#define X_COORDINATE 0
#define Y_COORDINATE 1

/*********************************************** Public Functions *********************************************************************/
/*
 * Initializes internal ADC
 * ADC input from PE3 and P32 (AIN0 and AIN1 - X/Y)
 */
void Joystick_Init_Without_Interrupt(void);

/*
 * Init's GPIO interrupt for joystick push button (PE1)
 */
void Joystick_Push_Button_Init_With_Interrupt(void);

/*
 * Functions returns X and Y coordinates
 */
void GetJoystickCoordinates(uint32_t * coordinates);

/*
 * Function to disable joystick interrupt
 */
void Disable_Joystick_Interrupt(void);

/*********************************************** Public Functions *********************************************************************/



#endif /* BOARDSUPPORTPACKAGE_JOYSTICK_H_ */
