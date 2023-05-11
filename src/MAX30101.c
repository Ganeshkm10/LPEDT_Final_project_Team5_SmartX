// *
// * Ported arduino library of sparkfun to C
// * MAX30101.c
// * reference: https://github.com/sparkfun/SparkFun_MAX3010x_Sensor_Library/tree/master/src
// */
//
///***************************************************
//  This is a library written for the Maxim MAX30105 Optical Smoke Detector
//  It should also work with the MAX30102. However, the MAX30102 does not have a Green LED.
//
//  These sensors use I2C to communicate, as well as a single (optional)
//  interrupt line that is not currently supported in this driver.
//
//  Written by Peter Jansen and Nathan Seidle (SparkFun)
//  BSD license, all text above must be included in any redistribution.
// *****************************************************/
//
//#include "MAX30105.h"


#include <src/MAX30101.h>
#include "i2c.h"
#include "irq.h"
#include "timers.h"


#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"


uint8_t _i2caddr;
uint8_t revisionID;
sense_struct sense;

uint8_t activeLEDs;
//
//// Status Registers
//static const uint8_t MAX30105_INTSTAT1 =    0x00;
//static const uint8_t MAX30105_INTSTAT2 =    0x01;
//static const uint8_t MAX30105_INTENABLE1 =    0x02;
//static const uint8_t MAX30105_INTENABLE2 =    0x03;
//
// FIFO Registers
static const uint8_t MAX30105_FIFOWRITEPTR =  0x04;
static const uint8_t MAX30105_FIFOOVERFLOW =  0x05;
static const uint8_t MAX30105_FIFOREADPTR =   0x06;
static const uint8_t MAX30105_FIFODATA =    0x07;
//
//// Configuration Registers
static const uint8_t MAX30105_FIFOCONFIG =    0x08;
static const uint8_t MAX30105_MODECONFIG =    0x09;
static const uint8_t MAX30105_PARTICLECONFIG =  0x0A;    // Note, sometimes listed as "SPO2" config in datasheet (pg. 11)
static const uint8_t MAX30105_LED1_PULSEAMP =   0x0C;
static const uint8_t MAX30105_LED2_PULSEAMP =   0x0D;
static const uint8_t MAX30105_LED3_PULSEAMP =   0x0E;
static const uint8_t MAX30105_LED_PROX_AMP =  0x10;
static const uint8_t MAX30105_MULTILEDCONFIG1 = 0x11;
static const uint8_t MAX30105_MULTILEDCONFIG2 = 0x12;
//
//// Die Temperature Registers
//static const uint8_t MAX30105_DIETEMPINT =    0x1F;
//static const uint8_t MAX30105_DIETEMPFRAC =   0x20;
//static const uint8_t MAX30105_DIETEMPCONFIG =   0x21;
//
//// Proximity Function Registers
//static const uint8_t MAX30105_PROXINTTHRESH =   0x30;
//
//// Part ID Registers
static const uint8_t MAX30105_REVISIONID =    0xFE;
static const uint8_t MAX30105_PARTID =      0xFF;    // Should always be 0x15. Identical to MAX30102.
//
//// MAX30105 Commands
//// Interrupt configuration (pg 13, 14)
//static const uint8_t MAX30105_INT_A_FULL_MASK =   (byte)~0b10000000;
//static const uint8_t MAX30105_INT_A_FULL_ENABLE =   0x80;
//static const uint8_t MAX30105_INT_A_FULL_DISABLE =  0x00;
//
//static const uint8_t MAX30105_INT_DATA_RDY_MASK = (byte)~0b01000000;
//static const uint8_t MAX30105_INT_DATA_RDY_ENABLE = 0x40;
//static const uint8_t MAX30105_INT_DATA_RDY_DISABLE = 0x00;
//
//static const uint8_t MAX30105_INT_ALC_OVF_MASK = (byte)~0b00100000;
//static const uint8_t MAX30105_INT_ALC_OVF_ENABLE =  0x20;
//static const uint8_t MAX30105_INT_ALC_OVF_DISABLE = 0x00;
//
//static const uint8_t MAX30105_INT_PROX_INT_MASK = (byte)~0b00010000;
//static const uint8_t MAX30105_INT_PROX_INT_ENABLE = 0x10;
//static const uint8_t MAX30105_INT_PROX_INT_DISABLE = 0x00;
//
//static const uint8_t MAX30105_INT_DIE_TEMP_RDY_MASK = (byte)~0b00000010;
//static const uint8_t MAX30105_INT_DIE_TEMP_RDY_ENABLE = 0x02;
//static const uint8_t MAX30105_INT_DIE_TEMP_RDY_DISABLE = 0x00;
//
static const uint8_t MAX30105_SAMPLEAVG_MASK =  (uint8_t)~0b11100000;
//static const uint8_t MAX30105_SAMPLEAVG_1 =   0x00;
//static const uint8_t MAX30105_SAMPLEAVG_2 =   0x20;
static const uint8_t MAX30105_SAMPLEAVG_4 =   0x40;
//static const uint8_t MAX30105_SAMPLEAVG_8 =   0x60;
//static const uint8_t MAX30105_SAMPLEAVG_16 =  0x80;
//static const uint8_t MAX30105_SAMPLEAVG_32 =  0xA0;
//
static const uint8_t MAX30105_ROLLOVER_MASK =   0xEF;
static const uint8_t MAX30105_ROLLOVER_ENABLE = 0x10;
//static const uint8_t MAX30105_ROLLOVER_DISABLE = 0x00;
//
//static const uint8_t MAX30105_A_FULL_MASK =   0xF0;
//
//// Mode configuration commands (page 19)
//static const uint8_t MAX30105_SHUTDOWN_MASK =   0x7F;
//static const uint8_t MAX30105_SHUTDOWN =    0x80;
//static const uint8_t MAX30105_WAKEUP =      0x00;
//
static const uint8_t MAX30105_RESET_MASK =    0xBF;
static const uint8_t MAX30105_RESET =       0x40;
//
static const uint8_t MAX30105_MODE_MASK =     0xF8;
static const uint8_t MAX30105_MODE_REDONLY =  0x02;
static const uint8_t MAX30105_MODE_REDIRONLY =  0x03;
static const uint8_t MAX30105_MODE_MULTILED =   0x07;
//
//// Particle sensing configuration commands (pgs 19-20)
static const uint8_t MAX30105_ADCRANGE_MASK =   0x9F;
//static const uint8_t MAX30105_ADCRANGE_2048 =   0x00;
//static const uint8_t MAX30105_ADCRANGE_4096 =   0x20;
//static const uint8_t MAX30105_ADCRANGE_8192 =   0x40;
static const uint8_t MAX30105_ADCRANGE_16384 =  0x60;
//
static const uint8_t MAX30105_SAMPLERATE_MASK = 0xE3;
//static const uint8_t MAX30105_SAMPLERATE_50 =   0x00;
static const uint8_t MAX30105_SAMPLERATE_100 =  0x04;
//static const uint8_t MAX30105_SAMPLERATE_200 =  0x08;
static const uint8_t MAX30105_SAMPLERATE_400 =  0x0C;
//static const uint8_t MAX30105_SAMPLERATE_800 =  0x10;
//static const uint8_t MAX30105_SAMPLERATE_1000 = 0x14;
//static const uint8_t MAX30105_SAMPLERATE_1600 = 0x18;
//static const uint8_t MAX30105_SAMPLERATE_3200 = 0x1C;
//
static const uint8_t MAX30105_PULSEWIDTH_MASK = 0xFC;
//static const uint8_t MAX30105_PULSEWIDTH_69 =   0x00;
//static const uint8_t MAX30105_PULSEWIDTH_118 =  0x01;
//static const uint8_t MAX30105_PULSEWIDTH_215 =  0x02;
static const uint8_t MAX30105_PULSEWIDTH_411 =  0x03;
//
//Multi-LED Mode configuration (pg 22)
static const uint8_t MAX30105_SLOT1_MASK =    0xF8;
static const uint8_t MAX30105_SLOT2_MASK =    0x8F;
static const uint8_t MAX30105_SLOT3_MASK =    0xF8;
static const uint8_t MAX30105_SLOT4_MASK =    0x8F;

//static const uint8_t SLOT_NONE =        0x00;
static const uint8_t SLOT_RED_LED =       0x01;
static const uint8_t SLOT_IR_LED =        0x02;
static const uint8_t SLOT_GREEN_LED =       0x03;
//static const uint8_t SLOT_NONE_PILOT =      0x04;
static const uint8_t SLOT_RED_PILOT =     0x05;
static const uint8_t SLOT_IR_PILOT =      0x06;
static const uint8_t SLOT_GREEN_PILOT =     0x07;
//
static const uint8_t MAX_30105_EXPECTEDPARTID = 0x15;
//

//
//boolean MAX30105_begin(TwoWire &wirePort, uint32_t i2cSpeed, uint8_t i2caddr)

void MAX30105_init()
{
  LOG_INFO("Register 0x08 %d\r", readRegister8(_i2caddr, 0x08));
  LOG_INFO("Register 0x09 %d\r", readRegister8(_i2caddr, 0x09));
  LOG_INFO("Register 0x0C %d\r", readRegister8(_i2caddr, 0x0C));
  LOG_INFO("Register 0x11 %d\r", readRegister8(_i2caddr, 0x11));
  LOG_INFO("Register 0x00 %d\r", readRegister8(_i2caddr, 0x00));

  writeRegister8(_i2caddr, 0x08, 0x51);
  timerWaitUs(5000);

  writeRegister8(_i2caddr, 0x09, 0x02);
  timerWaitUs(5000);

  writeRegister8(_i2caddr, 0x0C, 0x4F);
  timerWaitUs(5000);

  writeRegister8(_i2caddr, 0x11, 0x11);
  timerWaitUs(5000);

  writeRegister8(_i2caddr, 0x02, 0x80);
  timerWaitUs(5000);

  writeRegister8(_i2caddr, 0x0A, 0x1F);
  timerWaitUs(5000);

  LOG_INFO("After writing to registers\r");
  LOG_INFO("Register 0x08 %d\r", readRegister8(_i2caddr, 0x08));
  LOG_INFO("Register 0x09 %d\r", readRegister8(_i2caddr, 0x09));
  LOG_INFO("Register 0x0C %d\r", readRegister8(_i2caddr, 0x0C));
  LOG_INFO("Register 0x11 %d\r", readRegister8(_i2caddr, 0x11));
  LOG_INFO("Register 0x00 %d\r", readRegister8(_i2caddr, 0x00));




}
bool MAX30105_begin()
{

  uint8_t id = MAX30105_readPartID();
  LOG_INFO("Part ID = %d\r", id);
  if (id != MAX_30105_EXPECTEDPARTID) {
    // Error -- Part ID read from MAX30105 does not match expected part ID.
    // This may mean there is a physical connectivity problem (broken wire, unpowered, etc).
    return false;
  }

  // Populate revision ID
  MAX30105_readRevisionID();

  return true;
}
//
////
//// Configuration
////
//

void softReset(void) {
  bitMask(MAX30105_MODECONFIG, MAX30105_RESET_MASK, MAX30105_RESET);

  // Poll for bit to clear, reset is then complete
  // Timeout after 100ms
  unsigned long startTime = millis();
  while (millis() - startTime < 100)
  {
    uint8_t response = readRegister8(_i2caddr, MAX30105_MODECONFIG);
    if ((response & MAX30105_RESET) == 0) break; //We're done!
//    delay(1); //Let's not over burden the I2C bus
    timerWaitUs(1000);
  }
}

void setLEDMode(uint8_t mode) {
  // Set which LEDs are used for sampling -- Red only, RED+IR only, or custom.
  // See datasheet, page 19
  bitMask(MAX30105_MODECONFIG, MAX30105_MODE_MASK, mode);
}
//
void setADCRange(uint8_t adcRange) {
  // adcRange: one of MAX30105_ADCRANGE_2048, _4096, _8192, _16384
  bitMask(MAX30105_PARTICLECONFIG, MAX30105_ADCRANGE_MASK, adcRange);
}

void setSampleRate(uint8_t sampleRate) {
  // sampleRate: one of MAX30105_SAMPLERATE_50, _100, _200, _400, _800, _1000, _1600, _3200
  bitMask(MAX30105_PARTICLECONFIG, MAX30105_SAMPLERATE_MASK, sampleRate);
}

void setPulseWidth(uint8_t pulseWidth) {
  // pulseWidth: one of MAX30105_PULSEWIDTH_69, _188, _215, _411
  bitMask(MAX30105_PARTICLECONFIG, MAX30105_PULSEWIDTH_MASK, pulseWidth);
}
//
// NOTE: Amplitude values: 0x00 = 0mA, 0x7F = 25.4mA, 0xFF = 50mA (typical)
// See datasheet, page 21
void MAX30105_setPulseAmplitudeRed(uint8_t amplitude) {
  writeRegister8(_i2caddr, MAX30105_LED1_PULSEAMP, amplitude);
}
//
void MAX30105_setPulseAmplitudeIR(uint8_t amplitude) {
  writeRegister8(_i2caddr, MAX30105_LED2_PULSEAMP, amplitude);
}

void MAX30105_setPulseAmplitudeGreen(uint8_t amplitude) {
  writeRegister8(_i2caddr, MAX30105_LED3_PULSEAMP, amplitude);
}

void MAX30105_setPulseAmplitudeProximity(uint8_t amplitude) {
  writeRegister8(_i2caddr, MAX30105_LED_PROX_AMP, amplitude);
}

void enableSlot(uint8_t slotNumber, uint8_t device) {

  uint8_t originalContents;

  switch (slotNumber) {
    case (1):
      bitMask(MAX30105_MULTILEDCONFIG1, MAX30105_SLOT1_MASK, device);
      break;
    case (2):
      bitMask(MAX30105_MULTILEDCONFIG1, MAX30105_SLOT2_MASK, device << 4);
      break;
    case (3):
      bitMask(MAX30105_MULTILEDCONFIG2, MAX30105_SLOT3_MASK, device);
      break;
    case (4):
      bitMask(MAX30105_MULTILEDCONFIG2, MAX30105_SLOT4_MASK, device << 4);
      break;
    default:
      //Shouldn't be here!
      break;
  }
}

void setFIFOAverage(uint8_t numberOfSamples) {
  bitMask(MAX30105_FIFOCONFIG, MAX30105_SAMPLEAVG_MASK, numberOfSamples);
}
//
//Resets all points to start in a known state
//Page 15 recommends clearing FIFO before beginning a read
void clearFIFO(void) {
  writeRegister8(_i2caddr, MAX30105_FIFOWRITEPTR, 0);
  writeRegister8(_i2caddr, MAX30105_FIFOOVERFLOW, 0);
  writeRegister8(_i2caddr, MAX30105_FIFOREADPTR, 0);
}

//Enable roll over if FIFO over flows
void enableFIFORollover(void) {
  bitMask(MAX30105_FIFOCONFIG, MAX30105_ROLLOVER_MASK, MAX30105_ROLLOVER_ENABLE);
}

//Read the FIFO Write Pointer
uint8_t getWritePointer(void) {
  return (readRegister8(_i2caddr, MAX30105_FIFOWRITEPTR));
}
//
//Read the FIFO Read Pointer
uint8_t getReadPointer(void) {
  return (readRegister8(_i2caddr, MAX30105_FIFOREADPTR));
}
//// Device ID and Revision
////
uint8_t MAX30105_readPartID() {
  return readRegister8(_i2caddr, MAX30105_PARTID);
}

void MAX30105_readRevisionID() {
  revisionID = readRegister8(_i2caddr, MAX30105_REVISIONID);
}

uint8_t MAX30105_getRevisionID() {

  return revisionID;
}

//
//Setup the sensor
//The MAX30105 has many settings. By default we select:
// Sample Average = 4
// Mode = MultiLED
// ADC Range = 16384 (62.5pA per LSB)
// Sample rate = 50
//Use the default setup if you are just getting started with the MAX30105 sensor
// Call this function with the following values: uint8_t powerLevel = 0x1F, uint8_t sampleAverage = 4, uint8_t ledMode = 3, int sampleRate = 400, int pulseWidth = 411, int adcRange = 4096
void MAX30105_setup(uint8_t powerLevel, uint8_t sampleAverage, uint8_t ledMode, int sampleRate, int pulseWidth, int adcRange) {
  softReset(); //Reset all configuration, threshold, and data registers to POR values
//
//  //FIFO Configuration
    setFIFOAverage(MAX30105_SAMPLEAVG_4);
    enableFIFORollover(); //Allow FIFO to wrap/roll over
//  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
//  //Mode Configuration
//  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  if (ledMode == 3) setLEDMode(MAX30105_MODE_MULTILED); //Watch all three LED channels
  else if (ledMode == 2) setLEDMode(MAX30105_MODE_REDIRONLY); //Red and IR
  else setLEDMode(MAX30105_MODE_REDONLY); //Red only



    activeLEDs = ledMode; //Used to control how many bytes to read from FIFO buffer

  setADCRange(MAX30105_ADCRANGE_16384);

  setSampleRate(MAX30105_SAMPLERATE_400);

  setPulseWidth(MAX30105_PULSEWIDTH_411);

  MAX30105_setPulseAmplitudeRed(powerLevel);
  MAX30105_setPulseAmplitudeIR(powerLevel);
  MAX30105_setPulseAmplitudeGreen(powerLevel);
  MAX30105_setPulseAmplitudeProximity(powerLevel);

  enableSlot(1, SLOT_RED_LED);
  if (ledMode > 1) enableSlot(2, SLOT_IR_LED);
  if (ledMode > 2) enableSlot(3, SLOT_GREEN_LED);

  clearFIFO(); //Reset the FIFO before we begin checking the sensor
}

uint8_t MAX30105_available(void)
{
  int8_t numberOfSamples = sense.head - sense.tail;
  if (numberOfSamples < 0) numberOfSamples += STORAGE_SIZE;

  return (numberOfSamples);
}

uint32_t MAX30105_getRed(void)
{
  //Check the sensor for new data for 250ms
  if(MAX30105_safeCheck(250))
    return (sense.red[sense.head]);
  else
    return(0); //Sensor failed to find new data
}
//
//Report the most recent IR value
uint32_t MAX30105_getIR(void)
{
  //Check the sensor for new data for 250ms
  if(MAX30105_safeCheck(250))
    return (sense.IR[sense.head]);
  else
    return(0); //Sensor failed to find new data
}

//Advance the tail
void MAX30105_nextSample(void)
{
  if(MAX30105_available()) //Only advance the tail if new data is available
  {
    sense.tail++;
    sense.tail %= STORAGE_SIZE; //Wrap condition
  }
}

uint16_t check(void)
{
  uint8_t readPointer = getReadPointer();
  uint8_t writePointer = getWritePointer();


  int numberOfSamples = 0;

  if (readPointer != writePointer)
  {

    numberOfSamples = writePointer - readPointer;
    if (numberOfSamples < 0) numberOfSamples += 32; //Wrap condition

    int bytesLeftToRead = numberOfSamples * activeLEDs * 3;

    while (bytesLeftToRead > 0)
    {
      int toGet = bytesLeftToRead;
      if (toGet > I2C_BUFFER_LENGTH)
      {


        toGet = I2C_BUFFER_LENGTH - (I2C_BUFFER_LENGTH % (activeLEDs * 3)); //Trim toGet to be a multiple of the samples we need to read
      }

      bytesLeftToRead -= toGet;

      while (toGet > 0)
      {
        sense.head++; //Advance the head of the storage struct
        sense.head %= STORAGE_SIZE; //Wrap condition

        uint8_t tempred[sizeof(uint32_t)]; //Array of 4 bytes that we will convert into long
        uint8_t tempir[sizeof(uint32_t)]; //Array of 4 bytes that we will convert into long
        uint32_t tempLongred,tempLongir;

//        LOG_INFO("Read pointer 1 %d \r\n", getReadPointer());
        //Burst read three bytes - RED
        tempred[3] = 0;
        uint8_t t[6];
        max30101_i2c_burst_read(MAX30105_FIFODATA, t, 6);
        tempred[2] = t[0];
        tempred[1] = t[1];
        tempred[0] = t[2];

        tempir[2] = t[3];
        tempir[1] = t[4];
        tempir[0] = t[5];
        //Convert array to long
        memcpy(&tempLongred, tempred, sizeof(tempLongred));

        tempLongred &= 0x3FFFF; //Zero out all but 18 bits

        sense.red[sense.head] = tempLongred; //Store this reading into the sense array

        //Convert array to long
        memcpy(&tempLongir, tempir, sizeof(tempLongir));

        tempLongir &= 0x3FFFF; //Zero out all but 18 bits

        sense.IR[sense.head] = tempLongir; //Store this reading into the sense array

        toGet -= activeLEDs * 3;
      }

    }

  }

  return (numberOfSamples); //Let the world know how much new data we found
}
//
////Check for new data but give up after a certain amount of time
////Returns true if new data was found
////Returns false if new data was not found
bool MAX30105_safeCheck(uint8_t maxTimeToCheck)
{
  uint32_t markTime = millis();
//  LOG_INFO("Here\r");
  while(1)
  {
  if(millis() - markTime > maxTimeToCheck)
  {
      LOG_INFO("Exiting because > 250ms\r");
      return(false);
  }

  if(check() == true) //We found new data!
    return(true);

//  delay(1);
  timerWaitUs(10);
  }
}
//
//Given a register, read it, mask it, and then set the thing
void bitMask(uint8_t reg, uint8_t mask, uint8_t thing)
{
  // Grab current register context
  uint8_t originalContents = readRegister8(_i2caddr, reg);

  // Zero-out the portions of the register we're interested in
  originalContents = originalContents & mask;

  // Change contents
  writeRegister8(_i2caddr, reg, originalContents | thing);
}
//
//
// Low-level I2C Communication
//
uint8_t readRegister8(uint8_t address, uint8_t reg) {

  return max30101_i2c_read_reg(reg);

}

void writeRegister8(uint8_t address, uint8_t reg, uint8_t value) {

  max30101_i2c_write_reg(reg, value);
}
uint8_t _i2cPort_read()
{
  return max30101_i2c_read();
}

void _i2cPort_write(uint8_t reg)
{
  max30101_i2c_write(reg);
}
uint32_t millis(){
  return letimerMilliseconds();
}
