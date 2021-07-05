// DigiDog - Digispark based USB Hardware Watchdog
// see https://wiki.jjim.de/projects:digidog for details and documentation
// Code is licensed under the GPLv3 as provided here: https://www.gnu.org/licenses/gpl-3.0.txt or provided with this file
// Copyright 2017 by Julia Brunenberg

#include <DigiDog_SLCPv1.hpp>
#include <Arduino.h>
#include <DigiCDC.h>
#include <DigiDog_config.h>
#include <DigiDog_globals.h>


void status(void) {
  // current timer value
  SerialUSB.print(F("C:"));
  SerialUSB.println( state.timer, DEC);

  // timer start value
  SerialUSB.print(F("S:"));
  SerialUSB.println( eeprom.config.timer_start, DEC);

  // Is timer armed and running?
  SerialUSB.print(F("A:"));
  SerialUSB.println(state.armed, DEC);

  // Has watchdog fired since last start?
  SerialUSB.print(F("F:"));
  SerialUSB.println(state.fired, DEC);

  // Is the watchdog timer locked?
  SerialUSB.print(F("J:"));
  SerialUSB.println(state.locked, DEC);

  // How often has the watchdog fired since the last EEPROM clear?
  SerialUSB.print(F("L:"));
  SerialUSB.println(eeprom.counters.fired_counter, DEC);

  // How many times to press # before reboot?
  SerialUSB.print(F("#:"));
  SerialUSB.println(state.reboot_in, DEC);

  return;
}

void config(void) {
  // T: ROM timer start value (active after EEPROM reset)
  SerialUSB.print(F("T:"));
  SerialUSB.println(ROM_TIMER_START, DEC);

  // S: Runtime timer start value (modified by +/-)
  SerialUSB.print(F("S:"));
  SerialUSB.println(eeprom.config.timer_start, DEC);

  // K: Internal Watchdog timer
  SerialUSB.print(F("K:"));
  SerialUSB.println(state.int_wdt, DEC);

  // M: ROM Method of recovery
  SerialUSB.print(F("M:"));
  SerialUSB.println(POWER_CYCLE_ON_TIMEOUT, DEC);

  // N: Method of recovery
  SerialUSB.print(F("N:"));
  SerialUSB.println(eeprom.config.power_cycle_on_timeout, DEC);

  // H: Hardware Pin configuration
  SerialUSB.print(F("H:"));
  SerialUSB.print(RESET, DEC);
  SerialUSB.print(F(","));
  SerialUSB.print(POWER, DEC);
  SerialUSB.print(F(","));
  SerialUSB.println(LED, DEC);

  // I: Hardware Pin States
  SerialUSB.print(F("I:"));
  SerialUSB.print(RESET_LINE_ON, DEC);
  SerialUSB.print(F(","));
  SerialUSB.print(RESET_LINE_OFF, DEC);
  SerialUSB.print(F(" "));
  SerialUSB.print(POWER_LINE_ON, DEC);
  SerialUSB.print(F(","));
  SerialUSB.println(POWER_LINE_OFF, DEC);

  // Z: Power cycle timing values
  SerialUSB.print(F("Z:"));
  SerialUSB.print(POWER_OFF_TIME, DEC);
  SerialUSB.print(F(","));
  SerialUSB.print(POWER_SLEEP_TIME, DEC);
  SerialUSB.print(F(","));
  SerialUSB.println(POWER_ON_TIME, DEC);

  // R: Reset timing
  SerialUSB.print(F("R:"));
  SerialUSB.println(RESET_TIME, DEC);
}

void version(void) {
  SerialUSB.print(F("V:"));
  SerialUSB.println(VERSION, DEC);
  SerialUSB.print(F("U:"));
  SerialUSB.println(UNIT_ID, DEC);
  SerialUSB.print(F("O:"));
  SerialUSB.println(eeprom.device.serial, HEX);
}
