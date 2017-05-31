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
