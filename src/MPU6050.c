/*
 * MPU6050.c
 *
 *  Created on: Mar 24, 2023
 *      Author: jasan
 */



#include "MPU6050.h"
#include "i2c.h"
#include "math.h"
// Include logging specifically for this .c file
#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"


void MPU_6050_Init()
{
  I2C_TransferReturn_TypeDef ret;

  i2cInit();

  // Reset the device
  uint8_t data = MPU6050_PWR_MGMT_1;
  ret = i2c_write(MPU6050_I2C_ADDRESS, &data, 2);
  if(ret != i2cTransferDone){
      LOG_INFO("I2C ERROR: Reset the device. Error Code = %d\r\n",ret);
  }
  data = 0x80;
  ret = i2c_write(MPU6050_I2C_ADDRESS, &data, 2);
  if(ret != i2cTransferDone){
      LOG_INFO("I2C ERROR: Reset the device\r\n");
  }

  // Wait for some time. Need to figure the exact time
  for(int i=0; i<30000; i++){
      __asm("nop");
  }

  /*// Wake up the chip
  data = MPU6050_PWR_MGMT_1;
  ret = i2c_write(MPU6050_I2C_ADDRESS, &data, 1);
  if(ret != i2cTransferDone){
      LOG_INFO("I2C ERROR: Wake up the chip\r\n");
  }
  data = 0x00;
  ret = i2c_write(MPU6050_I2C_ADDRESS, &data, 1);
  if(ret != i2cTransferDone){
      LOG_INFO("I2C ERROR: Wake up the chip\r\n");
  }

  // Read Chip ID
  data = MPU6050_WHO_AM_I;
  ret = i2c_write(MPU6050_I2C_ADDRESS, &data, 1);
  if(ret != i2cTransferDone){
      LOG_INFO("I2C ERROR: Read Chip ID\r\n");
  }
  ret = i2c_read(MPU6050_I2C_ADDRESS, &data, 1);
  if(ret != i2cTransferDone){
      LOG_INFO("I2C ERROR: Read Chip ID\r\n");
  }

  LOG_INFO("Address = 0x%x\r\n",data);

  // Wait for some time.
  for(int i=0; i<30000; i++){
      __asm("nop");
  }*/

  /*// Set internal 20 MHz oscillator
  data = MPU6050_PWR_MGMT_1;
  ret = i2c_write(MPU6050_I2C_ADDRESS, &data, 1);
  if(ret != i2cTransferDone){
      LOG_INFO("I2C ERROR: Set internal 20 MHz oscillator\r\n");
  }
  data = 0x00;
  ret = i2c_write(MPU6050_I2C_ADDRESS, &data, 1);
  if(ret != i2cTransferDone){
      LOG_INFO("I2C ERROR: Set internal 20 MHz oscillator\r\n");
  }

  // Read PWR_MGMT_2 register
  data = MPU6050_PWR_MGMT_2;
  ret = i2c_write(MPU6050_I2C_ADDRESS, &data, 1);
  if(ret != i2cTransferDone){
      LOG_INFO("I2C ERROR: Read PWR_MGMT_2 register\r\n");
  }
  ret = i2c_read(MPU6050_I2C_ADDRESS, &data, 1);
  if(ret != i2cTransferDone){
      LOG_INFO("I2C ERROR: Read PWR_MGMT_2 register\r\n");
  }

  uint8_t pwr_mgmt_2_data = data;

  // Enable Acc/Gyro xyz
  data = MPU6050_PWR_MGMT_2;
  ret = i2c_write(MPU6050_I2C_ADDRESS, &data, 1);
  if(ret != i2cTransferDone){
      LOG_INFO("I2C ERROR: Enable Acc/Gyro xyz\r\n");
  }
  pwr_mgmt_2_data &= ~(0x38 | 0x07);
  ret = i2c_write(MPU6050_I2C_ADDRESS, &pwr_mgmt_2_data, 1);
  if(ret != i2cTransferDone){
      LOG_INFO("I2C ERROR: Enable Acc/Gyro xyz\r\n");
  }*/

  // Acc +- 2g
  /*data = MPU6050_ACCEL_CONFIG;
  ret = i2c_write(MPU6050_I2C_ADDRESS, &data, 1);
  if(ret != i2cTransferDone){
      LOG_INFO("I2C ERROR: Acc +- 2g\r\n");
  }
  data = (0x00 << 3);
  ret = i2c_write(MPU6050_I2C_ADDRESS, &data, 1);
  if(ret != i2cTransferDone){
      LOG_INFO("I2C ERROR: Acc +- 2g\r\n");
  }*/


  (void)ret;
}

void MPU6050_GetAccData(float * acc)
{
  uint8_t temp_data[6] = {0};
  int16_t temp_int[3] = {0};

  // Read the 6 bytes of accelerometer data
  uint8_t data = MPU6050_ACCEL_XOUT_H;


  for(int i=0; i<6; i++){
      i2c_write(MPU6050_I2C_ADDRESS, &data, 1);
      i2c_read(MPU6050_I2C_ADDRESS,&temp_data[i],1);

      data = data + 1;
  }

  // Each 2 bytes read correspond to 1 byte of integer data
  temp_int[0] = (int16_t)((temp_data[0]<< 8) | temp_data[1]);
  temp_int[1] = (int16_t)((temp_data[2]<< 8) | temp_data[3]);
  temp_int[2] = (int16_t)((temp_data[4]<< 8) | temp_data[5]);

  // Conversion to acceleration and getting only magnitude i.e the absoulute value
  acc[0] = (float)(abs(temp_int[0])) * 0.00059854736328125f;
  acc[1] = (float)(abs(temp_int[1])) * 0.00059854736328125f;
  acc[2] = (float)(abs(temp_int[2])) * 0.00059854736328125f;

}
