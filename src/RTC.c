/*
 * RTC.c
 *
 *  Created on: Mar 24, 2023
 *      Author: jasan
 */

#include "RTC.h"
#include "i2c.h"
#include "math.h"
// Include logging specifically for this .c file
#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

#define TIME_ARRAY_LENGTH 3 // Total number of writable values in device
uint8_t _time[TIME_ARRAY_LENGTH];



// Parse the __TIME__ predefined macro to generate time defaults:
// __TIME__ Format: HH:MM:SS (First number of each is padded by 0 if <10)
// <HOUR>
#define BUILD_HOUR_0 ((__TIME__[0] == ' ') ? 0 : (__TIME__[0] - 0x30))
#define BUILD_HOUR_1 (__TIME__[1] - 0x30)
#define BUILD_HOUR ((BUILD_HOUR_0 * 10) + BUILD_HOUR_1)
// <MINUTE>
#define BUILD_MINUTE_0 ((__TIME__[3] == ' ') ? 0 : (__TIME__[3] - 0x30))
#define BUILD_MINUTE_1 (__TIME__[4] - 0x30)
#define BUILD_MINUTE ((BUILD_MINUTE_0 * 10) + BUILD_MINUTE_1)
// <SECOND>
#define BUILD_SECOND_0 ((__TIME__[6] == ' ') ? 0 : (__TIME__[6] - 0x30))
#define BUILD_SECOND_1 (__TIME__[7] - 0x30)
#define BUILD_SECOND ((BUILD_SECOND_0 * 10) + BUILD_SECOND_1)

#define RTC_ADDR 0x52 // RTC module I2C address

static void delayApprox(int delay)
{
  volatile int i;

  for (i = 0; i < delay; ) {
      i=i+1;
  }

} // delayApprox()

//bool setTime(uint8_t sec, uint8_t min, uint8_t hour, uint8_t weekday, uint8_t date, uint8_t month, uint16_t year)
//{
//  _time[TIME_SECONDS] = DECtoBCD(sec);
//  _time[TIME_MINUTES] = DECtoBCD(min);
//  _time[TIME_HOURS] = DECtoBCD(hour);
//  _time[TIME_WEEKDAY] = DECtoBCD(weekday);
//  _time[TIME_DATE] = DECtoBCD(date);
//   _time[TIME_MONTH] = DECtoBCD(month);
//  _time[TIME_YEAR] = DECtoBCD(year - 2000);
//
//  bool status = false;
//
//  if (is12Hour())
//  {
//    set24Hour();
//    status = setTime(_time, TIME_ARRAY_LENGTH);
//    set12Hour();
//  }
//  else
//  {
//    status = setTime(_time, TIME_ARRAY_LENGTH);
//  }
//  return status;
//}


//bool writeMultipleRegisters(uint8_t addr, uint8_t * values, uint8_t len)
//{
//    for (uint8_t i = 0; i < len; i++)
//    {
//        ret = i2c_write(RV3028_ADDR, &data, 2);
//        if(ret != i2cTransferDone){
//            LOG_INFO("I2C ERROR: Reset the device. Error Code = %d\r\n",ret);
//        }
//    }
//}
//bool setTime(uint8_t * time, uint8_t len)
//{
//  if (len != TIME_ARRAY_LENGTH)
//    return false;
//
//  return writeMultipleRegisters(RV3028_SECONDS, time, len);
//}

uint8_t BCDtoDEC(uint8_t val)
{
  return ((val / 0x10) * 10) + (val % 0x10);
}


uint8_t DECtoBCD(uint8_t val)
{
  return ((val / 10) * 0x10) + (val % 10);
}

//bool setToCompilerTime()
//{
//  _time[TIME_SECONDS] = DECtoBCD(BUILD_SECOND);
//  _time[TIME_MINUTES] = DECtoBCD(BUILD_MINUTE);
//  _time[TIME_HOURS] = DECtoBCD(BUILD_HOUR);
//
//  //Build_Hour is 0-23, convert to 1-12 if needed
//  if (is12Hour())
//  {
//    uint8_t hour = BUILD_HOUR;
//
//    bool pm = false;
//
//    if (hour == 0)
//      hour += 12;
//    else if (hour == 12)
//      pm = true;
//    else if (hour > 12)
//    {
//      hour -= 12;
//      pm = true;
//    }
//
//    _time[TIME_HOURS] = DECtoBCD(hour); //Load the modified hours
//
//    if (pm == true) _time[TIME_HOURS] |= (1 << HOURS_AM_PM); //Set AM/PM bit if needed
//  }
//
////  // Calculate weekday (from here: http://stackoverflow.com/a/21235587)
////  // 0 = Sunday, 6 = Saturday
////  uint16_t d = BUILD_DATE;
////  uint16_t m = BUILD_MONTH;
////  uint16_t y = BUILD_YEAR;
////  uint16_t weekday = (d += m < 3 ? y-- : y - 2, 23 * m / 9 + d + 4 + y / 4 - y / 100 + y / 400) % 7 + 1;
////  _time[TIME_WEEKDAY] = DECtoBCD(weekday);
////
////  _time[TIME_DATE] = DECtoBCD(BUILD_DATE);
////  _time[TIME_MONTH] = DECtoBCD(BUILD_MONTH);
////  _time[TIME_YEAR] = DECtoBCD(BUILD_YEAR - 2000); //! Not Y2K (or Y2.1K)-proof :(
//
//  return setTime(_time, TIME_ARRAY_LENGTH);
//}

void RTC_RV3028_Init()
{
    I2C_TransferReturn_TypeDef ret;

    i2cInit();

//    // Reset the device
//    uint8_t data = MPU6050_PWR_MGMT_1;
//    ret = i2c_write(RV3028_ADDR, &data, 2);
//    if(ret != i2cTransferDone){
//        LOG_INFO("I2C ERROR: Reset the device. Error Code = %d\r\n",ret);
//    }
//    // Wait for some time. Need to figure the exact time
//      for(int i=0; i<30000; i++){
//          __asm("nop");
//      }
//    uint8_t timeSeconds = DECtoBCD(BUILD_SECOND);
//
//    ret = i2c_write(RV3028_ADDR, &timeSeconds, 2);
//    // Wait for some time. Need to figure the exact time
//    for(int i=0; i<30000; i++){
//        __asm("nop");
//    }
//    if(ret != i2cTransferDone){
//        LOG_INFO("I2C ERROR: Set Seconds failed. Error Code = %d\r\n",ret);
//    }
//    else
//      LOG_INFO("I2C ERROR: Set Seconds Success. Error Code = %d\r\n",ret);


}

uint8_t RTC_RV3028_Write( )
{
  //uint8_t _minutes = DECtoBCD(BUILD_MINUTE);

  _time[TIME_SECONDS] = DECtoBCD(BUILD_SECOND);
  _time[TIME_MINUTES] = DECtoBCD(BUILD_MINUTE);
  _time[TIME_HOURS] = DECtoBCD(BUILD_HOUR);

  printf(" H = %d M = %d S = %d", BUILD_HOUR, BUILD_MINUTE, BUILD_SECOND );

  uint8_t _data = _time[TIME_MINUTES];
  uint8_t reg = 0x01; // minutes register
  uint8_t data;
  I2C_TransferSeq_TypeDef seq;
  I2C_TransferReturn_TypeDef ret;



  // Set up the transfer sequence
  seq.addr = RTC_ADDR;
  seq.addr = 0xA4;
  seq.flags = I2C_FLAG_WRITE_WRITE;
  seq.buf[0].data = &reg;
  seq.buf[0].len = 1;
  seq.buf[1].data = &(_data);
  seq.buf[1].len = 1;

  // Perform the transfer
  ret = I2C_TransferInit(I2C0, &seq);
  while (ret == i2cTransferInProgress) {
      ret = I2C_Transfer(I2C0);
  }

  if (ret != i2cTransferDone) {
      // Error handling
      return 0xFF;
  }

  reg = 0x00;
  _data = _time[TIME_SECONDS];
  // Set up the transfer sequence
  seq.addr = RTC_ADDR;
  seq.addr = 0xA4;
  seq.flags = I2C_FLAG_WRITE_WRITE;
  seq.buf[0].data = &reg;
  seq.buf[0].len = 1;
  seq.buf[1].data = &(_data);
  seq.buf[1].len = 1;

  // Perform the transfer
  ret = I2C_TransferInit(I2C0, &seq);
  while (ret == i2cTransferInProgress) {
      ret = I2C_Transfer(I2C0);
  }

  if (ret != i2cTransferDone) {
      // Error handling
      return 0xFF;
  }

     reg = 0x02;
    _data = _time[TIME_HOURS];
    // Set up the transfer sequence
    seq.addr = RTC_ADDR;
    seq.addr = 0xA4;
    seq.flags = I2C_FLAG_WRITE_WRITE;
    seq.buf[0].data = &reg;
    seq.buf[0].len = 1;
    seq.buf[1].data = &(_data);
    seq.buf[1].len = 1;

    // Perform the transfer
    ret = I2C_TransferInit(I2C0, &seq);
    while (ret == i2cTransferInProgress) {
        ret = I2C_Transfer(I2C0);
    }

    if (ret != i2cTransferDone) {
        // Error handling
        return 0xFF;
    }
  return 0; // on successful write
}

//void RTC_RV3028_GetData()
//{
//  I2C_TransferReturn_TypeDef ret;
//  uint8_t data = RV3028_SECONDS;
//
//  ret = i2c_write(RV3028_ADDR, &data, 1);
//  if(ret != i2cTransferDone){
//          LOG_INFO("I2C ERROR: Reset the device. Error Code = %d\r\n",ret);
//      }
//  ret = i2c_read(RV3028_ADDR,&data,1);
//  if(ret != i2cTransferDone){
//          LOG_INFO("I2C ERROR: Reset the device. Error Code = %d\r\n",ret);
//      }
//  printf("Seconds read = %d \n\r", data);
//    // Wait for some time. Need to figure the exact time
//         for(int i=0; i<30000; i++){
//             __asm("nop");
//         }
//
//  printf(" read data = %d \n\r", data);
//  data = BCDtoDEC(data);
//
//  printf("Seconds read = %d \n\r", data);
//}

// Function to read a single byte from the RTC module
uint8_t RTC_ReadReg(uint8_t reg) {
    uint8_t data;
    I2C_TransferSeq_TypeDef seq;
    I2C_TransferReturn_TypeDef ret;



    // Set up the transfer sequence
    seq.addr = RTC_ADDR; // 0x52 = 0101_0010
    seq.addr = 0xA4;   // 1010_0100   I.E above address left shifted 1 bit
    seq.flags = I2C_FLAG_WRITE_READ;
    seq.buf[0].data = &reg;
    seq.buf[0].len = 1;
    seq.buf[1].data = &data;
    seq.buf[1].len = 1;

    // Perform the transfer
    ret = I2C_TransferInit(I2C0, &seq);
    while (ret == i2cTransferInProgress) {
        ret = I2C_Transfer(I2C0);
    }

    if (ret != i2cTransferDone) {
        // Error handling
        return 0xFF;
    }

    return data;
}




    //}
void RTC_RV3028_GetData()
{
  static uint8_t sec;
  static uint8_t min;
  static uint8_t hrs;

    // Read the time registers

   sec = RTC_ReadReg(0x00);
   //delayApprox(3500000/2);
   sec = BCDtoDEC(sec);
   min = RTC_ReadReg(0x01);
   //delayApprox(3500000/2);
   min = BCDtoDEC(min);
   hrs = RTC_ReadReg(0x02);
   //delayApprox(3500000/2);
   hrs = BCDtoDEC(hrs);

   printf("Current time: %02d : %02d : %02d \r\n\r\n", hrs ,min, sec);

   set_custom_font();
   displayPrintf(DISPLAY_ROW_BTADDR2, "%02d:%02d:%02d", hrs ,min, sec);
      // delayApprox(3500000/2);
}
