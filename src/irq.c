/*
 * irq.c
 *
 *  Created on: Sep 9, 2022
 *      Author: Jasan Singh
 */


#include <stdbool.h>
#include "irq.h"
#include "timers.h"
#include "gpio.h"
#include "scheduler.h"
#include "em_gpio.h"

// Include logging specifically for this .c file
#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"


uint32_t timeMs; //to keep track of the total ms since start


/*
 *  Look irq.h for description
 */
void LETIMER0_IRQHandler(void)
{
  uint32_t temp;

  temp = LETIMER_IntGetEnabled( LETIMER0 );
  LETIMER_IntClear (LETIMER0 ,temp);


  if( temp == LETIMER_IEN_UF)
  {
    timeMs += LETIMER_PERIOD_MS; //adds 3000 ms
    schedulerSetEventLETIMER_UF();                  //turns off LED at UF interrrupt
  }
  if( temp == LETIMER_IEN_COMP1)
    schedulerSetEventLETIMER_COMP1();                    //turns on LED at COMP1 interrupt

  //NVIC_EnableIRQ(LETIMER0_IRQn);


}

/*
 * Function returns the time value tracked in milliseconds
 */
uint32_t letimerMilliseconds()
{
  return timeMs;
}


void I2C0_IRQHandler (void)
{
  I2C_TransferReturn_TypeDef transferStatus;

  transferStatus = I2C_Transfer(I2C0);
  if( transferStatus == i2cTransferDone) {
      schedulerSetEventI2CTransferComplete();
  }

  if( transferStatus < 0 ) {
      LOG_ERROR("%d", transferStatus);
  }

}

//void GPIO_EVEN_IRQHandler(void)
//{
//    uint32_t flags = GPIO_IntGetEnabled();
//
//    // Clear the interrupt
//    GPIO_IntClear(flags);
//
//    if( flags == ( 1 << PB0_pin))
//    {
//        schedulerSetEventPB0Int();
//    }
//
//
//}
//
//void GPIO_ODD_IRQHandler(void)
//{
//    uint32_t flags = GPIO_IntGetEnabled();
//
//    // Clear the interrupt
//    GPIO_IntClear(flags);
//
//    if( flags == ( 1 << PB1_pin))
//    {
//        schedulerSetEventPB1Int();
//    }
//
//
//}


void GPIO_EVEN_IRQHandler(void)
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();
  uint32_t flags;
  flags = GPIO_IntGetEnabled();
  GPIO_IntClear(flags);
  uint8_t PB_Status = GPIO_PinInGet(PB0_port, PB0_pin);
  if(flags==(1<<PB0_pin))
    {
      if(!PB_Status)
        {
          //schedulerSetEventButtonPressed_PB0();
          schedulerSetEventPB0Int();

        }
//      else
//        {
//          schedulerSetEventButtonReleased_PB0();
//          GPIO_IntClear(flags);
//        }
    }
  CORE_EXIT_CRITICAL();
}

void GPIO_ODD_IRQHandler(void)
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();
    uint32_t flags;
    flags = GPIO_IntGetEnabled();
    GPIO_IntClear(flags);
    uint8_t PB_Status = GPIO_PinInGet(PB1_port, PB1_pin);
    if(flags==(1<<PB1_pin))
      {
        if(!PB_Status)
          {
            //schedulerSetEventButtonPressed_PB1();
            schedulerSetEventPB1Int();

          }
//        else
//          {
//            schedulerSetEventButtonReleased_PB1();
//            GPIO_IntClear(flags);
//          }
      }
    CORE_EXIT_CRITICAL();
}
