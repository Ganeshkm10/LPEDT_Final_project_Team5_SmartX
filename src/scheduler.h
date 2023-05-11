/*
 * scheduler.h
 *
 *  Created on: Sep 13, 2022
 *      Author: Jasan Singh
 */

#ifndef SRC_SCHEDULER_H_
#define SRC_SCHEDULER_H_

#include <stdint.h>
#include "em_core.h"
#include <stdint.h>
#include "sl_power_manager.h"
#include "sl_bt_api.h"

typedef enum uint32_t {
  evtIdle                = 0x00,
  evtLETIMER0_UF         = 0x01,
  evtLETIMER0_COMP1      = 0x02,
  evtI2CTransferComplete = 0x04,
  evtPB0Pressed          = 0x08,
  evtPB1Pressed          = 0x10
} event_t;

typedef enum{
  state_Sleep,
  state_TimeDisplay,
  state_T_H_P_AQ,
  state_HR_SPO2,
  state_BLE_Connect,
}features_t;

/*
 *  Gets the next event in the scheduler
 *  Returns the respective event enum value
 */
//uint32_t getNextEvent();

/*
 * Function calls BT external signal and passes the UnderFlow event occurred
 * arguments : null
 * return    : null
 */
void schedulerSetEventLETIMER_UF();

/*
 * Function calls BT external signal and passes the COMP1 event occurred
 * arguments : null
 * return    : null
 */
void schedulerSetEventLETIMER_COMP1();

/*
 * Function calls BT external signal and passes the I2C transfer event occurred
 * arguments : null
 * return    : null
 */
void schedulerSetEventI2CTransferComplete();


void schedulerSetEventPB0Int();

void schedulerSetEventPB1Int();


/*
 *   Function implement statemachine for temperature measurement
 *   from Si7021 based on the BL API messages
 *   argument : Takes pointer to BT API messagees
 *   Return   : null
 */
void temperature_state_machine(sl_bt_msg_t *evt);

void smartx_features(sl_bt_msg_t *evt);

void HR_SPO2_measure_state_machine(sl_bt_msg_t *event);

/*
 *  handles client side discoverya nd indications event
 *  Returns Null
 */
void discovery_state_machine(sl_bt_msg_t *evt);

#endif /* SRC_SCHEDULER_H_ */
