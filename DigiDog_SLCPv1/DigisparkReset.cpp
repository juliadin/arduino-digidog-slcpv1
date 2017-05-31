#include <Arduino.h>

void DigisparkReset(void) {
  noInterrupts(); // disable interrupts which could mess with changing prescaler
  CLKPR = 0b10000000; // enable prescaler speed change
  CLKPR = 0; // set prescaler to default (16mhz) mode required by bootloader
  void (*ptrToFunction)(); // allocate a function pointer
  ptrToFunction = 0x0000; // set function pointer to bootloader reset vector
  (*ptrToFunction)(); // jump to reset, which bounces in to bootloader
}
