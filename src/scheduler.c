/*
 * scheduler.c
 *
 *  Created on: Sep 13, 2022
 *      Author: Jasan Singh
 */

#include <src/HR_SPO2_arduino.h>
#include <src/MAX30101.h>
#include "gatt_db.h" // GATT database

#include "scheduler.h"
#include "em_core.h"
#include "i2c.h"
#include "gpio.h"
#include "ble.h"
#include "ble_device_type.h"
#include "app_assert.h"
#include "circular_buffer.h"
#include "RTC.h"
#include "MPU6050.h"

#include "lcdMaps.h"
#include "BME680_app.h"

#include <stdint.h>


#define CMD_DATA_TO_READ 0xF3
#define MULTIPLIER 175.72
#define MAX_RES 65536
#define CONSTANT 46.85

uint8_t PB0_flag = false;

// Include logging specifically for this .c file
#define INCLUDE_LOG_DEBUG 1
#include "log.h"


uint32_t irBuffer[100];
uint32_t redBuffer[100];
int32_t bufferLength;
int32_t spo2;
int8_t validSPO2;
int32_t heartRate;
int8_t validHeartRate;


int32_t FinalSPO2;
int32_t FinalheartRate;

#define NO_OF_SAMPLES 275
int32_t hr_buffer[NO_OF_SAMPLES];
int32_t spo2_buffer[NO_OF_SAMPLES];
uint8_t pulseLED = 11;
uint8_t readLED = 13;

#if (DEVICE_IS_BLE_SERVER == 0)
#define CHARACTERISTIC_HANDLE_INVALID ((uint16_t)0xFFFFu)
#define SERVICE_HANDLE_INVALID        ((uint32_t)0xFFFFFFFFu)

// Health Thermometer service UUID defined by Bluetooth SIG
static const uint8_t thermo_service[2] = { 0x09, 0x18 };
// Temperature Measurement characteristic UUID defined by Bluetooth SIG
static const uint8_t thermo_char[2] = { 0x1c, 0x2a };

typedef enum {
                Idle, DiscoverServices, DiscoverCharacterstics, EnableIndications, close
              } state_t;

#endif
/*
 *  Variable tracks the event happening see enum Masks for more info
 */
//uint32_t Event;

/*
 * Function calls BT external signal and passes the UnderFlow event occurred
 * arguments : null
 * return    : null
 */
void schedulerSetEventLETIMER_UF()
{
  CORE_DECLARE_IRQ_STATE;
  //enter the critical section
  CORE_ENTER_CRITICAL();

  //RMW
  //Event |= evtLETIMER0_UF;

  //giving a call to ble every 3sec
  sl_bt_external_signal(evtLETIMER0_UF);
  //exit critical section
  CORE_EXIT_CRITICAL();

}

/*
 * Function calls BT external signal and passes the COMP1 event occurred
 * arguments : null
 * return    : null
 */
void schedulerSetEventLETIMER_COMP1()
{
  CORE_DECLARE_IRQ_STATE;

  //enter the critical section
  CORE_ENTER_CRITICAL();

  sl_bt_external_signal(evtLETIMER0_COMP1);

  //exit critical section
  CORE_EXIT_CRITICAL();
}

/*
 * Function calls BT external signal and passes the I2C transfer event occurred
 * arguments : null
 * return    : null
 */
void schedulerSetEventI2CTransferComplete()
{
  CORE_DECLARE_IRQ_STATE;

  //enter the critical section
  CORE_ENTER_CRITICAL();

  //Event |= evtI2CTransferComplete;
  sl_bt_external_signal(evtI2CTransferComplete);
  //exit critical section
  CORE_EXIT_CRITICAL();
}

#if DEVICE_IS_BLE_SERVER
void schedulerSetEventPB0Int()
{
  CORE_DECLARE_IRQ_STATE;

  //enter the critical section
  CORE_ENTER_CRITICAL();

  //updating the status
  ble_data_struct_t* BleDataPtr = getBleDataPtr();

  BleDataPtr->pb0_val = gpioPB0PadGet();

  //Event |= evtI2CTransferComplete;
  sl_bt_external_signal(evtPB0Pressed);
  //exit critical section
  CORE_EXIT_CRITICAL();
}

void schedulerSetEventPB1Int()
{
  CORE_DECLARE_IRQ_STATE;

  //enter the critical section
  CORE_ENTER_CRITICAL();

  //updating the status
  ble_data_struct_t* BleDataPtr = getBleDataPtr();

  BleDataPtr->pb1_val = gpioPB0PadGet();

  //Event |= evtI2CTransferComplete;
  sl_bt_external_signal(evtPB1Pressed);
  //exit critical section
  CORE_EXIT_CRITICAL();
}
#endif

/*
 *  Gets the next event in the scheduler
 *  Returns the respective event enum value
 */
//uint32_t getNextEvent()
//{
//  //uint32_t TheEvent;
//  //TheEvent = Event;
//  CORE_DECLARE_IRQ_STATE;
//  uint32_t i =1;
//
//  do{
//      if( (i & Event) == i)
//      {
//          //enter the critical section
//          CORE_ENTER_CRITICAL();
//          Event &= (~i); //clearing the bit as the event is being served
//          //exit critical section
//          CORE_EXIT_CRITICAL();
//          return i;
//      }
//      else
//        i = i<<1;
//  }while(i != 0);
//
//  return 0;
//}

/*
 *   Function implement statemachine for temperature measurement
 *   from Si7021 based on the BL API messages
 *   argument : Takes pointer to BT API messagees
 *   Return   : null
 */
#if DEVICE_IS_BLE_SERVER
void temperature_state_machine(sl_bt_msg_t *evt)
{
    typedef enum uint32_t {
                Idle, TempRequest, StartMeasure, Measure, Report
              } state_t;

    state_t currState;
    static state_t nextState = Idle;
    uint32_t temperatureData=0;

    ble_data_struct_t* BleDataPtr = getBleDataPtr();

    sl_status_t sc;

    uint8_t htm_temperature_buf[5];
    uint8_t *ptr = htm_temperature_buf;

    uint32_t temperatureFLT;
    uint8_t flags;

    if(SL_BT_MSG_ID(evt->header) == sl_bt_evt_system_external_signal_id)
    {
        if(BleDataPtr->indicationFlag && BleDataPtr->connectionOpen)
          {
            currState =nextState;
                     switch(currState)
                     {
                       case Idle:
                           nextState = Idle;
                           // DOS: if we have a connection open and our UF event, then start
                           // DOS: a temp measurement cycle. May want to consider adding another if term
                           // DOS: for whether indications have been enabled for the HTM characteristic
                           if( (BleDataPtr->connectionOpen) &&
                               (evt->data.evt_system_external_signal.extsignals == evtLETIMER0_UF) )
                             {
                               // power on si7021 and call timewaitUs()
                               gpioSI7021On();
                               timerWaitUs_irq(80*MSECONDS);
                               nextState = TempRequest;
                             }
                               break;

                       case TempRequest:
                         nextState = TempRequest;
                         if( evt->data.evt_system_external_signal.extsignals == evtLETIMER0_COMP1)
                           {
                             // means timer expired
                             timerExpired();//Diasabling COMP1 interrupt
                             I2CInit();
                             I2CWrite(); //initializes I2C transfer
                             sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);//power requirement to em1
                             nextState = StartMeasure;
                           }
                             break;

                       case StartMeasure:
                         nextState = StartMeasure;
                         if( evt->data.evt_system_external_signal.extsignals == evtI2CTransferComplete)
                           {
                             NVIC_DisableIRQ(I2C0_IRQn);
                             sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
                             timerWaitUs_irq(11*MSECONDS);
                             nextState = Measure;
                           }
                           break;

                       case Measure:
                         nextState = Measure;
                         if( evt->data.evt_system_external_signal.extsignals == evtLETIMER0_COMP1)
                           {
                             // means timer expired
                             timerExpired();//Diasabling COMP1 interrupt
                             sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);//power requirement to em1
                             I2CRead();   //Initiates I2c read transfers
                             nextState = Report;
                           }
                         break;

                       case Report:
                         nextState = Report;
                         if( evt->data.evt_system_external_signal.extsignals == evtI2CTransferComplete)
                           {
                             NVIC_DisableIRQ(I2C0_IRQn);
                             sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
                             //gpioSI7021Off();
                             temperatureData = I2C_temperatureReport();
                             //Updating temperature on LCD
                             displayPrintf(DISPLAY_ROW_TEMPVALUE, "Temp=%d", (int)temperatureData);
                             flags = 0x00;
                             UINT8_TO_BITSTREAM(ptr, flags);

                             temperatureFLT  = UINT32_TO_FLOAT(temperatureData*1000, -3);
                             UINT32_TO_BITSTREAM(ptr, temperatureFLT);

                             // DOS - You need to check that the Client has enabled sending indications
                             // for this characteristic!
                            // if()
                             if( BleDataPtr->indication_in_flight == 1)
                               {
                                  //queue indication
                                  if( !write_queue( BleDataPtr->connectionHandle, 5, htm_temperature_buf ))
                                    {
                                      //successfuly queued
                                    }
                                  else
                                    {
                                      LOG_ERROR(" Buffer flooded \r\n");
                                    }

                               }
                             else
                               {
                                       //sending this temperature to Ble through indications
                                       sc = sl_bt_gatt_server_send_indication(
                                                 BleDataPtr->connectionHandle,
                                                 gattdb_temperature_measurement, // handle from gatt_db.h
                                                 sizeof(htm_temperature_buf),
                                                 htm_temperature_buf // in IEEE-11073 format
                                                 );

                                       if (sc != SL_STATUS_OK)
                                         {
                                           LOG_ERROR("sl_bt_gatt_server_send_indication() returned != 0 status=0x%04x\r\n", (unsigned int) sc);
                                         }
                                       else
                                         {
                                           BleDataPtr->indication_in_flight = 1; //high when indication is send
                                         }
                               }

                             sc = sl_bt_gatt_server_write_attribute_value(
                                       gattdb_temperature_measurement, // handle from gatt_db.h
                                       0,
                                       5,
                                       ptr // in IEEE-11073 format
                                       );

                             //app_assert_status(sc);


                             nextState = Idle;
                           }
                         break;

                       default:
                         break;

                     }
          }
         /* else {
              //graceful exit conditions, when connection gets down in betweeen any of the states
              // means timer expired
              timerExpired();//Diasabling COMP1 interrupt
              NVIC_DisableIRQ(I2C0_IRQn);
              sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
              gpioSI7021Off();
              nextState = Idle;
          }*/

    }

}
int hr_spo2_enable_flag;

/*
 *   Function implement statemachine for smartx features
 *   argument : Takes pointer to BT API messagees
 *   Return   : null
 */
void smartx_features(sl_bt_msg_t *evt)
{
            static float acc[3];
            static int display_flag;
//            LOG_INFO("Calling smartx_features statemachine");



            //pointer to the structure
            ble_data_struct_t* BleDataPtr = getBleDataPtr();

            volatile uint32_t Event_to_consider = evt->data.evt_system_external_signal.extsignals;

            static features_t CURRENT_FEATURE = state_Sleep;
            static features_t NEXT_FEATURE = state_Sleep;

            CURRENT_FEATURE = NEXT_FEATURE;

            switch(CURRENT_FEATURE)
            {
              case state_Sleep:
                {

                   sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);

                   if((Event_to_consider == evtPB0Pressed)||(Event_to_consider == evtPB1Pressed))
                     {
                       displayInit();
                       NEXT_FEATURE = state_TimeDisplay;
                       LOG_INFO(" Next State: TimeDisplay \r");
                     }


                    break;
                 }
              case state_TimeDisplay:

//                  displayPrintf( DISPLAY_ROW_CLIENTADDR, "Time" );
//                  displayPrintf(  DISPLAY_ROW_PASSKEY, "Date" );
//                  displayPrintf(   DISPLAY_ROW_ASSIGNMENT, "SmartX" );
//                  LOG_INFO(" entering state_TimeDisplay ");

                  //To do           /********** RTC Code ****************/

                if(display_flag==0)
                  {
                    draw_custom_graphics(home,0);
                    display_flag = 1;
                  }


                  RTC_RV3028_GetData();
//                  timerWaitUs_irq(200000);
                  // read the accelerometer values
                  MPU6050_GetAccData(acc);
//                  timerWaitUs_irq(200000);
                  //printf("X=%d Y=%d Z=%d \r\n",acc[0],acc[1],acc[2]);


                   LOG_INFO("X=%0.2f Y=%0.2f Z=%0.2f \r\n",acc[0],acc[1],acc[2]);

                   set_normal_font();
                   displayPrintf(DISPLAY_ROW_ASSIGNMENT, "X=%0.2f Y=%0.2f Z=%0.2f",acc[0],acc[1],acc[2] );

                   if(Event_to_consider == evtPB1Pressed)
                     //
                     {

                       LOG_INFO(" next state: BME680 \r");
                       display_flag = 0;
                       sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
                       NEXT_FEATURE  = state_T_H_P_AQ;
                       //schedulerClearEvent();
                     }
                  break;


              case state_T_H_P_AQ:
                {
//                  displayPrintf(DISPLAY_ROW_CLIENTADDR, "Temp humidity" );
//                  displayPrintf(  DISPLAY_ROW_PASSKEY, "press PB0" );
//                  displayPrintf(   DISPLAY_ROW_ASSIGNMENT, "SmartX" );
                  if(display_flag==0)
                    {
                      draw_custom_graphics(bme680,0);
                      display_flag = 1;
                    }

//                  LOG_INFO(" entering state_T_H_P_AQ ");
                  if(Event_to_consider == evtPB0Pressed)
                    {
                      //LOG_INFO(" entering state wait for power on ");
                      sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);

                      /********** T_H_P_AQ code: BME 680 ****************/
                      readTemperature();//Function name is decieving, it is reading temperature,pressure, humdity.

                    }
                  if(Event_to_consider == evtPB1Pressed)
                    {
                      LOG_INFO(" next state: HR_SPO2 \r");
                      display_flag = 0;
                      sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
                      NEXT_FEATURE  = state_HR_SPO2;
                    }

                 break;
                }

              case state_HR_SPO2:
                {
//                  displayPrintf( DISPLAY_ROW_CLIENTADDR, "HR_SP02" );
//                  displayPrintf(  DISPLAY_ROW_PASSKEY, "press PB0" );
//                  displayPrintf(   DISPLAY_ROW_ASSIGNMENT, "SmartX" );

                  if(display_flag==0)
                    {
                      draw_custom_graphics(heart,0);
                      display_flag = 1;
                    }
                  if((Event_to_consider == evtPB0Pressed)||(hr_spo2_enable_flag))
                     {

//                      HR_init();
                      HR_SPO2_init();
                      hr_spo2_enable_flag = 1;
                        sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);

              /********** HR_SPO2 code ****************/
                        HR_SPO2_measure_state_machine(evt);

                     }
                   if(Event_to_consider == evtPB1Pressed)
                      {
                       LOG_INFO(" next state: BLE_Connect \r");
                       display_flag = 0;
                       turnoff_led();
                       NEXT_FEATURE  = state_BLE_Connect;
                      }
                   break;
                    }

              case state_BLE_Connect:
                {

//                  displayPrintf( DISPLAY_ROW_CLIENTADDR, "BLE_Connect" );
//                  displayPrintf(  DISPLAY_ROW_PASSKEY, "press PB0" );
//                  displayPrintf(   DISPLAY_ROW_ASSIGNMENT, "SmartX" );


                  if(display_flag==0)
                   {
                     draw_custom_graphics(bleConnect,0);
                     display_flag = 1;
                   }
                  sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);
                  if(Event_to_consider == evtPB0Pressed || BleDataPtr->connectionOpen ==1)
                    {
                     if(BleDataPtr->connectionOpen ==0)
                       {
                         set_normal_font();
                         displayPrintf(DISPLAY_ROW_ACTION, "Advertising");
                       }
                     if(BleDataPtr->connectionOpen ==1)
                        {
                           set_normal_font();
                          displayPrintf(DISPLAY_ROW_ACTION, "Connected");
                        }

                    }

                  if(Event_to_consider == evtPB1Pressed)
                    {
                      display_flag = 0;
                      displayInit();
                      NEXT_FEATURE  = state_Sleep;
                    }

                   break;
                }


              default:
                {
                  LOG_WARN("Unknown state in State machine");//Should not reach here
                }
            }





 }

/*
 *   Function implement statemachine for HR_SPO2 Measurements
 *   argument : Takes pointer to BT API messages
 *   Return   : null
 */

void HR_SPO2_measure_state_machine(sl_bt_msg_t *event)
{

  ble_data_struct_t *ble_data_ptr;
  ble_data_ptr = getBleDataPtr();
  static volatile uint8_t count_hr_spo2 = 0;

  switch (SL_BT_MSG_ID(event->header))
  {
    case sl_bt_evt_system_external_signal_id:
      if(event->data.evt_system_external_signal.extsignals == evtPB0Pressed )
        {
          if(ble_data_ptr->pb0_val == 0)
            PB0_flag = true;
          LOG_INFO("spo2 state machine: PB0 flag \r");
        }
      break;

    case sl_bt_evt_system_soft_timer_id:


          if(PB0_flag)
            {
              bufferLength = 100; //buffer length of 100 stores 4 seconds of samples running at 25sps

                //read the first 100 samples, and determine the signal range
                for (uint8_t i = 0 ; i < bufferLength ; i++)
                {
                  while (MAX30105_available() == false) //do we have new data?
                    check(); //Check the sensor for new data

                  redBuffer[i] = MAX30105_getRed();
                  irBuffer[i] = MAX30105_getIR();
                  MAX30105_nextSample(); //We're finished with this sample so move to next sample
                  LOG_INFO("red val = %d\r", redBuffer[i]);

                  LOG_INFO("ir val = %d\r", irBuffer[i]);

                }

                //calculate heart rate and SpO2 after first 100 samples (first 4 seconds of samples)
                maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

                //Continuously taking samples from MAX30102.  Heart rate and SpO2 are calculated every 1 second
                while (count_hr_spo2 <= 30 && PB0_flag == true)
                {
//                    if(event->data.evt_system_soft_timer.handle == SOFT_TIMER_PULSE_SENSOR)
//                      {
                        count_hr_spo2++;
//
//                      }

                  //dumping the first 25 sets of samples in the memory and shift the last 75 sets of samples to the top
                  for (uint8_t i = 25; i < 100; i++)
                  {
                    redBuffer[i - 25] = redBuffer[i];
                    irBuffer[i - 25] = irBuffer[i];
                  }

                  //take 25 sets of samples before calculating the heart rate.
                  for (uint8_t i = 75; i < 100; i++)
                  {
                    while (MAX30105_available() == false) //do we have new data?
                      check(); //Check the sensor for new data

                    redBuffer[i] = MAX30105_getRed();
                    irBuffer[i] = MAX30105_getIR();
                    MAX30105_nextSample(); //We're finished with this sample so move to next sample


                    hr_buffer[i] = heartRate;
                    spo2_buffer[i] = spo2;
                    i++;
                    FinalSPO2 = spo2;
                    FinalheartRate = heartRate;

                    LOG_INFO("HR = %d \r", heartRate);
                    LOG_INFO("SPO2 = %d \r", spo2);
                  }

                  //After gathering 25 new samples recalculate HR and SP02
                  maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

                }

                LOG_INFO("final HR = %d \r", FinalheartRate);
                //
                //                    LOG_INFO("HRvalid = %d\r", validHeartRate);
                //
                LOG_INFO("final SPO2 = %d \r",FinalSPO2);
                displayPrintf(DISPLAY_ROW_10,"Heart rate = %d",FinalheartRate);
                displayPrintf(DISPLAY_ROW_11,"SPO2 = %d",FinalSPO2);
                PB0_flag = false;
                if(count_hr_spo2 >= 30)
                  {
                    count_hr_spo2 = 0;
                    hr_spo2_enable_flag = 0;
                  }


            }
  }



}

#endif


#if (DEVICE_IS_BLE_SERVER==0)


// sequence through service and characteristic discovery
/*
 *  handles client side discoverya nd indications event
 *  Returns Null
 */
void discovery_state_machine(sl_bt_msg_t *evt)
{


      static state_t currState = Idle;
      static state_t nextState = Idle;
      uint32_t event = SL_BT_MSG_ID(evt->header);
      ble_data_struct_t* BleDataPtr = getBleDataPtr();

      sl_status_t sc;




       switch(currState)
       {
         case Idle:
             nextState = Idle;
             if(event == sl_bt_evt_connection_opened_id)
               {
                 sc = sl_bt_gatt_discover_primary_services_by_uuid(BleDataPtr->server_connectionHandle,
                                                                        sizeof(thermo_service),
                                                                        (const uint8_t*)thermo_service);
                 if (sc != SL_STATUS_OK)
                 {
                    LOG_ERROR(" sl_bt_gatt_discover_primary_services_by_uuid() returned != 0 status=0x%04x", (unsigned int) sc);
                 }
                 nextState = DiscoverServices;
               }
             break;

         case DiscoverServices:
            nextState = DiscoverServices;

           if(event == sl_bt_evt_gatt_procedure_completed_id && BleDataPtr->serviceHandle != SERVICE_HANDLE_INVALID )
           {
              sc = sl_bt_gatt_discover_characteristics_by_uuid(BleDataPtr->server_connectionHandle,
                                                                 BleDataPtr->serviceHandle,
                                                                 sizeof(thermo_char),
                                                                 (const uint8_t*)thermo_char);
              if (sc != SL_STATUS_OK)
              {
                LOG_ERROR(" sl_bt_gatt_discover_primary_services_by_uuid() returned != 0 status=0x%04x", (unsigned int) sc);
              }
              nextState = DiscoverCharacterstics;
           }
              break;

         case DiscoverCharacterstics:
           nextState = DiscoverCharacterstics;
           if(event == sl_bt_evt_gatt_procedure_completed_id && BleDataPtr->characteristicHandle != CHARACTERISTIC_HANDLE_INVALID)
           {
               // Server found proceed to stop scanning
               sc = sl_bt_scanner_stop();
               if (sc != SL_STATUS_OK)
               {
                   LOG_ERROR("sl_bt_scanner_stop() returned != 0 status=0x%04x", (unsigned int) sc);
               }
             sc = sl_bt_gatt_set_characteristic_notification(BleDataPtr->server_connectionHandle,
                                                                  BleDataPtr->characteristicHandle,
                                                                  sl_bt_gatt_indication);
             if (sc != SL_STATUS_OK)
             {
              LOG_ERROR(" sl_bt_gatt_set_characteristic_notification() returned != 0 status=0x%04x", (unsigned int) sc);
             }
             nextState = EnableIndications;
           }
             break;

         case EnableIndications:
           nextState = EnableIndications;
           if(event == sl_bt_evt_gatt_procedure_completed_id)
           {
               nextState = close;
           }
           break;

         case close:
           nextState = close;
           if(event == sl_bt_evt_connection_closed_id)
           {
              nextState = Idle;
            }
           break;

         default:
           break;

       }

       currState =nextState;
}
#endif
