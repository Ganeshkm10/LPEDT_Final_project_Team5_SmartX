/*
 * oscillators.c
 *
 *  Created on: Sep 8, 2022
 *      Author: Jasan Singh
 */

#include "em_cmu.h"
#include "oscillators.h"
#include "app.h"
void LETIMER0_OSC()
{
#if (LOWEST_ENERGY_MODE == EM0 || LOWEST_ENERGY_MODE == EM1 || LOWEST_ENERGY_MODE == EM2)
  CMU_OscillatorEnable( cmuOsc_LFXO, true, true);
  CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFXO);
  CMU_ClockEnable(cmuClock_LFA, true);
  CMU_ClockDivSet(cmuClock_LETIMER0, cmuClkDiv_8);//CMU_ClockDivGet(cmuClock_LFA)); //cmuClock_LETIMER0
  CMU_ClockEnable(cmuClock_LETIMER0, true); //cmuClock_LETIMER/0
#elif (LOWEST_ENERGY_MODE == EM3)
  CMU_OscillatorEnable(cmuOsc_ULFRCO, true, true);
  CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_ULFRCO);
  CMU_ClockDivSet(cmuClock_LETIMER0, cmuClkDiv_1);
  CMU_ClockEnable(cmuClock_LETIMER0, true);
#endif
}
