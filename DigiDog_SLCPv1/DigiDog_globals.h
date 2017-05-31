// DigiDog - Digispark based USB Hardware Watchdog
// see https://wiki.jjim.de/projects:digidog for details and documentation
// Code is licensed under the GPLv3 as provided here: https://www.gnu.org/licenses/gpl-3.0.txt or provided with this file
// Copyright 2017 by Joel Brunenberg

extern unsigned int timer_start;
extern unsigned int timer;
extern unsigned int fired_counter;
extern bool fired;
extern bool armed;
extern bool power_cycle_on_timeout;
extern unsigned int int_wdt;
