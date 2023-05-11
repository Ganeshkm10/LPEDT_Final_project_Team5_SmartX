/*
  gpio.c
 
   Created on: Dec 12, 2018
       Author: Dan Walkes
   Updated by Dave Sluiter Dec 31, 2020. Minor edits with #defines.

   March 17
   Dave Sluiter: Use this file to define functions that set up or control GPIOs.

 */


// *****************************************************************************
// Students:
// We will be creating additional functions that configure and manipulate GPIOs.
// For any new GPIO function you create, place that function in this file.
// *****************************************************************************

#include <stdbool.h>
#include "em_gpio.h"
#include <string.h>

#include "gpio.h"


// Student Edit: Define these, 0's are placeholder values.
// See the radio board user guide at https://www.silabs.com/documents/login/user-guides/ug279-brd4104a-user-guide.pdf
// and GPIO documentation at https://siliconlabs.github.io/Gecko_SDK_Doc/efm32g/html/group__GPIO.html
// to determine the correct values for these.



// Set GPIO drive strengths and modes of operation
void gpioInit()
{

  // Student Edit:

//	//GPIO_DriveStrengthSet(LED0_port, gpioDriveStrengthStrongAlternateStrong);
//	GPIO_DriveStrengthSet(LED0_port, gpioDriveStrengthWeakAlternateWeak);
//	GPIO_PinModeSet(LED0_port, LED0_pin, gpioModePushPull, false);
//
//	//GPIO_DriveStrengthSet(LED1_port, gpioDriveStrengthStrongAlternateStrong);
//	GPIO_DriveStrengthSet(LED1_port, gpioDriveStrengthWeakAlternateWeak);
//	GPIO_PinModeSet(LED1_port, LED1_pin, gpioModePushPull, false);
//
	GPIO_DriveStrengthSet(PB0_port, gpioDriveStrengthWeakAlternateWeak);
	GPIO_PinModeSet(PB0_port, PB0_pin,  gpioModeInputPullFilter, true);
	GPIO_ExtIntConfig(PB0_port, PB0_pin, PB0_pin, true, true, true);


	GPIO_DriveStrengthSet(PB1_port, gpioDriveStrengthWeakAlternateWeak);
	GPIO_PinModeSet(PB1_port, PB1_pin,  gpioModeInputPullFilter, true);
	GPIO_ExtIntConfig(PB1_port, PB1_pin, PB1_pin, true, true, true);

	GPIO_DriveStrengthSet(LCD_Enable_Port, gpioDriveStrengthWeakAlternateWeak);
	GPIO_PinModeSet(LCD_Enable_Port, LCD_Enable_Pin, gpioModePushPull, false);


	//enabling pushbutton gpio
//	GPIO_ExtIntConfig(PB0_port, PB0_pin, PB0_pin, true, true, true);
	//GPIO_IntClear( 1 << 6 );
	//GPIO_IntEnable( 1 << 6);

	NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);
	NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);

	NVIC_EnableIRQ(GPIO_EVEN_IRQn);
	NVIC_EnableIRQ(GPIO_ODD_IRQn);

} // gpioInit()


void gpioLed0SetOn()
{
	GPIO_PinOutSet(LED0_port,LED0_pin);
}


void gpioLed0SetOff()
{
	GPIO_PinOutClear(LED0_port,LED0_pin);
}


void gpioLed1SetOn()
{
	GPIO_PinOutSet(LED1_port,LED1_pin);
}


void gpioLed1SetOff()
{
	GPIO_PinOutClear(LED1_port,LED1_pin);
}

void gpioSI7021On()
{
  GPIO_PinOutSet(LCD_Enable_Port,LCD_Enable_Pin);
}

void gpioSI7021Off()
{
  GPIO_PinOutClear(LCD_Enable_Port,LCD_Enable_Pin);
}

void gpioSetDisplayExtcomin(bool value)
{
  if(value)
    GPIO_PinOutSet(LCD_port,LCD_pin);
  else
    GPIO_PinOutClear(LCD_port,LCD_pin);
}

uint32_t gpioPB0PadGet()
{
  // reads the GPPIO pad values
  return GPIO_PinInGet(PB0_port,PB0_pin);

}




