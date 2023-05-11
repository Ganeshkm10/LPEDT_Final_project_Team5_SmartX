/*
 * i2c.c
 *
 *  Created on: Sep 15, 2022
 *      Author: Jasan Singh
 */

#include "i2c.h"

// Include logging specifically for this .c file
#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

// I2C and temperature sensor related data
I2C_TransferReturn_TypeDef transferStatus; // make this global for IRQs in A4
I2C_TransferSeq_TypeDef transferSequence; // this one can be local
uint8_t cmd_data; // make this global for IRQs in A4
uint16_t read_data; // make this global for IRQs in A4

#define BYTE_SWAP(x)                                  (((x << 8) & 0xFF00) | ((x >> 8) & 0x00FF))
#define CONVERT_RAW_TEMPERATURE_DATA_TO_CELSIUS(x)    (((x * 175.72) / 65536) - 46.85)





void i2cInit()
{
  I2CSPM_Init_TypeDef I2C_Config = {
      .port            = I2C0,
      .sclPort         = gpioPortC,
      .sclPin          = 10,
      .sdaPort         = gpioPortC,
      .sdaPin          = 11,
      .portLocationScl = 14,
      .portLocationSda = 16,
      .i2cRefFreq      = 0,
      .i2cMaxFreq      = I2C_FREQ_STANDARD_MAX,
      .i2cClhr         = i2cClockHLRStandard
  };

  I2CSPM_Init(&I2C_Config);
  //uint32_t i2c_bus_frequency = I2C_BusFreqGet(I2C0);
}

bool i2c_write(uint8_t device_address, uint8_t *data, uint8_t number_of_bytes)
{
  I2C_TransferSeq_TypeDef seq;
  I2C_TransferReturn_TypeDef ret;

  // Set the transaction to be write operation
  seq.addr = device_address << 1;
  seq.flags = I2C_FLAG_WRITE;

  seq.buf[0].data = data;
  seq.buf[0].len  = number_of_bytes;

  ret = I2CSPM_Transfer(I2C0, &seq);

  if(ret != i2cTransferDone){
      printf(" i2c transfer failed\n");
      return true; // Error has occured
  }

  return false; // No error
}

bool i2c_read(uint8_t device_address, uint8_t *data, uint8_t number_of_bytes)
{
  I2C_TransferSeq_TypeDef seq;
  I2C_TransferReturn_TypeDef ret;

  // Set the transaction to be write operation
  seq.addr = device_address << 1;
  seq.flags = I2C_FLAG_READ;

  seq.buf[0].data = data;
  seq.buf[0].len  = number_of_bytes;

  ret = I2CSPM_Transfer(I2C0, &seq);

  if(ret != i2cTransferDone){
      return true; // Error has occured
  }

  return false; // No error

}


//#define NoHoldMaster (0xF3)
//#define SI7021_DEVICE_ADDR (0x40)
//
//
//I2C_TransferReturn_TypeDef transferStatus;
//I2C_TransferSeq_TypeDef    transferSequence;
//uint8_t                    cmd_data;
//
///*
// *  Function initializes the I2C module with I2C_FREQ_STANDARD_MAX frequency
// */
//void I2CInit ( )
//{
//  I2CSPM_Init_TypeDef I2C_Config = {
//      .port            = I2C0,
//      .sclPort         = gpioPortC,
//      .sclPin          = 10,
//      .sdaPort         = gpioPortC,
//      .sdaPin          = 11,
//      .portLocationScl = 14,
//      .portLocationSda = 16,
//      .i2cRefFreq      = 0,
//      .i2cMaxFreq      = I2C_FREQ_STANDARD_MAX,
//      .i2cClhr         = i2cClockHLRStandard
//
//  };
//
//  I2CSPM_Init(&I2C_Config);
//
//  //uint32_t i2c_bus_frequency = I2C_BusFreqGet(I2C0);
//
//
//}
//
///*
// *  Function helps sending I2C command
// *  arguments: Takes uint8_t pointer to the data tp send or receive
// *             @RWFlag   takes the read or write flag
// *             @datasize takes the number of bytes of data to transfered
// */
//uint8_t send_I2C_command(uint8_t * data, uint8_t RWFlag, uint8_t dataSize)
//{
//  transferSequence.addr        = SI7021_DEVICE_ADDR << 1 ;
//  transferSequence.flags       = RWFlag;
//  transferSequence.buf[0].data = data;
//  transferSequence.buf[0].len  = dataSize;
//
//
//  //enable nvic for i2c
//  NVIC_EnableIRQ(I2C0_IRQn);
//
//  transferStatus = I2C_TransferInit (I2C0, &transferSequence); // non blocking call, just initailizes the transfer call and returns
//  if( transferStatus < 0) {
//      LOG_ERROR ( " I2C_TransferInit: cmd failed \r");
//      return 1;
//  }
//  else
//   return 0;
//}
//
//
///*
// *  Function does write I2C transfer initialization with No Hold Master
// *  Returns 0 on success and 1 on failure
// */
//uint8_t I2CWrite()
//{
//    uint8_t ret;
//
//    cmd_data                     = NoHoldMaster;
//
//    ret = send_I2C_command (&cmd_data, I2C_FLAG_WRITE, sizeof(cmd_data));
//
//    return ret;
//}
//
//
///*
// *  Function initializes I2C transfer to get the 16bit data
// *  returns 1 on failure else 0 on success
// */
//uint16_t I2CRead()
//{
//    uint16_t ret;
//
//
//    ret = send_I2C_command (&cmd_data, I2C_FLAG_READ, sizeof(ret));
//
//    return ret;
//}
//
///*
// *  Function logs the converted Temperature transferred from I2C read
// *  and converts into celsius
// */
//uint32_t I2C_temperatureReport()
//{
//  int Celsius;
//  uint16_t TemperatureHex = (uint16_t)cmd_data;
//
//  // Arrange MSB and LSB into variable
//  TemperatureHex = (( TemperatureHex << 8) | (TemperatureHex >> 8));
//  // Convert received value to degrees Celsius and log
//  Celsius = (int) ((175.72 * TemperatureHex)/65536 - 46.85);
//  //LOG_INFO("Temperature ( 'C) = %d C\n\r", Celsius);
//
//  return (uint32_t)Celsius;
//}
//


void max30101_i2c_write(uint8_t reg)
{
//  uint8_t read_data;
  transferSequence.flags = I2C_FLAG_WRITE, // Write command
  transferSequence.addr = (MAX30105_ADDRESS<<1), // Slave address needs to be left shift by one bit
  transferSequence.buf[0].data = &reg, // Passing the pointer that has the command data stored
  transferSequence.buf[0].len = sizeof(reg); // Length of the command data

//  i2c_init();
  I2C_TransferReturn_TypeDef trans_ret = I2CSPM_Transfer(I2C0, &transferSequence);

  // Checking if the transfer is done or no.
  if(trans_ret != i2cTransferDone)
      {
        LOG_ERROR("I2C Write error: %d", trans_ret); // If transfer is not done then we will log error message
//        createEventSystemError();
      }

//  return read_data;
}
uint8_t max30101_i2c_read()
{

  uint8_t read_data;
  transferSequence.flags = I2C_FLAG_READ, // Write command
  transferSequence.addr = (MAX30105_ADDRESS<<1), // Slave address needs to be left shift by one bit
  transferSequence.buf[0].data = &read_data, // Passing the pointer that has the command data stored
  transferSequence.buf[0].len = sizeof(read_data); // Length of the command data

//  i2c_init();
  I2C_TransferReturn_TypeDef trans_ret = I2CSPM_Transfer(I2C0, &transferSequence);

  // Checking if the transfer is done or no.
  if(trans_ret != i2cTransferDone)
      {
        LOG_ERROR("I2C Write error: %d", trans_ret); // If transfer is not done then we will log error message
//        createEventSystemError();
      }

  return read_data;

//  I2C_TransferSeq_TypeDef transferSequence = {
//      .addr = SI7021_ADDRESS << 1,
//      .flags = I2C_FLAG_READ,
//      .buf[0].data = (uint8_t*)(&read_data),
//      .buf[0].len = sizeof(read_data)
//  };
//
//  transferStatus = I2CSPM_Transfer(I2C0, &transferSequence);
//  if (transferStatus != i2cTransferDone) {
//      LOG_ERROR("I2CSPM Transfer: I2C Read failed with error = %d\n\r", transferStatus);
//  }
//
//  uint16_t new_data =  (read_data << 8) | ((read_data >> 8) & 0x00FF);
//
//  float temp = ( (175.72 * new_data) / 65536 ) - 46.85;
//
//  LOG_INFO("Temperature = %f\n\r", temp);

}

uint8_t max30101_i2c_burst_read(uint8_t reg, uint8_t *data, unsigned int len)
{

//  uint8_t read_data;
//  transferSequence.flags = I2C_FLAG_READ, // Write command
//  transferSequence.addr = (MAX30105_ADDRESS<<1), // Slave address needs to be left shift by one bit
//  transferSequence.buf[0].data = &read_data, // Passing the pointer that has the command data stored
//  transferSequence.buf[0].len = sizeof(read_data); // Length of the command data
//
//
//  I2C_TransferReturn_TypeDef trans_ret = I2CSPM_Transfer(I2C0, &transferSequence);
//
//  // Checking if the transfer is done or no.
//  if(trans_ret != i2cTransferDone)
//      {
//        LOG_ERROR("I2C Write error: %d", trans_ret); // If transfer is not done then we will log error message
////        createEventSystemError();
//      }
//
//  return read_data;


  //uint8_t cmd_data[2];
  uint8_t cmd_data;
  //uint8_t no_data[1];
  I2C_TransferReturn_TypeDef transferStatus;

//  i2c_init();

  cmd_data = reg;

  // Sequence to send over I2C
  transferSequence.addr = MAX30105_ADDRESS << 1;
  transferSequence.flags = I2C_FLAG_WRITE_READ;

  transferSequence.buf[0].data = &cmd_data;
  transferSequence.buf[0].len = sizeof(cmd_data);

  transferSequence.buf[1].data = data;
  transferSequence.buf[1].len = len;

  // Enable NVIC for I2C0
  //NVIC_EnableIRQ(I2C0_IRQn);

  //transferStatus = I2C_TransferInit(I2C0, &transferSequence);
  transferStatus = I2CSPM_Transfer(I2C0, &transferSequence);
  if (transferStatus < 0) {
      LOG_ERROR("i2c_read_data_block_apds9960 failed: reg=0x%x, data=0x%x, len=%d, error = 0x%x\n\r", cmd_data, *data, len, transferStatus);
      //*data = 0xFF;
//      return (uint32_t)transferStatus;
      return -1;

  }

//  return (uint32_t)1;
  return (int)len;

}

uint8_t max30101_i2c_read_reg(uint8_t reg)
{

  uint8_t read_data;
  transferSequence.flags = I2C_FLAG_WRITE_READ, // Write command
  transferSequence.addr = (MAX30105_ADDRESS<<1), // Slave address needs to be left shift by one bit
  transferSequence.buf[0].data = &reg, // Passing the pointer that has the command data stored
  transferSequence.buf[0].len = sizeof(reg); // Length of the command data
  transferSequence.buf[1].data = &read_data, // Passing the pointer that has the command data stored
//  transferSequence.buf[1].len = nbytes_read_data;
  transferSequence.buf[1].len = 1;

//  i2c_init();
  // This will initialize the write command on to the bus
  I2C_TransferReturn_TypeDef trans_ret = I2CSPM_Transfer(I2C0,&transferSequence);
//  LOG_INFO("i2c_Read_blocking: %d\r", trans_ret);
  // Checking if the transfer is done or no.
  if(trans_ret != i2cTransferDone)
      {
        LOG_ERROR("I2C Write error: %d", trans_ret); // If transfer is not done then we will log error message
//        createEventSystemError();
      }
//  for (int i = 0; i < nbytes_read_data; i++)
//    {
//      printf("Reg 0x%02x :: Read %d:0x%02x\t", reg, i+1, *(read_data+i));
//    }
//  printf("\n\n");
  return read_data;
}
void max30101_i2c_write_reg(uint8_t reg, uint8_t value)
{
  transferSequence.flags = I2C_FLAG_WRITE_WRITE, // Write command
  transferSequence.addr = (MAX30105_ADDRESS<<1), // Slave address needs to be left shift by one bit
  transferSequence.buf[0].data = &reg, // Passing the pointer that has the command data stored
  transferSequence.buf[0].len = sizeof(reg); // Length of the command data
  transferSequence.buf[1].data = &value, // Passing the pointer that has the command data stored
//  transferSequence.buf[1].len = nbytes_write_data;
  transferSequence.buf[1].len = 1;

//  i2c_init();
  // This will initialize the write command on to the bus
  I2C_TransferReturn_TypeDef trans_ret = I2CSPM_Transfer(I2C0,&transferSequence);
//  LOG_INFO("i2c_Write_Write_blocking: %d\r", trans_ret);
  // Checking if the transfer is done or no.
  if(trans_ret != i2cTransferDone)
      {
        LOG_ERROR("I2C Write error: %d", trans_ret); // If transfer is not done then we will log error message
//        createEventSystemError();
      }

}
