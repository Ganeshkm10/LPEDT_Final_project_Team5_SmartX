/*
 * BME680.c
 *  used Bosch's library files for BME setup: bme680_defs.h, bme680.c, bme680.h
 *  credits: Bosch Sensortec GmbH
 *  Created on: 03-Mar-2023
 *      Author: Ganesh KM
 */



#include <stdio.h>
#include <stdint.h>
#include "em_device.h"
#include "em_i2c.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "bme680.h"
#include "BME680_app.h"
#include "bme680_defs.h"
#include "ble.h"
#include "lcd.h"


#define BME680_OSR_SETTINGS    BME680_OS_1X
#define BME680_FILTER_SIZE     BME680_FILTER_SIZE_3
#define BME680_TEMP_OFFSET     0

I2C_TypeDef *i2c = I2C0;

void i2c_init_bme680(void)
{
  CMU_ClockEnable(cmuClock_I2C0, true);
  I2C_Init_TypeDef i2cInit = I2C_INIT_DEFAULT;
  i2cInit.freq = I2C_FREQ_STANDARD_MAX;
  I2C_Init(i2c, &i2cInit);
}

static void delayApprox(int delay)
{
  volatile int i;

  for (i = 0; i < delay; ) {
      i=i+1;
  }

} // delayApprox()


uint8_t buf[16];

// I2C write function for BME680 library
int8_t bme_i2c_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len)
{
    I2C_TransferSeq_TypeDef seq;
    I2C_TransferReturn_TypeDef status;

        buf[0] = reg_addr;

      for (uint16_t i = 1; i <len+1; i++)
        {
            buf[i] = reg_data[i-1];
        }

          seq.addr = dev_id << 1;
          seq.flags = I2C_FLAG_WRITE;
          seq.buf[0].data = buf;
          seq.buf[0].len = len+1;// *********

    status = I2C_TransferInit(I2C0, &seq);
    while (status == i2cTransferInProgress) {

         status = I2C_Transfer(I2C0);
//       printf("Error writing to BME680\n = status %d \n\r",status);
       if(status == i2cTransferDone)
         {
//           printf(" I2C transfer done \n\r");
         }

     }

return 0;

}
// I2C read function for BME680 library
int8_t bme_i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len)
{
I2C_TransferSeq_TypeDef seq;
I2C_TransferReturn_TypeDef status;

seq.addr = dev_id << 1;
seq.flags = I2C_FLAG_WRITE_READ;
seq.buf[0].data = &reg_addr;
seq.buf[0].len = 1;
seq.buf[1].data = reg_data;
seq.buf[1].len = len;

status = I2C_TransferInit(I2C0, &seq);
while (status == i2cTransferInProgress) {

     status = I2C_Transfer(I2C0);
//    printf("Error writing to BME680\n = status %d \n\r",status);
   if(status == i2cTransferDone)
     {
//        printf(" I2C read done \n\r");
     }

 }

return 0;
}

/*
 *   Function to get temperature,pressure,humdity,gas resistance values
 *   argument : none
 *   Return   : null
 */
void readTemperature(void) {
    struct bme680_dev bme;
    int result = 1;
//    bme680_init(&bme);
//    printf("bme680 init completed \n\r");

    ble_data_struct_t* BleDataPtr = getBleDataPtr();

    bme.write = bme_i2c_write;
    bme.read = bme_i2c_read;
    bme.dev_id = BME680_I2C_ADDRESS;
    bme.intf = BME680_I2C_INTF;
    bme.amb_temp = 25;
    bme.delay_ms = 3;

    result = bme680_init(&bme);
    printf("bme680 init completed = %d \n\r",result);

    bme.tph_sett.os_hum = BME680_OS_2X;
    bme.tph_sett.os_pres = BME680_OS_4X;
    bme.tph_sett.os_temp = BME680_OS_8X;
    bme.tph_sett.filter = BME680_FILTER_SIZE_3;

    /* Set the remaining gas sensor settings and link the heating profile */
    bme.gas_sett.run_gas = BME680_ENABLE_GAS_MEAS;
    /* Create a ramp heat waveform in 3 steps */
    bme.gas_sett.heatr_temp = 320; /* degree Celsius */
    bme.gas_sett.heatr_dur = 150; /* milliseconds */


    /* Select the power mode */
    /* Must be set before writing the sensor configuration */
    bme.power_mode = BME680_FORCED_MODE;
//  bme.power_mode = BME680_FORCED_MODE;


    uint8_t set_required_settings = BME680_OST_SEL | BME680_OSP_SEL | BME680_OSH_SEL | BME680_FILTER_SEL| BME680_GAS_SENSOR_SEL;

    result = bme680_set_sensor_settings(set_required_settings, &bme);
    printf("setting is okay = %d \n\r",result);

    result = bme680_set_sensor_mode(&bme);
    printf("sensor mode = %d \n\r",result);

    uint16_t meas_period;
     bme680_get_profile_dur(&meas_period, &bme);
     printf(" meas_period = %d \n\r",meas_period);


 struct bme680_field_data data;
//while(1)
   {
     delayApprox(640500);
         printf("entering get sensor data\n\r");
          result = bme680_get_sensor_data(&data,&bme);
          if(result!=BME680_OK)
            {
              printf("error in getting data \n\r");

            }
          printf("exiting get sensor data \n\r");

          printf("Temperature: %0.2f C\n\r", data.temperature / 100.0);
          printf("Pressure: %0.2f hPa\n\r", data.pressure / 100.0);
          printf("Humidity: %0.2f %%RH\n\r", data.humidity / 1000.0);
          printf("Gas resistance: %0.2f kOhms\n\r", data.gas_resistance / 1000.0);

          set_normal_font();
          displayPrintf( DISPLAY_ROW_8, "Temperature: %0.2f C", data.temperature / 100.0 );
          displayPrintf(  DISPLAY_ROW_9, "Pressure: %0.2f hPa", data.pressure / 100.0 );
          displayPrintf(   DISPLAY_ROW_10, "Humidity: %0.2f %%RH",data.humidity / 1000.0 );
          displayPrintf(   DISPLAY_ROW_11, "Gas Val: %0.2f kOhms", data.gas_resistance / 1000.0);



          BleDataPtr->ambienttemperature =  (data.temperature);
          BleDataPtr->pressure = data.pressure / 10.0;
          BleDataPtr->humidity = data.humidity / 10.0;
          BleDataPtr->gasvalue = data.gas_resistance / 1000.0;


          /***Debug code ***/
//          if(bme.power_mode == BME680_FORCED_MODE)
//            {
//              result = bme680_set_sensor_mode(&bme);
//                       printf("sensor mode = %d \n\r",result);
//            }


   }




}
