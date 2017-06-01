// DigiDog - Digispark based USB Hardware Watchdog
// see https://wiki.jjim.de/projects:digidog for details and documentation
// Code is licensed under the GPLv3 as provided here: https://www.gnu.org/licenses/gpl-3.0.txt or provided with this file
// Copyright 2017 by Joel Brunenberg

#ifndef EEPROM_STRUCT_DEFINITION
#define EEPROM_STRUCT_DEFINITION

typedef struct Eeprom_content_struct {
  unsigned int magic;
  unsigned int version;
  struct counters {
    unsigned int fired_counter;
  } counters;
  struct config {
    unsigned int timer_start;
    bool power_cycle_on_timeout;
  } config;
  struct device {
    unsigned long serial;
  } device;
} Eeprom_content;

#endif

extern unsigned int timer;
extern bool fired;
extern bool armed;
extern unsigned int int_wdt;
extern Eeprom_content eeprom;
