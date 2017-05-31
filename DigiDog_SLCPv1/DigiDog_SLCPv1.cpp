// DigiDog - Digispark based USB Hardware Watchdog
// see https://wiki.jjim.de/projects:digidog for details and documentation
// Code is licensed under the GPLv3 as provided here: https://www.gnu.org/licenses/gpl-3.0.txt or provided with this file
// Copyright 2017 by Joel Brunenberg

// Configuration can be done in DigiDog_config.h

#include <DigiDog_SLCPv1.hpp>
#include <Arduino.h>
#include <DigiCDC.h>
#include <DigisparkReset.hpp>
#include <DigiDog_config.h>
#include <DigiDog_globals.h>
#include <DigiDog_target_commands.hpp>
#include <DigiDog_output.hpp>

#define VERSION 2
#define UNIT_ID 1

unsigned int timer_start=ROM_TIMER_START;
unsigned int timer=timer_start;
unsigned int fired_counter = 0;
bool fired = 0;
bool armed = ARMED_ON_BOOT;
bool power_cycle_on_timeout = POWER_CYCLE_ON_TIMEOUT;
unsigned int int_wdt = INTERNAL_WATCHDOG_START;

void sanitize_timer(void) {
  if (timer_start < TIMER_SET_MIN){
    timer_start = TIMER_SET_MIN;
  }
  if (timer_start >= TIMER_SET_MAX){
    timer_start = TIMER_SET_MAX;
  }
}


void reset_timer(void) {
  if (armed > 0) {
    timer = timer_start;
  }
}

void setup(void) {
  // Close USB Connection if established
  SerialUSB.end();
  // open it again
  SerialUSB.begin();

  // Initialize LED
  pinMode(LED, OUTPUT);
  digitalWrite(LED,LOW);

  // Initialize Reset line
  pinMode(RESET, OUTPUT);
  digitalWrite(RESET,RESET_LINE_OFF);

  // Initialize Power line
  pinMode(POWER, OUTPUT);
  digitalWrite(POWER,POWER_LINE_OFF);
}

void commands(void){
  // output supported commands

  // D is for Debug command (also for DANGER! ;))
  SerialUSB.println(F("W:Warning - be careful with the D: commands."));
  SerialUSB.println(F("D:fFpP#!"));

  // E is for Query commands
  SerialUSB.println(F("E:CSVQ?"));

  // G is for Timer commands
  SerialUSB.println(F("G:mMRxX0-+*"));
}

void loop(void) {

  // Try to read a byte from serial and evaulate
  while (SerialUSB.available() > 0) {
    digitalWrite(LED,HIGH);
    int input = SerialUSB.read();
    switch (input) {
      // Debug commands

      // force reset trigger
      case 'F':
#ifdef ALLOW_DEBUG_RESET
           SerialUSB.println(F("W:RST"));
           reset_target();
#else
           SerialUSB.println(F("Q:F"));
#endif
           break;
      // force power cycle trigger
      case 'P':
#ifdef ALLOW_DEBUG_POWER_CYCLE
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
           DigisparkReset();
#else
           SerialUSB.println(F("Q:#"));
#endif
           break;

      // force reinit of USB-Stack - also reinitialize all
      // I/O Ports.
      case '!':
           setup();
           break;


      // Timer Commands

      // reduce a running timer to minimum, output status
      case '*':
           if (armed > 0) {
             timer = TIMER_SET_MIN;
           }
           status();
           break;

      // reset timer and disarm Watchdog
      case 'x':
#ifdef ALLOW_TIMER_STOP
           reset_timer();
           armed = 0;
#else
           SerialUSB.println(F("Q:x"));
#endif
           status();
           break;

      // arm watchdog, reset and start timer
      case 'X':
           armed = 1;
           reset_timer();
           fired = 0;
           status();
           break;

      // Reset timer. to
      case 'R':
           reset_timer();
           break;

      // Reset counter that keeps track of the times the watchdog fired
      case '0':
#ifdef ALLOW_FIRED_COUNTER_RESET
           fired_counter=0;
           SerialUSB.print(F("L:"));
           SerialUSB.println(fired_counter, DEC);
#else
           SerialUSB.println(F("Q:0"));
#endif
           break;

      // change recovery method to "reset"
      case 'm':
#ifdef ALLOW_RECOVERY_MODE_CHANGE
           power_cycle_on_timeout = 0;
           SerialUSB.print(F("N:"));
           SerialUSB.println(power_cycle_on_timeout, DEC);
#else
           SerialUSB.println(F("Q:m"));
#endif
           break;

      // Change recovery method to "power-cycle"
      case 'M':
#ifdef ALLOW_RECOVERY_MODE_CHANGE
           power_cycle_on_timeout = 1;
           SerialUSB.print(F("N:"));
           SerialUSB.println(power_cycle_on_timeout, DEC);
#else
           SerialUSB.println(F("Q:M"));
#endif
           break;

      // Decrement timer start value
      case '-':
#ifdef ALLOW_TIMER_CHANGE
           timer_start = timer_start - TIMER_SET_STEP;
           sanitize_timer();
           reset_timer();
           SerialUSB.print(F("S:"));
           SerialUSB.println(timer_start, DEC);
#else
           SerialUSB.println(F("Q:-"));
#endif
           break;

      // Increment timer start value
      case '+':
#ifdef ALLOW_TIMER_CHANGE
           timer_start = timer_start + TIMER_SET_STEP;
           sanitize_timer();
           reset_timer();
           SerialUSB.print(F("S:"));
           SerialUSB.println(timer_start, DEC);
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
           SerialUSB.print(F("V:"));
           SerialUSB.println(VERSION, DEC);
           SerialUSB.print(F("U:"));
           SerialUSB.println(UNIT_ID, DEC);
           break;

      // Output all the commands that are forbidden by firmeware configuration
      // to establish a "End of list" marker, this always ends with the message
      // that this command itself is forbidden
      case 'Q':
#ifndef ALLOW_TIMER_CHANGE
           SerialUSB.println(F("Q:-"));
           SerialUSB.println(F("Q:+"));
#endif
#ifndef ALLOW_DEBUG_RESET
           SerialUSB.println(F("Q:F"));
#endif
#ifndef ALLOW_DEBUG_POWER_CYCLE
           SerialUSB.println(F("Q:P"));
#endif
#ifndef ALLOW_RECOVERY_MODE_CHANGE
           SerialUSB.println(F("Q:m"));
           SerialUSB.println(F("Q:M"));
#endif
#ifndef ALLOW_FIRED_COUNTER_RESET
           SerialUSB.println(F("Q:0"));
#endif
#ifndef ALLOW_DEBUG_WATCHDOG_REBOOT
           SerialUSB.println(F("Q:#"));
#endif
#ifndef ALLOW_TIMER_STOP
           SerialUSB.println(F("Q:x"));
#endif
           SerialUSB.println(F("Q:Q"));
           break;
      case '?':
           commands();
           break;
      default:
           SerialUSB.print(F("X:"));
           SerialUSB.println(input, DEC);
           break;
    }


    // Reset the watchdog on serial ingress communication
    int_wdt=INTERNAL_WATCHDOG_START;
    digitalWrite(LED,LOW);
  }

  // decrement internal watchdog to reset USB Stack when device was not spoken to for a long time
  // this assumes that the internal USB stack is less stable than it should be and to recover from user error
  // on remote devices.
  int_wdt--;

  // If the timer is armed, run timer routine
  if (armed > 0) {
    if (timer == 0) {

      // if power_cycle_on_timeout is set power cycle the target, use reset otherwise
      if ( power_cycle_on_timeout > 0 ) {
        power_cycle_target();
      } else {
        reset_target();
      }

      // Disable Watchdog after it fired
      armed = 0;

      // Set "fired" bit
      fired = 1;

      // increment fired counter if it is not overflowing
      if (fired_counter < 65535) {
        fired_counter++;
      }
    } else {
      timer--;
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
  if (int_wdt == 0) {
    setup();
  }
}
