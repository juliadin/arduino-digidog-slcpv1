// DigiDog - Digispark based USB Hardware Watchdog
// see https://wiki.jjim.de/projects:digidog for details and documentation
// Code is licensed under the GPLv3 as provided here: https://www.gnu.org/licenses/gpl-3.0.txt or provided with this file
// Copyright 2017 by Julia Brunenberg

// Configuration can be done in DigiDog_config.h

#include <DigiDog_SLCPv1.hpp>
#include <Arduino.h>
#include <DigiCDC.h>
#include <DigisparkReset.hpp>
#include <DigiDog_config.h>
#include <DigiDog_globals.h>
#include <DigiDog_target_commands.hpp>
#include <DigiDog_output.hpp>
#include <DigiDog_EEPROM.hpp>
#include <EEPROM.h>

Eeprom_content eeprom;
State_content state;

void sanitize_timer(void) {
  bool timer_limited = 0;
  if (eeprom.config.timer_start < TIMER_SET_MIN){
    eeprom.config.timer_start = TIMER_SET_MIN;
    timer_limited = 1;
  }
  if (eeprom.config.timer_start >= TIMER_SET_MAX){
    eeprom.config.timer_start = TIMER_SET_MAX;
    timer_limited = 1;
  }
  if (timer_limited == 0) {
    // only update eeprom with new timer values if the timer is not maxed out in either
    // direction. This is to improve flash lifetime
  }
}

void update_eeprom(void) {
  EEPROM.put(EEPROM_STRUCT_ADDRESS, eeprom);
}

void reset_timer(void) {
  if (state.armed > 0) {
    state.timer = eeprom.config.timer_start;
  }
}

void iosetup(void) {

  // Initialize LED
  pinMode(LED, OUTPUT);
  digitalWrite(LED,LOW);

  // Initialize Reset line
  pinMode(RESET, OUTPUT);
  digitalWrite(RESET,RESET_LINE_OFF);

  // Initialize Power line
  pinMode(POWER, OUTPUT);
  digitalWrite(POWER,POWER_LINE_OFF);

  // Close USB Connection if established
  SerialUSB.end();
  // open it again
  SerialUSB.begin();
}

void setup(void) {
  eeprom = read_eeprom();
  state.armed = 0;
  state.fired = 0;
  state.timer = 0;
  state.int_wdt = INTERNAL_WATCHDOG_START;
  state.timer=eeprom.config.timer_start;
  state.reboot_in = REBOOT_AFTER_PRESSES;

  iosetup();
}

void loop(void) {
  // This is to carry over the fired counter in case this is necessary
  int old_fired_counter = 0;
  int reboot_counter;

  // Try to read a byte from serial and evaulate
  while (SerialUSB.available() > 0) {
    digitalWrite(LED,HIGH);
    int input = SerialUSB.read();
    if ( input != '#' ) {
      state.reboot_in = REBOOT_AFTER_PRESSES;
    }
    switch (input) {
      // Debug commands

      // force reset trigger
      case 'F':
#ifdef ALLOW_DEBUG
        SerialUSB.println(F("W:RST"));
        reset_target();
#else
        SerialUSB.println(F("Q:F"));
#endif
        break;

      // force power cycle trigger
      case 'P':
#ifdef ALLOW_DEBUG
        SerialUSB.println(F("W:PWR"));
        power_cycle_target();
#else
        SerialUSB.println(F("Q:P"));
#endif
        break;

      // force watchdog device reboot
      //  !! do not use for firmware update !!
      case '#':
#ifdef ALLOW_DEBUG_WATCHDOG_REBOOT
        if ( state.reboot_in <= 1 ) {
          DigisparkReset();
        } else {
          state.reboot_in -= 1;
          SerialUSB.print(F("#:"));
          SerialUSB.println(state.reboot_in, DEC);
        }
#else
        SerialUSB.println(F("Q:#"));
#endif
        break;

      // force reinit of USB-Stack - also reinitialize all
      // I/O Ports.
      case '!':
        iosetup();
        break;

      // force reinit of EEPROM from ROM
      case '<':
#ifndef ALLOW_FIRED_COUNTER_RESET
        old_fired_counter = eeprom.counters.fired_counter;
#endif
        eeprom = init_eeprom();
        reset_timer();
#ifndef ALLOW_FIRED_COUNTER_RESET
        // if resetting the fired counter is not permitted
        // carry it over. This might enable an attach with physical access
        eeprom.counters.fired_counter = old_fired_counter;
        update_eeprom();
#endif
        SerialUSB.println(F("P:<"));
        config();
        status();
        break;

       // force writing current runtime configuration to EEPROM
      case '>':
#ifdef ALLOW_EEPROM_UPDATE
        update_eeprom();
        SerialUSB.println(F("P:>"));
#else
        SerialUSB.println(F("Q:>"));
#endif
        break;


      // Timer Commands

      // reduce a running timer to minimum, output status
      case '*':
        if (state.armed > 0) {
          state.timer = TIMER_SET_MIN;
        }
        status();
        break;

      // reset timer and disarm Watchdog
      case 'x':
#ifdef ALLOW_TIMER_STOP
        reset_timer();
        state.armed = 0;
#else
        SerialUSB.println(F("Q:L"));
#endif
        break;

      // reset timer and disarm Watchdog
      case 'x':
        if ( state.locked == 0) {
          reset_timer();
          state.armed = 0;
          update_eeprom();
        } else {
          SerialUSB.println(F("Q:x"));
        }
        status();
        break;

      // arm watchdog, reset and start timer
      case 'X':
        state.armed = 1;
        reset_timer();
        state.fired = 0;
        status();
        break;

      // Reset timer. to
      case 'R':
        reset_timer();
        SerialUSB.println(F("P:R"));
        break;

      // Reset counter that keeps track of the times the watchdog fired
      case '0':
#ifdef ALLOW_FIRED_COUNTER_RESET
        eeprom.counters.fired_counter=0;
        update_eeprom();
        SerialUSB.print(F("L:"));
        SerialUSB.println(eeprom.counters.fired_counter, DEC);
#else
        SerialUSB.println(F("Q:0"));
#endif
        break;

      // change recovery method to "reset"
      case 'm':
#ifdef ALLOW_RECOVERY_MODE_CHANGE
        eeprom.config.power_cycle_on_timeout = 0;
        SerialUSB.print(F("N:"));
        SerialUSB.println(eeprom.config.power_cycle_on_timeout, DEC);
#else
        SerialUSB.println(F("Q:m"));
#endif
        break;

      // Change recovery method to "power-cycle"
      case 'M':
#ifdef ALLOW_RECOVERY_MODE_CHANGE
        eeprom.config.power_cycle_on_timeout = 1;
        SerialUSB.print(F("N:"));
        SerialUSB.println(eeprom.config.power_cycle_on_timeout, DEC);
#else
        SerialUSB.println(F("Q:M"));
#endif
        break;

      // Decrement timer start value
      case '-':
#ifdef ALLOW_TIMER_CHANGE
        eeprom.config.timer_start = eeprom.config.timer_start - TIMER_SET_STEP;
        sanitize_timer();
        reset_timer();
        SerialUSB.print(F("S:"));
        SerialUSB.println(eeprom.config.timer_start, DEC);
#else
        SerialUSB.println(F("Q:-"));
#endif
        break;

      // Increment timer start value
      case '+':
#ifdef ALLOW_TIMER_CHANGE
        eeprom.config.timer_start = eeprom.config.timer_start + TIMER_SET_STEP;
        sanitize_timer();
        reset_timer();
        SerialUSB.print(F("S:"));
        SerialUSB.println(eeprom.config.timer_start, DEC);
#else
        SerialUSB.println(F("Q:+"));
#endif
        break;

      // Query commands

      // output firmware and current configuration
      case 'C':
        config();
        break;

      // output operational status
      case 'S':
        status();
        break;

      // output Version and device identifier
      case 'V':
        version();
        break;

      // Output all the commands that are forbidden by firmware configuration
      // to establish a "End of list" marker, this always ends with the message
      // that this command itself is forbidden
      case 'Q':
#ifndef ALLOW_TIMER_CHANGE
        SerialUSB.println(F("Q:-"));
        SerialUSB.println(F("Q:+"));
#endif
#ifndef ALLOW_DEBUG
        SerialUSB.println(F("Q:F"));
        SerialUSB.println(F("Q:P"));
        SerialUSB.println(F("Q:#"));
#endif
#ifndef ALLOW_RECOVERY_MODE_CHANGE
        SerialUSB.println(F("Q:m"));
        SerialUSB.println(F("Q:M"));
#endif
#ifndef ALLOW_FIRED_COUNTER_RESET
        SerialUSB.println(F("Q:0"));
#endif
#ifndef ALLOW_TIMER_STOP
        if (state.locked) {
          SerialUSB.println(F("Q:x"));
        }
#endif
#ifndef ALLOW_EEPROM_UPDATE
        SerialUSB.println(F("Q:>"));
#endif
        SerialUSB.println(F("Q:Q"));
        break;

      default:
        SerialUSB.print(F("X:"));
        SerialUSB.println(input, DEC);
        break;
    }

    // Reset the watchdog on serial ingress communication
    state.int_wdt=INTERNAL_WATCHDOG_START;
    digitalWrite(LED,LOW);
  }

  // decrement internal watchdog to reset USB Stack when device was not spoken to for a long time
  // this assumes that the internal USB stack is less stable than it should be and to recover from user error
  // on remote devices.
  state.int_wdt--;

  // If the timer is armed, run timer routine
  if ( state.armed > 0) {
    if (state.timer == 0) {

      // if power_cycle_on_timeout is set power cycle the target, use reset otherwise
      if ( eeprom.config.power_cycle_on_timeout > 0 ) {
        power_cycle_target();
      } else {
        reset_target();
      }

      // Disable Watchdog after it fired
      state.armed = 0;

      // Set "fired" bit
      state.fired = 1;

      // increment fired counter if it is not overflowing
      if (eeprom.counters.fired_counter < 65535) {
        eeprom.counters.fired_counter++;
        update_eeprom();
      }
    } else {
      state.timer--;
    }
    // toggle LED while timer is running
    digitalWrite(LED, !digitalRead(LED));

    // always reset internal watchdog when armed, so we do do reset no matter what.
    // int_wdt=INTERNAL_WATCHDOG_START;
  } else {
    // turn LED off to assure defined state while timer is off.
    digitalWrite(LED, LOW);
  }
  SerialUSB.delay(100);
  if (state.int_wdt == 0) {
    setup();
  }
}
