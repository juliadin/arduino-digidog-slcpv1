// DigiDog - Digispark based USB Hardware Watchdog
// see https://wiki.jjim.de/projects:digidog for details and documentation
// Code is licensed under the GPLv3 as provided here: https://www.gnu.org/licenses/gpl-3.0.txt or provided with this file
// Copyright 2017 by Julia Brunenberg

// START OF CONFIGURATION
// Initial startup configuration.

//timer_start will be the value, the timer is reset to. 1200 will give you about 123s of rundown time.
#define ROM_TIMER_START 1200

//POWER_CYCLE_ON_TIMEOUT defines, if the system is going to be reset or powercycled in case of a timer timeout
// 0 - reset is triggered on timeout
// 1 - power cycle is triggered on timeout (DigiDog MUST be on 5VSB+ otherwise the device won't turn on again)
#define POWER_CYCLE_ON_TIMEOUT 0

// ALLOW_... configures, if the respective command is allowed.
// #define ALLOW_FIRED_COUNTER_RESET
#define ALLOW_RECOVERY_MODE_CHANGE
// #define ALLOW_DEBUG
#define ALLOW_TIMER_CHANGE
// #define ALLOW_TIMER_STOP  // means that timer can not be stopped if locked with "L" command
#define ALLOW_EEPROM_UPDATE

// Board configuration. If your LED does not start to blink after enabling watchdog
// swap try 0 or 1 for LED and adjust the other settings
#define LED 1
#define RESET 0
#define POWER 2

// Reset configuration. If your reset levels need replacing, then do it here. Also choose the time for the
// reset here. 1000 will most likely give you ~1s of reset.
#define RESET_TIME 1000
#define RESET_LINE_ON  LOW
#define RESET_LINE_OFF HIGH

// Power configuration. OFF, SLEEP and ON are the times to press the power button and to wait between presses. A normal
// ATX system will power off after around 4 seconds of power button press (POWER_OFF_TIME) and will be ready to power
// up again after about a second (POWER_SLEEP_TIME). It can  usually be powered up again by a short press of the power
// button (POWER_ON_TIME);
// Setting any of the values OFF or ON to 0 disables that step.
#define POWER_OFF_TIME 5000
#define POWER_SLEEP_TIME 2000
#define POWER_ON_TIME 1000
#define POWER_LINE_ON  LOW
#define POWER_LINE_OFF HIGH

// Internal Watchdog start value
#define INTERNAL_WATCHDOG_START 18000

#define REBOOT_AFTER_PRESSES 3

#define TIMER_SET_STEP 100
#define TIMER_SET_MIN 100
#define TIMER_SET_MAX 65000

// EEPROM Magic to detect if flash is initialized - dd09 for DigiDog
#define EEPROM_MAGIC 0xdd09
#define EEPROM_VERSION 1
#define EEPROM_STRUCT_ADDRESS 0x00

// END OF CONFIGURATION

#define VERSION 2
#define UNIT_ID 1
#define DEVICE_SERIAL 0x10000002
