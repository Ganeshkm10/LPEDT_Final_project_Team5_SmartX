/*
 * ble.c
 *
 *  Created on: Sep 28, 2022
 *      Author: Jasan Singh
 */

#include "gatt_db.h" // # GATT database

#include "ble.h"

// Include logging specifically for this .c file
#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"
#include <math.h>


#if (DEVICE_IS_BLE_SERVER)
//Macros to set advertising intervals in milliseconds
#define ADVMIN (250)
#define ADVMAX (250)

//Macros to connection event interval i milliseconds
#define CONMIN (75)
#define CONMAX (75)
#define SM_CONFIGURATION_MASK 0x0f

#else
// connection parameters
#define CONN_INTERVAL_MIN             60   //  60*1.25ms = 75ms
#define CONN_INTERVAL_MAX             60   //  60*1.25ms = 75ms
#define CONN_RESPONDER_LATENCY        4    //no latency
#define CONN_TIMEOUT                  (82.5)  //82.5*10ms = 825ms
#define CONN_MIN_CE_LENGTH            0
#define CONN_MAX_CE_LENGTH            4

#define SCAN_INTERVAL                 80   //10ms
#define SCAN_WINDOW                   40   //10ms
#define SCAN_PASSIVE                  0
#endif

//BLE private data
ble_data_struct_t     ble_data;

ble_data_struct_t* getBleDataPtr() {

  return (&ble_data);
}


//sending buffer
uint32_t Ambient_temp_Buffer;
uint32_t Pressure_Buffer;
uint32_t Humidity_Buffer;

uint32_t Heart_Rate_Buffer;
uint32_t SPO2_Buffer;



#if (DEVICE_IS_BLE_SERVER==0)
/*
 *  Verify's and confirms the slave address matches the required server address
 *  returns true if matches else false
 */
bool ConfirmSlaveAddr(sl_bt_msg_t *evt)
{
  bool ret;
  bd_addr required_slave_addr = SERVER_BT_ADDRESS;
  ble_data.serverAddress = required_slave_addr;

  for(int i=0; i<6; i++)
    {
      if(evt->data.evt_scanner_scan_report.address.addr[i] != required_slave_addr.addr[i])
             {
               ret = false;
               break;
             }
      else
        {   ret = true; }
    }
  return ret;
}


// -----------------------------------------------
// Private function, original from Dan Walkes. I fixed a sign extension bug.
// We'll need this for Client A7 assignment to convert health thermometer
// indications back to an integer. Convert IEEE-11073 32-bit float to signed integer.
// -----------------------------------------------
static int32_t FLOAT_TO_INT32(const uint8_t *value_start_little_endian)
{
    uint8_t signByte = 0;
    int32_t mantissa;
    // input data format is:
    // [0] = flags byte
    // [3][2][1] = mantissa (2's complement)
    // [4] = exponent (2's complement)
    // BT value_start_little_endian[0] has the flags byte
    int8_t exponent = (int8_t)value_start_little_endian[4];
    // sign extend the mantissa value if the mantissa is negative
    if (value_start_little_endian[3] & 0x80) { // msb of [3] is the sign of the mantissa
    signByte = 0xFF;
    }

    mantissa = (int32_t) (value_start_little_endian[1] << 0) | \
                         (value_start_little_endian[2] << 8) | \
                         (value_start_little_endian[3] << 16) | \
                         (signByte << 24) ;

    // value = 10^exponent * mantissa, pow() returns a double type
    return (int32_t) (pow(10, exponent) * mantissa);
} // FLOAT_TO_INT32
#endif
/*
 *  Function is called on every sl_Bt_on_event()
 *  @p Takes argument as pointer to the BT API messages
 *     of type sl_bt_msg
 *  Returns null
 */
void handle_ble_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  uint8_t address_type;

#if DEVICE_IS_BLE_SERVER
  uint8_t PB0_buf[5]; //buffer to update GATT characterstics
  uint8_t * buf_ptr = PB0_buf;
  uint16_t charHandle;
  size_t   bufferLength;

  //for queue support
  uint32_t pending_indications=0;


#else
  //client
  bool required_slave_addr_found = false;
  const uint8_t * temp_buf;
  int32_t temperature_val;
#endif
//  if( SL_BT_MSG_ID(evt->header) == sl_bt_evt_system_external_signal_id)
//    {
//      return;
//    }

  switch( SL_BT_MSG_ID(evt->header))
  {

    case sl_bt_evt_system_boot_id:

     LOG_INFO("Bluetooth entered Boot mode \r");
     ble_data.connectionOpen=0;
//     ble_data.bonding_state=0;
//     ble_data.connectionHandle=0;
     sc = sl_bt_system_set_soft_timer(SOFT_TIMER_INTERVAL_FOR_PULSE_SENSOR, SOFT_TIMER_PULSE_SENSOR, false);
           if (sc != SL_STATUS_OK) {
                     LOG_ERROR("Soft time 10ms error\r\n");
                  }


     sc = sl_bt_system_get_identity_address(&ble_data.myAddress, &address_type);

           if (sc != SL_STATUS_OK) {
               LOG_ERROR("sl_bt_system_get_identity_address() returned != 0 status=0x%04x", (unsigned int) sc);
           }

#if DEVICE_IS_BLE_SERVER

      // Create an advertising set.
      ble_data.advertisingSetHandle = 0xff; // DOS
      sc = sl_bt_advertiser_create_set(&(ble_data.advertisingSetHandle));
      //DOS app_assert_status(sc);
      if (sc != SL_STATUS_OK) {
          LOG_ERROR("sl_bt_advertiser_create_set() returned != 0 status=0x%04x", (unsigned int) sc);
      }

      //setting the advertising timings according to assignment requirement
      sc = sl_bt_advertiser_set_timing(
                ble_data.advertisingSetHandle, // advertising set handle
                (ADVMIN *1.6), // min. adv. interval (milliseconds * 1.6)
                (ADVMAX *1.6), // max. adv. interval (milliseconds * 1.6)
                0,   // adv. duration
                0);  // max. num. adv. events
      //DOS app_assert_status(sc);
      if (sc != SL_STATUS_OK) {
          LOG_ERROR("sl_bt_advertiser_set_timing() returned != 0 status=0x%04x", (unsigned int) sc);
      }

       // Start general advertising and enable connections.
       sc = sl_bt_advertiser_start(
                 ble_data.advertisingSetHandle,
                 sl_bt_advertiser_general_discoverable,
                 sl_bt_advertiser_connectable_scannable);
       //DOS app_assert_status(sc);
       if (sc != SL_STATUS_OK) {
           LOG_ERROR("sl_bt_advertiser_start() returned != 0 status=0x%04x", (unsigned int) sc);
       }

       LOG_INFO("Started advertising\r\n");

//       sc = sl_bt_sm_configure(SM_CONFIGURATION_MASK, sl_bt_sm_io_capability_displayyesno);
//
//       if (sc != SL_STATUS_OK)
//       {
//         LOG_ERROR("sl_bt_sm_configure() returned != 0 status=0x%04x\r\n", (unsigned int) sc);
//       }
//
//       sc = sl_bt_sm_delete_bondings();
//
//       if (sc != SL_STATUS_OK)
//         {
//           LOG_ERROR("sl_bt_sm_delete_bondings() returned != 0 status=0x%04x\r\n", (unsigned int) sc);
//         }

       sc = sl_bt_system_set_soft_timer(4096, 1, 0);  //repeated timer 125ms

       if (sc != SL_STATUS_OK)
        {
           LOG_ERROR("sl_bt_system_set_soft_timer() returned != 0 status=0x%04x", (unsigned int) sc);
        }

       //TURNoff leds
//       gpioLed0SetOff();gpioLed1SetOff();
#else
       //client


                   sc = sl_bt_scanner_set_mode(sl_bt_gap_1m_phy, SCAN_PASSIVE);

                   sc = sl_bt_scanner_set_timing(sl_bt_gap_1m_phy, SCAN_INTERVAL, SCAN_WINDOW);
                   if (sc != SL_STATUS_OK) {
                    LOG_ERROR("sl_bt_scanner_set_timing() returned != 0 status=0x%04x", (unsigned int) sc);
                     }
                    // Set the default connection parameters for subsequent connections
                    sc = sl_bt_connection_set_default_parameters(CONN_INTERVAL_MIN,
                                                                 CONN_INTERVAL_MAX,
                                                                 CONN_RESPONDER_LATENCY,
                                                                 CONN_TIMEOUT,
                                                                 CONN_MIN_CE_LENGTH,
                                                                 CONN_MAX_CE_LENGTH);
                    if (sc != SL_STATUS_OK) {
                                 LOG_ERROR(" sl_bt_connection_set_default_parameters() returned != 0 status=0x%04x", (unsigned int) sc);
                                  }
                    sc = sl_bt_scanner_start(sl_bt_gap_1m_phy, sl_bt_scanner_discover_generic);
                    if (sc != SL_STATUS_OK) {
                                 LOG_ERROR("sl_bt_scanner_start() returned != 0 status=0x%04x", (unsigned int) sc);
                                  }
#endif

          // Initializing the display and printing Bluetooth address obtained after boot
//          displayInit();
//          displayPrintf(DISPLAY_ROW_NAME, BLE_DEVICE_TYPE_STRING);
//          displayPrintf(DISPLAY_ROW_BTADDR, "%02X:%02X:%02X:%02X:%02X:%02X", ble_data.myAddress.addr[0], ble_data.myAddress.addr[1], ble_data.myAddress.addr[2], \
//                                    ble_data.myAddress.addr[3], ble_data.myAddress.addr[4], ble_data.myAddress.addr[5]);
#if DEVICE_IS_BLE_SERVER
//          displayPrintf(DISPLAY_ROW_CONNECTION, "Advertising");
#else
          displayPrintf(DISPLAY_ROW_CONNECTION, "Discovering");
#endif
//          displayPrintf(DISPLAY_ROW_ASSIGNMENT, "A8");

          break; // sl_bt_evt_system_boot_id




    case sl_bt_evt_connection_opened_id:

     // LOG_INFO("Connection opened\r\n");

      ble_data.connectionOpen = true; // DOS

#if DEVICE_IS_BLE_SERVER
      // DOS save this handle, use everywhere else while connection is open for
      // API routines that require a connection handle
      ble_data.connectionHandle = evt->data.evt_connection_opened.connection;

     sc = sl_bt_advertiser_stop(ble_data.advertisingSetHandle);
     if (sc != SL_STATUS_OK) {
        LOG_ERROR("sl_bt_advertiser_stop() returned != 0 status=0x%04x", (unsigned int) sc);
     }

      sc = sl_bt_connection_set_parameters(
          evt->data.evt_connection_opened.connection,
          (CONMIN/1.25),  // value = 75ms/1.25 @api
          (CONMAX/1.25),  // value = 75ms/1.25 @api
          4,        // 300ms/75ms = 4
          75,
          0,
          0xFFFF);
      if (sc != SL_STATUS_OK) {
         LOG_ERROR("sl_bt_connection_set_parameters() returned != 0 status=0x%04x", (unsigned int) sc);
       }

      // Updating Connection to connected on LCD
//      displayPrintf(DISPLAY_ROW_TEMPVALUE, "Connected");
#else
        // CLIENT
        //Get the connection handle and save it and set connection opened event
        ble_data.server_connectionHandle = evt->data.evt_connection_opened.connection;


        /* Display Connected on the LCD and Server Address */
        displayPrintf(DISPLAY_ROW_CONNECTION, "Connected");
        displayPrintf(DISPLAY_ROW_BTADDR2, "%02X:%02X:%02X:%02X:%02X:%02X", ble_data.serverAddress.addr[0], \
                      ble_data.serverAddress.addr[1], ble_data.serverAddress.addr[2], \
                      ble_data.serverAddress.addr[3], ble_data.serverAddress.addr[4], \
                      ble_data.serverAddress.addr[5]);

#endif
    break; // sl_bt_evt_connection_opened_id



    case sl_bt_evt_connection_closed_id:

        //LOG_INFO("Connection terminated\r\n");
#if DEVICE_IS_BLE_SERVER


        ble_data.connectionOpen = false; // DOS
        ble_data.indicationFlag = false;
        //turn off LEDS
//        gpioLed0SetOff();gpioLed1SetOff();


        // Restart advertising after client has disconnected.
        sc = sl_bt_advertiser_start(
                  ble_data.advertisingSetHandle,
                  sl_bt_advertiser_general_discoverable,
                  sl_bt_advertiser_connectable_scannable);
        if (sc != SL_STATUS_OK)
        {
           LOG_ERROR("sl_bt_advertiser_start() returned != 0 status=0x%04x", (unsigned int) sc);
        }
        //app_assert_status(sc);

//       sc = sl_bt_sm_delete_bondings();
//
//       if (sc != SL_STATUS_OK)
//         {
//           LOG_ERROR("sl_bt_sm_delete_bondings() returned != 0 status=0x%04x\r\n", (unsigned int) sc);
//         }

        //LOG_INFO("Started advertising\r\n");

        //Clearing off any previously written Temperature value
//        displayPrintf(DISPLAY_ROW_TEMPVALUE, " ");
//        //Updating back the status to advertising on LCD
//        displayPrintf(DISPLAY_ROW_CONNECTION, "Advertising");
//        displayPrintf(DISPLAY_ROW_9, " ");
//        displayPrintf(DISPLAY_ROW_ACTION, " ");
#else
        //CLIENT
        //start scanning
        sc = sl_bt_scanner_start(sl_bt_gap_phy_1m, sl_bt_scanner_discover_observation);

        if (sc != SL_STATUS_OK)
          {
            LOG_ERROR("sl_bt_scanner_start() returned != 0 status=0x%04x", (unsigned int) sc);
          }


        /* Display Discovering on the LCD */
        displayPrintf(DISPLAY_ROW_TEMPVALUE, " ");
        displayPrintf(DISPLAY_ROW_BTADDR2, " ");
        displayPrintf(DISPLAY_ROW_CONNECTION, "Discovering");
#endif
    break; // sl_bt_evt_connection_closed_id


     case sl_bt_evt_connection_parameters_id:
            LOG_INFO("Interval = %d\r\n Latency = %d\r\n Timeout = %d\r\n",
                      evt->data.evt_connection_parameters.interval,
                      evt->data.evt_connection_parameters.latency,
                      evt->data.evt_connection_parameters.timeout);
     break;

     case sl_bt_evt_gatt_server_indication_timeout_id:
          //can be used to track whether an indication is in flight or not
       break;


     //Event every 1 sec to refresh, helps prevent charge buildup by refreshing EXTCOMIN
     case sl_bt_evt_system_soft_timer_id:
       if(evt->data.evt_system_soft_timer.handle == 0)  //for sof timer handle 0
         displayUpdate();
       if(evt->data.evt_system_soft_timer.handle == 5)  //for sof timer handle 0
//                LOG_INFO("10 ms soft timer \r");
//       else if(evt->data.evt_system_soft_timer.handle == 1) // for cb_fifo i.e handle 1
//         {
//           //for handle 1
//           //check whether ther somethingh onthe queue
//           pending_indications = get_queue_depth();
//           if((pending_indications > 0) && (ble_data.indication_in_flight != 0 ))
//             {
//               //dequeue 1 indication
//               if(read_queue ( &charHandle, &bufferLength,  buf_ptr))
//                 {
//                   LOG_ERROR("Empty Queue\r\n");
//                 }
//               else
//                 {
//                   //send the indication
//                    sc = sl_bt_gatt_server_send_indication(ble_data.connectionHandle,
//                                                                             charHandle,
//                                                                             bufferLength,
//                                                                             buf_ptr
//                                                                             );
//
//                    if (sc != SL_STATUS_OK)
//                      {
//                        LOG_ERROR("sl_bt_gatt_server_send_indication() returned != 0 status=0x%04x\r\n", (unsigned int) sc);
//                      }
//                    else
//                      {
//                        ble_data.indication_in_flight = 1; //high when indication is send
//                      }
//                 }
//
//
//             }
//         }
       break;


#if DEVICE_IS_BLE_SERVER==0
       /********* Events only for client ******************/
     case sl_bt_evt_scanner_scan_report_id:
       required_slave_addr_found = ConfirmSlaveAddr(evt);
       if(required_slave_addr_found)
                {
                  // Server found proceed to stop scanning
//                  sc = sl_bt_scanner_stop();
//
//                  if (sc != SL_STATUS_OK)
//                    {
//                      LOG_ERROR("sl_bt_scanner_stop() returned != 0 status=0x%04x", (unsigned int) sc);
//                    }

                  /* Connect to the Server */
                  sc = sl_bt_connection_open(evt->data.evt_scanner_scan_report.address,
                                             evt->data.evt_scanner_scan_report.address_type,
                                             sl_bt_gap_phy_1m, NULL);

                  if (sc != SL_STATUS_OK)
                    {
                      LOG_ERROR("sl_bt_connection_open() returned != 0 status=0x%04x", (unsigned int) sc);
                    }

                }

       break;

     case sl_bt_evt_gatt_service_id:
              //A GATT service in the remote GATT database was discovered
              //(Save GATT HTM service handle)
              ble_data.serviceHandle = evt->data.evt_gatt_service.service;
              break;

     case sl_bt_evt_gatt_characteristic_id:
              //A GATT characteristic in the remote GATT database was discovered
              //(Save GATT HTM characterstics handle)
              ble_data.characteristicHandle = evt->data.evt_gatt_characteristic.characteristic;
              break;

     case sl_bt_evt_gatt_characteristic_value_id:
              //If an indication or notification has been enabled for a characteristic, this event is triggered whenever an indication or notification is received from the remote GATT server.
              //Is the characteristic handle and att_opcode we expect? If yes:
              if( (evt->data.evt_gatt_characteristic_value.att_opcode ==  sl_bt_gatt_handle_value_indication) && (evt->data.evt_gatt_characteristic.characteristic == ble_data.characteristicHandle ) )
                {
                  sc = sl_bt_gatt_send_characteristic_confirmation(ble_data.server_connectionHandle);
                  if (sc != SL_STATUS_OK)
                    {
                       LOG_ERROR(" sl_bt_gatt_send_characteristic_confirmation() returned != 0 status=0x%04x", (unsigned int) sc);
                    }

                  //convert and display temperature
                   temp_buf = &(evt->data.evt_gatt_characteristic_value.value.data[0]);
                   temperature_val = FLOAT_TO_INT32(temp_buf);
                   //Updating temperature on LCD
                   displayPrintf(DISPLAY_ROW_CONNECTION, "Handling Indications");
                   displayPrintf(DISPLAY_ROW_TEMPVALUE, "Temp=%d", (int)temperature_val);
                }

              //
              break;

#endif



      /*
       *  Events only for SERVER
       *
       */
#if DEVICE_IS_BLE_SERVER
    case sl_bt_evt_gatt_server_characteristic_status_id:

      if(evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_ambient_temperature_bme680)
              {
                //Indications are enabled
                if(evt->data.evt_gatt_server_characteristic_status.client_config_flags == sl_bt_gatt_server_indication)
                  {
//                    uint8_t htm_temperature_buffer[5]={0};
//                     uint8_t *p = htm_temperature_buffer;
//                      uint32_t htm_temperature_flt;
//
//                     UINT8_TO_BITSTREAM(p, 0x00);
//
//                     htm_temperature_flt = UINT16_TO_FLOAT(((ble_data.ambienttemperature)*1000), -2);
//
//                                UINT32_TO_BITSTREAM(p, htm_temperature_flt);



//                    Ambient_temp_Buffer[0]=0;
                    Ambient_temp_Buffer=(uint32_t)ble_data.ambienttemperature;

                    ble_data.connectionHandle = evt->data.evt_gatt_server_characteristic_status.connection;
                    ble_data.indicationFlag = true;
                    sc = sl_bt_gatt_server_send_indication(ble_data.connectionHandle,
                                                           gattdb_ambient_temperature_bme680,
                                                           4,
                                                           (uint8_t*)&Ambient_temp_Buffer);

                  }
                else
                  {
                    ble_data.indicationFlag = false;
//                    gpioLed0SetOff();
                    //LOG_INFO("Indications Disabled %d\n\r",0);
                    // Clearing back the temperature on Screen after the indication have been displayed
//                    displayPrintf(DISPLAY_ROW_TEMPVALUE, " ");
                  }
              }

      //Pressure send

          if(evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_pressure_bme680)
                        {
                          //Indications are enabled
                          if(evt->data.evt_gatt_server_characteristic_status.client_config_flags == sl_bt_gatt_server_indication)
                            {

                              Pressure_Buffer=(uint32_t)ble_data.pressure;

                              ble_data.connectionHandle = evt->data.evt_gatt_server_characteristic_status.connection;
                              ble_data.indicationFlag = true;
                              sc = sl_bt_gatt_server_send_indication(ble_data.connectionHandle,
                                                                     gattdb_pressure_bme680,
                                                                     4,
                                                                     (uint8_t*)&Pressure_Buffer);

                            }
                          else
                            {
                              ble_data.indicationFlag = false;
          //                    gpioLed0SetOff();
                              //LOG_INFO("Indications Disabled %d\n\r",0);
                              // Clearing back the temperature on Screen after the indication have been displayed
//                              displayPrintf(DISPLAY_ROW_TEMPVALUE, " ");
                            }
                        }

          //Humidity send

                   if(evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_humidity_bme680)
                                 {
                                   //Indications are enabled
                                   if(evt->data.evt_gatt_server_characteristic_status.client_config_flags == sl_bt_gatt_server_indication)
                                     {

                                       Humidity_Buffer=(uint32_t)ble_data.humidity;

                                       ble_data.connectionHandle = evt->data.evt_gatt_server_characteristic_status.connection;
                                       ble_data.indicationFlag = true;
                                       sc = sl_bt_gatt_server_send_indication(ble_data.connectionHandle,
                                                                              gattdb_humidity_bme680,
                                                                              4,
                                                                              (uint8_t*)&Humidity_Buffer);

                                     }
                                   else
                                     {
                                       ble_data.indicationFlag = false;
                   //                    gpioLed0SetOff();
                                       //LOG_INFO("Indications Disabled %d\n\r",0);
                                       // Clearing back the temperature on Screen after the indication have been displayed
//                                       displayPrintf(DISPLAY_ROW_TEMPVALUE, " ");
                                     }
                                 }

                   //heart rate
                   if(evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_heart_rate_bpm)
                                                    {
                                                      //Indications are enabled
                                                      if(evt->data.evt_gatt_server_characteristic_status.client_config_flags == sl_bt_gatt_server_indication)
                                                        {

                                                          Heart_Rate_Buffer=(uint32_t)ble_data.heartrate;

                                                          ble_data.connectionHandle = evt->data.evt_gatt_server_characteristic_status.connection;
                                                          ble_data.indicationFlag = true;
                                                          sc = sl_bt_gatt_server_send_indication(ble_data.connectionHandle,
                                                                                                 gattdb_heart_rate_bpm,
                                                                                                 4,
                                                                                                 (uint8_t*)&Heart_Rate_Buffer);

                                                        }
                                                      else
                                                        {
                                                          ble_data.indicationFlag = false;
                                      //                    gpioLed0SetOff();
                                                          //LOG_INFO("Indications Disabled %d\n\r",0);
                                                          // Clearing back the temperature on Screen after the indication have been displayed
                   //                                       displayPrintf(DISPLAY_ROW_TEMPVALUE, " ");
                                                        }
                                                    }

                   //spo2
                   if(evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_spo2_percentage)
                                                    {
                                                      //Indications are enabled
                                                      if(evt->data.evt_gatt_server_characteristic_status.client_config_flags == sl_bt_gatt_server_indication)
                                                        {

                                                          SPO2_Buffer=(uint32_t)ble_data.spo2;

                                                          ble_data.connectionHandle = evt->data.evt_gatt_server_characteristic_status.connection;
                                                          ble_data.indicationFlag = true;
                                                          sc = sl_bt_gatt_server_send_indication(ble_data.connectionHandle,
                                                                                                 gattdb_spo2_percentage,
                                                                                                 4,
                                                                                                 (uint8_t*)&SPO2_Buffer);

                                                        }
                                                      else
                                                        {
                                                          ble_data.indicationFlag = false;
                                      //                    gpioLed0SetOff();
                                                          //LOG_INFO("Indications Disabled %d\n\r",0);
                                                          // Clearing back the temperature on Screen after the indication have been displayed
                   //                                       displayPrintf(DISPLAY_ROW_TEMPVALUE, " ");
                                                        }
                                                    }
//
//      else if(evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_temperature_measurement &&
//                evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_confirmation)
//        {
//          ble_data.indication_in_flight = false;
//        }
//
//      //in case of Push button
//      if (evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_button_state &&
//          evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_client_config)
//               {
//
//                 // Indications for PB0 state have been turned on
//                 if(evt->data.evt_gatt_server_characteristic_status.client_config_flags == sl_bt_gatt_indication)
//                   {
//                       ble_data.pb0_indicationFlag = 1;
//                       //LED1 status update
//                       gpioLed1SetOn();
//                   }
//                 else if (evt->data.evt_gatt_server_characteristic_status.client_config_flags == sl_bt_gatt_disable)
//                   {
//                       ble_data.pb0_indicationFlag = 0;
//                       // off LED1
//                       gpioLed1SetOff();
//                   }
//               }
//      else if(evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_button_state &&
//          evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_confirmation){
//          ble_data.indication_in_flight = false;
//      }
      break;


    case sl_bt_evt_sm_confirm_bonding_id:

            // Accepting the bonding request from the client
//            sc = sl_bt_sm_bonding_confirm(ble_data.connectionHandle, 1);
//            if (sc != SL_STATUS_OK)
//              {
//                LOG_ERROR("sl_bt_sm_bonding_confirm() returned != 0 status=0x%04x\r\n", (unsigned int) sc);
//              }
            break;

    case sl_bt_evt_sm_bonding_failed_id:
            LOG_ERROR("Bonding request for connection failed \r\n");
            break;

         //occurs upon succesful sl_bt_sm_bonding_confirm() initiation
    case sl_bt_evt_sm_confirm_passkey_id:

            if(ble_data.connectionHandle == evt->data.evt_sm_confirm_passkey.connection)
              {
//                displayPrintf(DISPLAY_ROW_PASSKEY, "Passkey %d", evt->data.evt_sm_confirm_passkey.passkey);
//                displayPrintf(DISPLAY_ROW_ACTION, "Confirm with PB0");


                ble_data.bonding_state = 1;  //keeping track of bonding taking place
              } //now evtPB0Pressed is checked, whether it has occured or no
            break;

    case sl_bt_evt_system_external_signal_id:
//          if( evt->data.evt_system_external_signal.extsignals == evtPB0Pressed )
//            {
//                if(ble_data.bonding_state == 1) //confirms that device is in bonding process
//                  {
//                    sc = sl_bt_sm_passkey_confirm(ble_data.connectionHandle, true);
//                    if (sc != SL_STATUS_OK)
//                    {
//                      LOG_ERROR("sl_bt_sm_passkey_confirm() returned != 0 status=0x%04x\r\n", (unsigned int) sc);
//                    }
//
//                    ble_data.bonding_state = 2; //bonded
//                  }
//                else if(ble_data.bonding_state == 0)
//                  {
//                    //should initiate the bonding sequence
//                  }
//
//
//                //REading the PB0 state
//               // pb0_val = gpioPB0PadGet();
//
//                if(ble_data.pb0_val == 1)
//                  {
//                    displayPrintf(DISPLAY_ROW_9, "Button Released");
//                    PB0_buf[1] = 0; //sending 1 when when button is in release state
//                  }
//                else if(ble_data.pb0_val == 0)
//                  {
//                    displayPrintf(DISPLAY_ROW_9, "Button Pressed");
//                    PB0_buf[1] = 1; //sending 1 when in pressed state
//                  }
//                PB0_buf[0] = 0x00; //flags
//
//                sc = sl_bt_gatt_server_write_attribute_value(
//                                                       gattdb_button_state, // handle from gatt_db.h
//                                                       0,
//                                                       2,
//                                                       buf_ptr // in IEEE-11073 format
//                                                       );
//
//
//                if (sc != SL_STATUS_OK)
//                 {
//                   LOG_ERROR("sl_bt_gatt_server_write_attribute_value() returned != 0 status=0x%04x\r\n", (unsigned int) sc);
//                 }
//
//
//                //sending Indications
//                if((ble_data.connectionHandle != 0) && (ble_data.pb0_indicationFlag == 1) && (ble_data.bonding_state == 2) )
//                {
//                  if( ble_data.indication_in_flight == 1)
//                    {
//                       //queue indication
//                       if( !write_queue( gattdb_button_state, 2, buf_ptr ))
//                         {
//                           //successfuly queued
//                         }
//                       else
//                         {
//                           LOG_ERROR(" Buffer flooded \r\n");
//                         }
//
//                    }
//                  else
//                    {
//                      sc = sl_bt_gatt_server_send_indication(ble_data.connectionHandle,
//                                                                               gattdb_button_state,
//                                                                               2,
//                                                                               buf_ptr
//                                                                               );
//
//                      if (sc != SL_STATUS_OK)
//                        {
//                          LOG_ERROR("sl_bt_gatt_server_send_indication() returned != 0 status=0x%04x\r\n", (unsigned int) sc);
//                        }
//                      else
//                        {
//                          ble_data.indication_in_flight = 1; //high when indication is send
//                        }
//
//                    }
//
//                }
//
//
//
//            }
          break;
    case sl_bt_evt_sm_bonded_id:

//      if(ble_data.bonding_state == 2)
//        {
//            displayPrintf(DISPLAY_ROW_PASSKEY, " ");
//            displayPrintf(DISPLAY_ROW_ACTION, "Bonded");
//        }
          break;


#endif
     default:
       break;
  }

}


