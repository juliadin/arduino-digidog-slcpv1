// DigiDog - Digispark based USB Hardware Watchdog
// see https://wiki.jjim.de/projects:digidog for details and documentation
// Code is licensed under the GPLv3 as provided here: https://www.gnu.org/licenses/gpl-3.0.txt or provided with this file
// Copyright 2017 by Julia Brunenberg

void reset_target(void);
void power_cycle_target(void);
void reset_timer(void);
void sanitize_timer(void);
void iosetup(void);
void update_eeprom(void);
