// DigiDog - Digispark based USB Hardware Watchdog
// see https://wiki.jjim.de/projects:digidog for details and documentation
// Code is licensed under the GPLv3 as provided here: https://www.gnu.org/licenses/gpl-3.0.txt or provided with this file
// Copyright 2017 by Julia Brunenberg

#include <Arduino.h>
#include <DigiCDC.h>
#include <DigiDog_config.h>

void reset_target(void){
  digitalWrite(LED, HIGH);
  digitalWrite(RESET, RESET_LINE_ON);
  SerialUSB.delay(RESET_TIME);
  digitalWrite(RESET, RESET_LINE_OFF);
  digitalWrite(LED, LOW);
}

void power_cycle_target(void){
  // Power the system off if configured
  digitalWrite(LED, HIGH);
  if (POWER_OFF_TIME > 0) {
    digitalWrite(POWER, POWER_LINE_ON);
    SerialUSB.delay(POWER_OFF_TIME);
    digitalWrite(POWER, POWER_LINE_OFF);
  }
  SerialUSB.delay(POWER_SLEEP_TIME);
  if (POWER_ON_TIME > 0) {
    digitalWrite(POWER, POWER_LINE_ON);
    SerialUSB.delay(POWER_ON_TIME);
    digitalWrite(POWER, POWER_LINE_OFF);
  }
  digitalWrite(LED, LOW);
}
