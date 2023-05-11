/*
 * irq.h
 *
 *  Created on: Sep 9, 2022
 *      Author: Jasan Singh
 */

#ifndef SRC_IRQ_H_
#define SRC_IRQ_H_

#include <stdint.h>
#include "em_i2c.h"
/*
 *  Function defines the IRQ handler LED timer 0
 */
void LETIMER0_IRQHandler(void);


void GPIO_EVEN_IRQHandler(void);

void GPIO_ODD_IRQHandler(void);

/*
 * Function returns the time value tracked in milliseconds
 */
uint32_t letimerMilliseconds();



#endif /* SRC_IRQ_H_ */
