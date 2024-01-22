#ifndef UTILS_h
#define UTILS_h

#include "Arduino.h"

// message codes sent to controller
#define BATTERY_PERCENT 1 

// I2C address of I2C devices
#define ADDR_CONTROLLER 0x08
#define ADDR_MAX17048 0x36

// MAX17048 Registers
#define REG_MAX17048_CONFIG 0x0C
#define REG_MAX17048_SOC 0x04
#define REG_MAX17048_VERSION 0x08
#define REG_MAX17048_ID 0x19
#define REG_MAX17048_STATUS 0x1A
#define REG_MAX17048_CMD 0xFE

// Controller Registers
#define REG_CONTROLLER_PERCENT 0x01

// MAX17048 bit positions
#define BIT_CONFIG_ATRT 0x20

// pin definitions
#define STAT_PIN PIN_PA6
#define BUTTON PIN_PC2
#define LED1 PIN_PA1
#define LED2 PIN_PA2
#define LED3 PIN_PA5
#define LED4 PIN_PA3
#define ERROR_INDICATOR_LED LED4
#define SDA_PIN SDA
#define SCL_PIN SCL

void blink(uint8_t led, uint8_t times, int onDuration, int offDuration);
void displayLevel(float percent);
void displayChargingStatus(float percent);
void displayError(uint8_t e);

void resetI2CBus();
uint8_t setConfig();
uint8_t writeToController(uint8_t _register, uint8_t data);


#endif