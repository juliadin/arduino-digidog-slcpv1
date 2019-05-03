// DigiDog - Digispark based USB Hardware Watchdog
// see https://wiki.jjim.de/projects:digidog for details and documentation
// Code is licensed under the GPLv3 as provided here: https://www.gnu.org/licenses/gpl-3.0.txt or provided with this file
// Copyright 2017 by Julia Brunenberg

#include <Arduino.h>
#include <DigiDog_globals.h>
#include <DigiDog_config.h>
#include <DigiDog_EEPROM.hpp>
#include <EEPROM.h>

Eeprom_content init_eeprom(void){
  Eeprom_content clean_eeprom;
  clean_eeprom.magic = EEPROM_MAGIC;
  clean_eeprom.version = EEPROM_VERSION;
  clean_eeprom.counters.fired_counter = 0;
  clean_eeprom.config.timer_start = ROM_TIMER_START;
  clean_eeprom.config.power_cycle_on_timeout = POWER_CYCLE_ON_TIMEOUT;
  clean_eeprom.device.serial = DEVICE_SERIAL;
  EEPROM.put( EEPROM_STRUCT_ADDRESS, clean_eeprom);
  return clean_eeprom;
}

Eeprom_content read_eeprom(void){
  Eeprom_content my_eeprom;
  EEPROM.get(EEPROM_STRUCT_ADDRESS, my_eeprom);
  if ( my_eeprom.magic == EEPROM_MAGIC && my_eeprom.version == EEPROM_VERSION ) {
    return my_eeprom;
  } else {
    return init_eeprom();
  }
}
