/*
// * Ported arduino library of sparkfun to C
// * MAX30101.h
 * reference: https://github.com/sparkfun/SparkFun_MAX3010x_Sensor_Library/tree/master/src
 */

#ifndef SRC_MAX30101_H_
#define SRC_MAX30101_H_

/***************************************************
 This is a library written for the Maxim MAX30105 Optical Smoke Detector
 It should also work with the MAX30102. However, the MAX30102 does not have a Green LED.

 These sensors use I2C to communicate, as well as a single (optional)
 interrupt line that is not currently supported in this driver.

 Written by Peter Jansen and Nathan Seidle (SparkFun)
 BSD license, all text above must be included in any redistribution.
 *****************************************************/


#include <stdint.h>
#include <stdbool.h>
#include <string.h> // For memcpy


#define I2C_BUFFER_LENGTH 64


    bool MAX30105_begin();

    uint32_t MAX30105_getRed(void); //Returns immediate red value
    uint32_t MAX30105_getIR(void); //Returns immediate IR value

    bool MAX30105_safeCheck(uint8_t maxTimeToCheck); //Given a max amount of time, check for new data

  void softReset();

    void setLEDMode(uint8_t mode);

  void setADCRange(uint8_t adcRange);
  void setSampleRate(uint8_t sampleRate);
  void setPulseWidth(uint8_t pulseWidth);

    void MAX30105_setPulseAmplitudeRed(uint8_t value);
    void MAX30105_setPulseAmplitudeIR(uint8_t value);
    void MAX30105_setPulseAmplitudeGreen(uint8_t value);
    void MAX30105_setPulseAmplitudeProximity(uint8_t value);

  void setFIFOAverage(uint8_t samples);
  void enableFIFORollover();

  uint16_t check(void);

  uint8_t MAX30105_available(void);
  void MAX30105_nextSample(void);

  uint8_t getWritePointer(void);
  uint8_t getReadPointer(void);
  void clearFIFO(void);

  uint8_t MAX30105_getRevisionID();
  uint8_t MAX30105_readPartID();

  void MAX30105_setup(uint8_t powerLevel, uint8_t sampleAverage, uint8_t ledMode, int sampleRate, int pulseWidth, int adcRange);
  //
  // Low-level I2C communication
  uint8_t _i2cPort_read();
  uint8_t readRegister8(uint8_t address, uint8_t reg);
  void writeRegister8(uint8_t address, uint8_t reg, uint8_t value);


  void MAX30105_readRevisionID();

  void bitMask(uint8_t reg, uint8_t mask, uint8_t thing);
//
  #define STORAGE_SIZE 4 //Each long is 4 bytes so limit this to fit on your micro
  typedef struct Record
  {
    uint32_t red[STORAGE_SIZE];
    uint32_t IR[STORAGE_SIZE];
    uint32_t green[STORAGE_SIZE];
    uint8_t head;
    uint8_t tail;
  } sense_struct; //This is our circular buffer of readings from the sensor


  uint32_t millis();

  void MAX30105_init();

#endif /* SRC_MAX30101_H_ */
