#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "pico/multicore.h"

#include "displays.h"

int sentenceCounter = 0;

char sentence[100];
char latestRMC[100];

void processTime() {
  // Separate Time from NMEA
  char processString[100];
  strcpy(processString, latestRMC);

  strtok(processString, ",");
  char *rawTime = strtok(NULL, ",");

  for (int i = 0; i < 6; i++) {
    strtok(NULL, ",");
  }

  char *date = strtok(NULL, ",");
  // Get only hours
  char *timeData = strtok(rawTime, ".");

  setTime(timeData, date);
}

int main() {
  stdio_init_all();

  // Init Display Pins
  for (int i = 2; i < 12; i++) {
    gpio_init(i);
    gpio_set_dir(i, GPIO_OUT);
    gpio_put(i, 1);
  }

  gpio_init(15);
  gpio_init(26);
  gpio_init(27);

  gpio_set_dir(15, GPIO_OUT);
  gpio_set_dir(26, GPIO_OUT);
  gpio_set_dir(27, GPIO_OUT);

  gpio_set_function(0, GPIO_FUNC_UART);
  gpio_set_function(1, GPIO_FUNC_UART);

  uart_init(uart0, 115200);

  initDisplay();

  while (true) {
  if (uart_is_readable(uart0)) {
      // Get current character and see if it is the start of a code
      char currentChar = uart_getc(uart0);

      if (currentChar == '$') {
        // Loop to read the whole code
        while (true) {
          char readChar = uart_getc(uart0);
          sentence[sentenceCounter++] = readChar;

          // When code ends set the sentence, expected value:
          //GNRMC,164552.00,A,1628.46113,S,05434.60482,W,0.181,,180625,,,A*62
          if (readChar == '\n') {
            if (sentence[2] == 'R') {
              /* printf("%s", sentence); */
              strcpy(latestRMC, sentence);
              processTime();
            }

            sentenceCounter = 0;
            memset(sentence, 0, sizeof(char) * 100);
            break;
          }
        }
      }
    }
  }

  return 0;
}
