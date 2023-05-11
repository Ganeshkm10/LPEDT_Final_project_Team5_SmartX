/*
 * BME680.h
 *
 *  Created on: 03-Mar-2023
 *      Author: Ganesh KM
 */

#ifndef SRC_BME680_H_
#define SRC_BME680_H_

#include <stdint.h>

#define BME680_I2C_ADDRESS 0x76
//#define BME680_TEMP_MSB_ADDR 0x22
//#define BME680_TEMP_LSB_ADDR 0x23
//#define BME680_TEMP_XLSB_ADDR 0x24
//#define BME680_PRESS_MSB_ADDR 0x1F
//#define BME680_PRESS_LSB_ADDR 0x20
//#define BME680_PRESS_XLSB_ADDR 0x21
//#define BME680_HUM_MSB_ADDR 0x25
//#define BME680_HUM_LSB_ADDR 0x26

//#define BME680_CTRL_HUM_ADDR 0x72
//#define BME680_CTRL_MEAS_ADDR 0x74
//#define BME680_CONFIG_ADDR 0x75
//#define BME680_RESET_ADDR 0xE0

void i2c_init_bme680(void);
//void i2c_write_reg(uint8_t reg_addr, uint8_t value);
//uint8_t i2c_read_reg(uint8_t reg_addr);
//int16_t read_temp(void);
//int32_t read_pressure(void);
//int16_t read_humidity(void);

void readTemperature(void);
int8_t bme_i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len);
int8_t bme_i2c_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len);

#endif /* SRC_BME680_H_ */
