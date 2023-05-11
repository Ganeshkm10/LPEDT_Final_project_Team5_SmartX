/*
 * i2c.h
 *
 *  Created on: Sep 15, 2022
 *      Author: Jasan Singh
 */

#ifndef SRC_I2C_H_
#define SRC_I2C_H_

#include "em_i2c.h"
#include "sl_i2cspm.h"
#include "timers.h"
#include "gpio.h"
#include "lcd.h"
#define MAX30105_ADDRESS          0x57
void i2cInit();
bool i2c_write(uint8_t device_address, uint8_t *data, uint8_t number_of_bytes);
bool i2c_read(uint8_t device_address, uint8_t *data, uint8_t number_of_bytes);
///*
// *  Function initializes the I2C module with I2C_FREQ_STANDARD_MAX frequency
// */
//void I2CInit( );
//
///*
// *  Function does write I2C transfer with No Hold Master
// *  Returns 0 on success and 1 on failure
// */
//uint8_t I2CWrite();
//
///*
// *  Function reads from I2c Bus using I2C transfer
// *  and returns the 16 bit unsignedd value read
// *  Returns 1 when failures
// */
//uint16_t I2CRead();
//
///*
// *  Function call the sequence of routines to read
// *  temperature from Si7021
// */
//void i2c_read_temp();
//
///*
// *  Function logs the converted Temperature transferred from I2C read
// *  and converts into celsius
// */
//uint32_t I2C_temperatureReport();
uint8_t max30101_i2c_read_reg(uint8_t reg);
void max30101_i2c_write_reg(uint8_t reg, uint8_t value);
//void max30101_i2c_write_reg(uint8_t reg, uint8_t *value);
uint8_t max30101_i2c_read();
void max30101_i2c_write(uint8_t reg);
uint8_t max30101_i2c_burst_read(uint8_t reg, uint8_t *data, unsigned int len);

#endif /* SRC_I2C_H_ */
