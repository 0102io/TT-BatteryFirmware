#include "utils.h"
#include <Arduino.h>
#include <Wire.h>

uint8_t leds[] = {LED1, LED2, LED3, LED4};

// ------------------------------ LED Functions ------------------------------

// blink one of the indicator LEDs on and off; used for debugging
void blink(uint8_t led, uint8_t times, int onDuration, int offDuration) {
    if (led < 1 || led > 4) led = 4;
    led = led - 1; // make 0-based
    for(uint8_t i = 0; i < times; i++) {
        digitalWrite(leds[led], LOW);
        delay(onDuration);
        digitalWrite(leds[led], HIGH);
        delay(offDuration);
    }
}

// light up a number of LEDs corresponding to the value of the passed parameter
void displayLevel(float percent) {
    // display cell level
    digitalWrite(LED1, LOW);
    if (percent > 25) {
        digitalWrite(LED2, LOW);
        if (percent > 50) {
            digitalWrite(LED3, LOW);
            if (percent > 75) {
                digitalWrite(LED4, LOW);
            }
        }
    }
}

void turnOffLeds() {
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, HIGH);
    digitalWrite(LED4, HIGH);
}

// animation for displaying where we are in the charging cycle
void displayChargingStatus(float percent) { 
    uint8_t ledPosition = 3;
    if (percent < 75) ledPosition = 2;
    else if (percent < 50) ledPosition = 1;
    else if (percent < 25) ledPosition = 0;

    for(int i = 0; i < ledPosition; i++) {
        digitalWrite(leds[i], LOW);
    }
    digitalWrite(leds[ledPosition - 1], LOW);
    delay(500);
    digitalWrite(leds[ledPosition - 1], HIGH);
    delay(500);

    for(int i = 0; i < ledPosition; i++) {
        digitalWrite(leds[i], HIGH);
    }
}

// debugging function that blinks a light a number of times equal to the value of an error value
void displayError(uint8_t e) {
    // error codes: https://github.com/SpenceKonde/megaTinyCore/blob/master/megaavr/libraries/Wire/src/Wire.cpp 
    // err = 0 --> transmission was successful
    // err = 2 --> NACK on transmit of address. This usually means the device is not responding, e.g. if it's not connected.
    // err = 5 --> timeout. e.g. this happens when the SCL_PIN or SDA_PIN are held low while the attiny is trying to write to the bus. 
    blink(ERROR_INDICATOR_LED, e, 100, 300);
    if (e == 5) resetI2CBus();
}

// ------------------------------ I2C Functions ------------------------------

/* 
write to the config register of the MAX17048, setting it to 0x975F, which is the default setting (0x971C) plus:
- ALSC = 1, which allows the SOC alert to pull the ~ALRT pin low when they trigger
- ATHD = 11111b, which sets the SOC detection threshold to 1%
*/
uint8_t setConfig() {
    uint8_t msb = 0x97;
    uint8_t lsb = 0x5F;
    Wire.beginTransmission(ADDR_MAX17048);
    Wire.write(REG_MAX17048_CONFIG); // CONFIG register address
    Wire.write(msb); // Write high byte
    Wire.write(lsb); // Write low byte
    uint8_t err = Wire.endTransmission();
    delay(10);
    displayError(err);
    return err;
}

// sends an I2C message to the controller
uint8_t writeToController(uint8_t _register, uint8_t data) {
    Wire.beginTransmission(ADDR_CONTROLLER);
    Wire.write(_register);
    Wire.write(data);
    uint8_t err = Wire.endTransmission();
    delay(10); // doesn't seem to work without this delay. why?!?!
    return err;
}

/*
Resets the I2C bus by disabling the TWI module, pulling the SDA and SCL pins high, and clocking out a few pulses on SCL, then re-enabling the module.
We found that if the controller is partially connected such that GND and the I2C lines are in contact but 
V+ isn't (or the controller is damaged or something), the controller will pull the bus lines low, which causes the ATtiny to throw a time out error.
Subsequent attempts to write to the bus also time out, but resetting the bus fixes it.
*/
void resetI2CBus() {
  // disable the module
  Wire.end();

  pinMode(SDA_PIN, OUTPUT);
  pinMode(SCL_PIN, OUTPUT);
  digitalWrite(SDA_PIN, HIGH);
  digitalWrite(SCL_PIN, HIGH);

  // generate clock pulses on SCL_PIN
  for (int i = 0; i < 10; i++) {
      digitalWrite(SCL_PIN, LOW);
      delayMicroseconds(10);
      digitalWrite(SCL_PIN, HIGH);
      delayMicroseconds(10);
  }

  // Re-enable the bus
  Wire.begin();
  delay(10);
}