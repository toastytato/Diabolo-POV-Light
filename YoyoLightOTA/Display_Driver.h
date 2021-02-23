#include <Adafruit_NeoPixel.h>
#include "Generated_Map.h"

#ifndef NUM_LEDS
#define NUM_LEDS      10 //radius (# of leds)
#endif

#ifndef NUM_SECTORS
#define NUM_SECTORS   50 //circumference (resolution in the polar axis)
#endif

#define abs(x) ((x)>0?(x):-(x)) //esp8266 doesn't have abs for floats for some reason, so this works for floats

#define LED_PIN       0
#define THETA         360 / NUM_SECTORS  //angle per sector

class DisplayDriverPOV
{
  private:

    Adafruit_NeoPixel Pixels = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

    const uint16_t cnt_per_cycle = NUM_SECTORS / 3;

    volatile uint32_t count = 0;
    volatile uint8_t curr_sector = 0;
    volatile uint8_t brightness = 255;

  public:
    DisplayDriverPOV();

    //
    void show_sector(bool flip);
    void show_line(int brightness);
    void show_boot_up_sequence();
    void cycle_display(int cycle_time);
    bool set_curr_sector(uint8_t value);
    uint8_t get_curr_sector();
    void increment_sector();
    void decrement_sector();
    void increment_count();
    void set_brightness(uint8_t value);
    float time_in_sector_ms(float dps);
};

DisplayDriverPOV::DisplayDriverPOV() {
  Pixels.begin();
}

void DisplayDriverPOV::show_sector(bool flip) {
  uint8_t r = (sin(count * 2 * PI / cnt_per_cycle) + 1) / 2 * brightness;
  uint8_t g = (sin(count * 2 * PI / cnt_per_cycle + (2 * PI / 3)) + 1) / 2 * brightness;
  uint8_t b = (sin(count * 2 * PI / cnt_per_cycle + (4 * PI / 3)) + 1) / 2 * brightness;

  //      uint8_t r = 0;
  //      uint8_t g = 0;
  //      uint8_t b = brightness;

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint8_t pixel_idx = i;
    if (flip) {
      pixel_idx = NUM_LEDS - i - 1;
    }
    //        uint8_t state = displayLed[curr_sector][pixel_idx]; //0 or 1
    uint8_t state = GENERATED_DISPLAY[curr_sector][pixel_idx];
    Pixels.setPixelColor(i, Pixels.Color(r * state, g * state, b * state));
  }
  Pixels.show();
}

void DisplayDriverPOV::show_line(int brightness) {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    Pixels.setPixelColor(i, Pixels.Color(0, brightness, 0));
  }
  Pixels.show();
}

void DisplayDriverPOV::show_boot_up_sequence() {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    Pixels.setPixelColor(i, Pixels.Color(0, 50, 0));
    Pixels.show();
    delay(25);
  }
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    Pixels.setPixelColor(i, Pixels.Color(0, 0, 0));
    Pixels.show();
    delay(25);
  }
}
void DisplayDriverPOV::cycle_display(int cycle_time) {
  for (uint8_t i = 0; i < NUM_SECTORS; i++) {
    for (uint8_t j = 0; j < NUM_LEDS; j++) {
      uint8_t state = GENERATED_DISPLAY[i][j];
      Pixels.setPixelColor(j, Pixels.Color(100 * state, 0, 0));
    }
    Pixels.show();
    delay(cycle_time / NUM_SECTORS);
  }
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    Pixels.setPixelColor(i, Pixels.Color(0, 0, 0));
  }
  Pixels.show();
}

bool DisplayDriverPOV::set_curr_sector(uint8_t value) {
  if (value > NUM_SECTORS) {
    return false;
  } else {
    curr_sector = value;
    return true;
  }
}

uint8_t DisplayDriverPOV::get_curr_sector() {
  return curr_sector;
}

void ICACHE_RAM_ATTR DisplayDriverPOV::increment_sector() {
  curr_sector = (curr_sector + 1) % NUM_SECTORS;
}

void ICACHE_RAM_ATTR DisplayDriverPOV::decrement_sector() {
  if (curr_sector == 0) {
    curr_sector = NUM_SECTORS;
  }
  curr_sector--;
}

void ICACHE_RAM_ATTR DisplayDriverPOV::increment_count() {
  count++;
}

void DisplayDriverPOV::set_brightness(uint8_t value) {
  brightness = value;
}

float ICACHE_RAM_ATTR DisplayDriverPOV::time_in_sector_ms(float dps) {
  return THETA * 1000 / abs(dps); //time inside the sector in ms
}
