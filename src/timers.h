/*
 * timers.h
 *
 *  Created on: Sep 8, 2022
 *      Author: Jasan Singh
 */

#ifndef SRC_TIMERS_H_
#define SRC_TIMERS_H_

#include "em_letimer.h"
#include "app.h"

#define MSECONDS (1000)
#define USECONDS (1000000)

/*
 *  Function used to initialization of LETIMER0
 */
void LETimer0_Init();

/*
 *  Function to enable NVIC interrupts for LETIMER0
 */
void LETIMER0_ITREnable();

/*
 *   Functions helps atleast wait for us_wait microweconds
 *   keeps polling the Letimer count value
 */
void timerWaitUs( uint32_t us_wait);

/*
 *  Function helps atleast wait for us_wait microseconds
 *  uses COMP1 interrupt to track
 */
void timerWaitUs_irq( uint32_t us_wait);

/*
 *  Function disables LETIMER0 COMP1 interrupt
 */
void timerExpired();


#endif /* SRC_TIMERS_H_ */
