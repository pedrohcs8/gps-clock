#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"

int serial_pin[2] = { 12, 15 };
int shift_clock_pin[2] = { 14, 27 };
int register_clock_pin[2] = { 13, 26 };

int timezone = -4; // UTC Timezone

uint8_t numberTable[11] = { 0xFC, 0x60, 0xDA, 0xF2, 0x66, 0xB6, 0xBE, 0xE0, 0xFE, 0xF6, 0x1 }; // Last entry represent decimal point

int digitsTop[6] = { 2, 3, 4, 5, 6, 7 };
int digitsBottom[4] = { 8, 9, 10, 11 };

char timeString[7];
char dateString[5];

void displayNumber(int number, int display) {
  uint8_t segments = numberTable[number];

  gpio_put(register_clock_pin[display], 0);

  for (int i = 0; i < 8; i++) {
    bool segment = (segments >> i) & 1;

    // Set Data
    gpio_put(serial_pin[display], segment);
    sleep_us(1);

    // Shift Register Clock
    gpio_put(shift_clock_pin[display], 1);

    //Save clock

    sleep_us(1);

    gpio_put(serial_pin[display], 0);
    gpio_put(shift_clock_pin[display], 0);
  }

  gpio_put(register_clock_pin[display], 1);
}

void noNumber(int display) {
  gpio_put(serial_pin[display], 0);
  gpio_put(register_clock_pin[display], 0);

  for (int i = 0; i < 8; i++) {
    gpio_put(shift_clock_pin[display], 1);
    sleep_us(1);
    gpio_put(shift_clock_pin[display], 0);
  }

  gpio_put(register_clock_pin[display], 1);
}

void topNumbersLoop() {
  // Get number from each digit
  for (int digit = 0; digit < 6; digit++) {
    // Set number and turn display and shift register on
    displayNumber(timeString[digit] - '0', 0);
    gpio_put(digitsTop[digit], 0);
    sleep_us(200);

    // Turn shift register off and turn off display
    noNumber(0);
    gpio_put(digitsTop[digit], 1);

    sleep_us(400);
  }
}

void bottomNumbersLoop() {
  for (int digit = 0; digit < 4; digit++) {
    // Set number and turn display and shift register on
    displayNumber(dateString[digit] - '0', 1);
    gpio_put(digitsBottom[digit], 0);
    sleep_us(300);

    // Turn shift register off and turn off display
    noNumber(1);
    gpio_put(digitsBottom[digit], 1);

    sleep_us(400);
  }

  // Turn Decimal Point ON
  for (int dp = 2; dp < 4; dp++) {
    displayNumber(10, 1);
    gpio_put(digitsBottom[dp], 0);
    sleep_us(200);

    // Turn shift register off and turn off display
    noNumber(1);
    gpio_put(digitsBottom[dp], 1);

    sleep_us(400);
  }
}

void setTime(char *gpsTime, char *gpsDate) {
  // Separate wanted time and date for parsing to int
  char utcHoursChar[3];
  char utcMinsChar[3];
  char utcSecsChar[3];

  char utcDayChar[3];
  char utcMonthChar[3];

  utcHoursChar[0] = gpsTime[0];
  utcHoursChar[1] = gpsTime[1];
  utcHoursChar[2] = '\0';

  utcMinsChar[0] = gpsTime[2];
  utcMinsChar[1] = gpsTime[3];
  utcMinsChar[2] = '\0';

  utcSecsChar[0] = gpsTime[4];
  utcSecsChar[1] = gpsTime[5];
  utcSecsChar[2] = '\0';

  utcDayChar[0] = gpsDate[0];
  utcDayChar[1] = gpsDate[1];
  utcDayChar[2] = '\0';

  utcMonthChar[0] = gpsDate[2];
  utcMonthChar[1] = gpsDate[3];
  utcMonthChar[2] = '\0';

  // Parse date and time to int for transforming into tm time
  int utcHours = atoi(utcHoursChar);
  int utcMins = atoi(utcMinsChar);
  int utcSecs= atoi(utcSecsChar);

  int utcDay = atoi(utcDayChar);
  int utcMonth = atoi(utcMonthChar);

  // Construct tm time
  struct tm utcTime;
  utcTime.tm_hour = utcHours + timezone;
  utcTime.tm_min = utcMins;
  utcTime.tm_sec = utcSecs;

  // Set random year (BDay year :))
  utcTime.tm_year = 708;
  utcTime.tm_mon = utcMonth - 1;
  utcTime.tm_mday = utcDay;
  utcTime.tm_isdst = -1;

  // Correct time with mktime and convert back to struct with localtime
  time_t correctedTime = mktime(&utcTime);
  struct tm *timezoneTime = localtime(&correctedTime);

  // Format time and send out
  char processedTime[7];
  char processedDate[5];

  // Output time and date back to strings for displaying
  strftime(processedTime, 7, "%H%M%S", timezoneTime);
  strftime(processedDate, 7, "%d%m", timezoneTime);

  strcpy(timeString, processedTime);
  strcpy(dateString, processedDate);
}
