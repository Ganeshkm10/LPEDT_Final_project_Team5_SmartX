/*
 * ble.h
 *
 *  Created on: Sep 28, 2022
 *      Author: Jasan Singh
 */

#ifndef SRC_BLE_H_
#define SRC_BLE_H_

#include <stdbool.h>
#include "sl_bt_api.h"
#include "app_assert.h"
#include "lcd.h"
#include "ble_device_type.h"
#include "scheduler.h"
#include "circular_buffer.h"
#include "gpio.h"


#define UINT8_TO_BITSTREAM(p, n)    { *(p)++ = (uint8_t) (n); }

#define UINT32_TO_BITSTREAM(p, n)   { *(p)++ = (uint8_t)(n); *(p)++ = (uint8_t)((n) >> 8); \
                                      *(p)++ = (uint8_t)((n) >> 16); *(p)++ = (uint8_t)((n) >> 24); }

#define UINT32_TO_FLOAT(m, e)  (((uint32_t)(m) & 0x00FFFFFFFFU) | (uint32_t)((int32_t)(e) << 24))

#define UINT16_TO_FLOAT(m, e) ((float)(m) * ((e) >= 0 ? 1 << (e) : 1.0 / (1 << -(e))))
#define INT16_TO_FLOAT(m, e) ((float)(m) * ((e) >= 0 ? 1 << (e) : 1.0 / (1 << -(e))))


#define SOFT_TIMER_INTERVAL_FOR_PULSE_SENSOR    32700 // 327 is for a period of 10  ms
#define SOFT_TIMER_PULSE_SENSOR                 5

//BLE data structure
#if DEVICE_IS_BLE_SERVER
typedef struct {

  //values are common to servers and clients
  bd_addr     myAddress;
  uint8_t     myAddressType;
  bool        connectionOpen;


  //values unique for server
  // The advertising set handle allocated from Bluetooth Stack
  uint8_t advertisingSetHandle;
  uint8_t connectionHandle;
  uint8_t readTemperature;
  bool    indicationFlag;  //for temperature reading
  bool    statusFlag;

  //SM related flags
  uint8_t bonding_state;
  //
  uint32_t pb0_val;
  uint32_t pb1_val;
  bool    pb0_indicationFlag;

  uint8_t indication_in_flight;

  // for smartx
  uint16_t ambienttemperature;
  uint16_t pressure;
  uint16_t humidity;
  uint16_t gasvalue;
  uint16_t heartrate;
  uint16_t spo2;

} ble_data_struct_t;
#else
typedef struct {

  //values are common to servers and clients
  bd_addr     myAddress;
  uint8_t     myAddressType;
  bool        connectionOpen;

  //client
  bd_addr serverAddress;
  uint8_t temperature_indications_enabled;
  uint8_t server_connectionHandle;
  uint32_t serviceHandle;
  uint16_t characteristicHandle;
} ble_data_struct_t;
#endif
//function helps enhance isolation avoiding the use of extern
// returns a pointer to BLE data structure
ble_data_struct_t* getBleDataPtr(void);


/*
 *  Function is called on every sl_Bt_on_event()
 *  @p Takes argument as pointer to the BT API messages
 *     of type sl_bt_msg
 *  Returns null
 */
void handle_ble_event(sl_bt_msg_t *evt);



#endif /* SRC_BLE_H_ */
