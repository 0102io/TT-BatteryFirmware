#include "Arduino.h"
#include "avr/sleep.h"
#include "avr/interrupt.h"
#include <Adafruit_MAX1704X.h>
#include <utils.h>

volatile bool buttonPressed = false;
volatile bool statAsserted = false; // "STAT" pin is exerted (pulled high) by connected devices to let the battery pack know that they are there
volatile bool socChanged = false; // State of charge alert flag, set when we receive an SOC alert from the fuel gauge
float percent = 0; // remaining battery percent, estimated by the fuel gauge

Adafruit_MAX17048 lipo; // I2C object for communicating with the fuel gauge

void socGetUpdate() {
  delay(10);
  percent = lipo.cellPercent();
  delay(1);
  socChanged = false;
  lipo.clearAlertFlag(MAX1704X_ALERTFLAG_SOC_CHANGE); // clear the alert flag in the status register
  delay(1);
  setConfig(); // this clears the alert bit in the config register (needed to pull the alert pin high again)
}

void setup() {
  delay(100); // let fuel gauge have some time to start up before we reset it
  // LED indicator pins, used for displaying the battery state of charge
  pinMode(LED1, OUTPUT);
  digitalWrite(LED1, HIGH);
  pinMode(LED2, OUTPUT);
  digitalWrite(LED2, HIGH);
  pinMode(LED3, OUTPUT);
  digitalWrite(LED3, HIGH);
  pinMode(LED4, OUTPUT);
  digitalWrite(LED4, HIGH);

  // alternate pins for reading the button / STAT / SOC alert pins
  pinMode(PIN_PA7, INPUT);
  pinMode(PIN_PB3, INPUT);
  pinMode(PIN_PC1, INPUT);

  // Hardware reset pin for the fuel gauge, active high
  pinMode(QSTRT_PIN, OUTPUT);
  digitalWrite(QSTRT_PIN, LOW);

  // set unused pins to OUTPUT to save power; see https://github.com/SpenceKonde/megaTinyCore/blob/c7afbb3161086edb54112005df15e4a1db84bf16/megaavr/statras/PowerSave.md
  pinMode(PIN_PA4, OUTPUT);
  pinMode(PIN_PB5, OUTPUT);
  pinMode(PIN_PC0, OUTPUT);
  pinMode(PIN_PC3, OUTPUT);

  // Set interupts for the button, stat, and SOC alert pins
  PORTC.PIN2CTRL = 0b00001011; // Interrupt on falling edge for BUTTON (PULLUPEN = 1, ISC = 011); note that this relies on pin 2 being an async pin; otherwise if pin 1 is used, this would have to be a pin change interrupt instead. see:https://github.com/SpenceKonde/megaTinyCore/blob/c7afbb3161086edb54112005df15e4a1db84bf16/megaavr/extras/Ref_PinInterrupts.md
  PORTA.PIN6CTRL = 0b00000010; // Interrupt on rising edge for STAT (PULLUPEN = 0, ISC = 010); note that this relies on pin 6 being an async pin; otherwise if pin 7 is used, this would have to be a pin change interrupt instead. see:https://github.com/SpenceKonde/megaTinyCore/blob/c7afbb3161086edb54112005df15e4a1db84bf16/megaavr/extras/Ref_PinInterrupts.md
  PORTB.PIN2CTRL = 0b00001011; // Interrupt on falling edge for ALERT (PULLUPEN = 1, ISC = 011); note that this relies on pin 2 being an async pin; otherwise if pin 3 is used, this would have to be a pin change interrupt instead. see:https://github.com/SpenceKonde/megaTinyCore/blob/c7afbb3161086edb54112005df15e4a1db84bf16/megaavr/extras/Ref_PinInterrupts.md

  if (!lipo.begin(&Wire)) resetI2CBus(); // note: the adafruit begin() function sends a reset command which makes the IC fully re-estimate battery level
  delay(10);
  setConfig();

  delay(10);
  percent = lipo.cellPercent();
  if ( percent == 0) { // if there was an issue with the I2C bus, battery percent is usually still at 0%, so reset the bus and try reading again if that's the case
    resetI2CBus();
    percent = lipo.cellPercent();
  }

  // disable ADC to save power; see https://github.com/SpenceKonde/megaTinyCore/blob/c7afbb3161086edb54112005df15e4a1db84bf16/megaavr/extras/PowerSave.md
  ADC0.CTRLA &= ~ADC_ENABLE_bm;
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
}

void loop() {
  sleep_cpu(); // sleep the cpu after we have acted on whatever interrupt woke us up.

  // STAT pin woke us up, display the charging animation
  if (statAsserted) {
    delay(10); // wait a bit and read pin to make sure it's still high, to ignore blips
    if (!digitalRead(STAT_PIN)) {
      statAsserted = false;
      return;
    }
    
    while (digitalRead(STAT_PIN)) { // continue to display the charging animation until the charger is disconnected or charging is complete (STAT will be pulled low in either case)
      if (socChanged) socGetUpdate();
      displayChargingStatus(percent);
    }
    statAsserted = false;
  }

  // button press woke us up; display the battery SOC on the indicator LEDs
  else if (buttonPressed) {
    if (socChanged) {
      socGetUpdate();
    }
    displayLevel(percent);
    delay(3000);
    turnOffLeds();
    buttonPressed = false;
  }
}

// ISR to handle button interrupt
ISR(PORTC_PORT_vect) {
  byte flags = PORTC.INTFLAGS;
  PORTC.INTFLAGS = flags; // Clear flags

  if (flags & (1 << 2)) { // button is on pin 2, so check if that pin caused the interrupt
    buttonPressed = true;
  }
}

// ISR to handle status pin interrupt
ISR(PORTA_PORT_vect) {
  byte flags = PORTA.INTFLAGS;
  PORTA.INTFLAGS = flags; // Clear flags

  if (flags & (1 << 6)) { // stat is on pin 6, so check if that pin caused the interrupt
    statAsserted = true;
  }
}

// ISR to handle alert interrupt
ISR(PORTB_PORT_vect) {
  byte flags = PORTB.INTFLAGS;
  PORTB.INTFLAGS = flags; // Clear flags

  if (flags & (1 << 2)) { // alert is on pin 2, so check if that pin caused the interrupt
    socChanged = true;
  }
}