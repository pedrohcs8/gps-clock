#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "display-driver.pio.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"

int serial_pin = 15;
int shift_clock_pin = 27;
int register_clock_pin = 26;

int timezone = -4; // UTC Timezone

uint8_t numberTable[11] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x7, 0x7F, 0x6F, 0x01 }; // Last entry represent decimal point
uint8_t numberTable2[11] = {0xFC, 0x60, 0xDA, 0xF2, 0x66, 0xB6, 0xBE, 0xE0, 0xFE, 0xF6, 0x01};

int digitsTop[6] = { 2, 3, 4, 5, 6, 7 };
int digitsBottom[4] = { 8, 9, 10, 11 };

char timeString[7];
char dateString[5];

bool topDisplayState = true;
int selectedTopDisplay = 0;

bool bottomDisplayDP = false;
bool bottomDisplayState = true;
bool decimalPointState = true;
int selectedDecimalPoint = 2;
int selectedBottomDisplay = 0;

static struct repeating_timer topDispTimer;
static struct repeating_timer bottomDispTimer;

void displayNumber(int number, int display) {
  if (display == 0) {
    uint8_t segments = numberTable[number];
    loadData(segments);
  } else {
    uint8_t segments = numberTable2[number];
    gpio_put(register_clock_pin, 0);

    for (int i = 0; i < 8; i++) {
      bool segment = (segments >> i) & 1;

      // Set Data
      gpio_put(serial_pin, segment);

      // Shift Register Clock
      gpio_put(shift_clock_pin, 1);

      //Save clock

      gpio_put(serial_pin, 0);
      gpio_put(shift_clock_pin, 0);
    }

    gpio_put(register_clock_pin, 1);
  }
}

void noNumber(int display) {
  gpio_put(serial_pin, 0);
  gpio_put(register_clock_pin, 0);

  for (int i = 0; i < 8; i++) {
    gpio_put(shift_clock_pin, 1);
    gpio_put(shift_clock_pin, 0);
  }

  gpio_put(register_clock_pin, 1);
}

bool runTopLoop(__unused repeating_timer_t *t) {
  if (topDisplayState == true) {
    // Set number and turn display and shift register on
    displayNumber(timeString[selectedTopDisplay] - '0', 0);
    gpio_put(digitsTop[selectedTopDisplay], 0);
    printf("%d\n", timeString[selectedTopDisplay] - '0');

    topDisplayState = false;
  } else {
    // Turn shift register off and turn off display
    loadData(0x00);
    gpio_put(digitsTop[selectedTopDisplay], 1);
    topDisplayState = true;
    selectedTopDisplay++;

    if (selectedTopDisplay > 5) {
      selectedTopDisplay = 0;
    }
  }

  return true;
}

bool runBottomLoop(__unused repeating_timer_t *t) {
  if (bottomDisplayDP == false) {
    // Main numbers loop
    if (bottomDisplayState == true) {
      displayNumber(dateString[selectedBottomDisplay] - '0', 1);
      gpio_put(digitsBottom[selectedBottomDisplay], 0);
      bottomDisplayState = false;
    } else {
      noNumber(1);
      gpio_put(digitsBottom[selectedBottomDisplay], 1);
      bottomDisplayState = true;
      selectedBottomDisplay++;

      if (selectedBottomDisplay > 3) {
        selectedBottomDisplay = 0;
        bottomDisplayDP = true;
      }
    }
  } else {
    //decimal point loop
    if (decimalPointState == true) {
      displayNumber(10, 1);
      gpio_put(digitsBottom[selectedDecimalPoint], 0);
      decimalPointState = false;
    } else {
      noNumber(1);
      gpio_put(digitsBottom[selectedDecimalPoint], 1);

      decimalPointState = true;
      selectedDecimalPoint++;

      if (selectedDecimalPoint > 3) {
        selectedDecimalPoint = 2;
        bottomDisplayDP = false;
      }
    }
  }

  return true;
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

void initDisplay() {
  init_display_driver(12);
  add_repeating_timer_us(400, runTopLoop, NULL, &topDispTimer);
  add_repeating_timer_us(400, runBottomLoop, NULL, &bottomDispTimer);
}
