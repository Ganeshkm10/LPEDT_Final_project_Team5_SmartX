/*
 *
 *  @author: Jasan Singh: jasi7234@colorado.edu
 *
 */
#include <stdbool.h>
#include "timers.h"

/* LFXO and low energy mode  */
#if ((LOWEST_ENERGY_MODE == EM0) || (LOWEST_ENERGY_MODE == EM1) || (LOWEST_ENERGY_MODE == EM2))
#define PRESCALAR   8
#define ACTUAL_CLOCK_FREQ (32768/PRESCALAR)

/* Use ULFRCO and prescalar for low energy modes */
#else
#define PRESCALAR   1
#define ACTUAL_CLOCK_FREQ (1000/PRESCALAR)
#endif

#define CTRMAX 0xffff
#define LETIMER_CTR_VAL   ((LETIMER_PERIOD_MS * ACTUAL_CLOCK_FREQ)/1000)
#define LETIMER_COMP1_VAL ((LETIMER_ON_TIME_MS * ACTUAL_CLOCK_FREQ)/1000)

void  LETimer0_Init()
{
  uint32_t temp;

  LETIMER_Init_TypeDef LETIMER0Init_t;

  LETIMER0Init_t.bufTop=false;   // bufTop;  don't load COMP1 into COMP0 when REP0==0
  LETIMER0Init_t.comp0Top=true;  // comp0Top; load COMP0 into CNT on underflow
  LETIMER0Init_t.debugRun=false;  // debugRun; useful to have the timer running when single-stepping in the debugger
  LETIMER0Init_t.enable=false;  // enable;  don't enable when init completes, we'll enable last
  LETIMER0Init_t.out0Pol=0;     // out0Pol; 0 default output pin value
  LETIMER0Init_t.out1Pol=0;     // out1Pol; 0 default output pin value
  LETIMER0Init_t.repMode=letimerRepeatFree;  // repMode; free running mode i.e. load & go forever
  LETIMER0Init_t.ufoa0=letimerUFOANone;   // ufoa0;  no underflow output action
  LETIMER0Init_t.ufoa1=letimerUFOANone;   // ufoa1;  no underflow output action

  LETIMER_Init(LETIMER0 ,&LETIMER0Init_t);

  LETIMER_IntClear (LETIMER0, 0xffffffff);

  //temp = LETIMER_IEN_UF | LETIMER_IEN_COMP1; //enabling interrupt for underflow and comp1
  temp = LETIMER_IEN_UF;
  LETIMER_IntEnable (LETIMER0, temp); //Int Enable

  LETIMER_CounterSet(LETIMER0, LETIMER_CTR_VAL);  //Initializing the CTR value calculated using macros

  LETIMER_CompareSet(LETIMER0, 0, LETIMER_CTR_VAL );   //Initializind COMP0 and Comp1 registers as per design requirements
  //LETIMER_CompareSet(LETIMER0, 1, LETIMER_COMP1_VAL );

  //LETIMER_Enable(LETIMER0, true);  //finally enabling the timer


}

/*
 *  Function to enable the NVIC interrupts for LETIMER0
 */
void LETIMER0_ITREnable()
{
    LETIMER_IntEnable(LETIMER0, (LETIMER_IEN_UF));
    NVIC_ClearPendingIRQ(LETIMER0_IRQn);
    NVIC_EnableIRQ(LETIMER0_IRQn);
    LETIMER_Enable(LETIMER0, 1);
}


/*
 *   Functions helps atleast wait for us_wait microweconds
 *   keeps polling the Letimer count value
 */
void timerWaitUs( uint32_t us_wait)
{
  //Range handle conditions
  // Range  lower limit
    if(us_wait <= (USECONDS/ACTUAL_CLOCK_FREQ))
      us_wait = 0;
    else if(us_wait >= (LETIMER_PERIOD_MS*MSECONDS))// Range upper limit
      us_wait = LETIMER_PERIOD_MS*MSECONDS;


  uint16_t UsTicks = (us_wait * ACTUAL_CLOCK_FREQ)/USECONDS;
  uint16_t CurrCtr = LETIMER_CounterGet(LETIMER0);

  UsTicks = CurrCtr - UsTicks;
  if( UsTicks > LETIMER_CTR_VAL )   //handling CTR loopback case
    {
      UsTicks = CTRMAX - UsTicks;
      UsTicks = LETIMER_CTR_VAL - UsTicks;
    }

   while( LETIMER_CounterGet(LETIMER0) != UsTicks ); // Polls and waits untill Required count is reached


}

/*
 *  Function helps atleast wait for us_wait microseconds
 *  uses COMP1 interrupt to track
 */
void timerWaitUs_irq( uint32_t us_wait)
{
  //Range handle conditions
  // Range  lower limit
  if(us_wait <= (USECONDS/ACTUAL_CLOCK_FREQ))
      us_wait = 0;
  else if(us_wait >= (LETIMER_PERIOD_MS*MSECONDS))// Range upper limit
      us_wait = LETIMER_PERIOD_MS*MSECONDS;


  uint16_t UsTicks = (us_wait * ACTUAL_CLOCK_FREQ)/USECONDS;
  uint16_t CurrCtr = LETIMER_CounterGet(LETIMER0);

  UsTicks = CurrCtr - UsTicks;
  if( UsTicks > LETIMER_CTR_VAL )   //handling CTR loopback case
  {
     UsTicks = CTRMAX - UsTicks;
     UsTicks = LETIMER_CTR_VAL - UsTicks;
  }


  LETIMER_IntClear (LETIMER0, LETIMER_IFC_COMP1);

  //setting up the COMP1 reg to the latest calculated us_ticks
  LETIMER_CompareSet (LETIMER0, 1, UsTicks);

  //Enable the Comp1 Irq
  LETIMER_TypeDef *letimer;
  letimer       = LETIMER0;
  letimer->IEN |= LETIMER_IEN_COMP1;

}


/*
 *  Function disables LETIMER0 COMP1 interrupt
 */
void timerExpired()
{
  //Has to disable the COMP1 interrupt
  LETIMER_TypeDef *letimer;
  letimer       = LETIMER0;
  letimer->IEN &= ~LETIMER_IEN_COMP1;
}
