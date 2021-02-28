#include <Adafruit_NeoPixel.h>
// #include "Generated_Map.h"

// #ifndef num_pixels
// #define num_pixels 10 //radius (# of leds)
// #endif

// #ifndef num_sectors
// #define num_sectors 50 //circumference (resolution in the polar axis)
// #endif

#define NUM_LEDS 10

#define abs(x) ((x) > 0 ? (x) : -(x)) //esp8266 doesn't have abs for floats for some reason, so this works for floats

#define LED_PIN 0
#define THETA 360 / num_sectors //angle per sector

class DisplayDriverPOV
{
private:
  Adafruit_NeoPixel Pixels = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

  const uint16_t cnt_per_cycle = num_sectors / 3;

  uint8_t *display = nullptr;
  uint8_t num_sectors = 0;
  uint8_t num_pixels = 0;
  uint8_t num_colors = 0;
  volatile uint32_t count = 0;
  volatile uint8_t curr_sector = 0;
  volatile uint8_t brightness = 255;

  uint8_t get_pixel_color(uint8_t sector, uint8_t pixel, uint8_t color);

public:
  DisplayDriverPOV();

  //
  void print();
  void set_display_array(uint8_t *display, uint8_t x, uint8_t y, uint8_t z);
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

/***Private Functions***/

uint8_t DisplayDriverPOV::get_pixel_color(uint8_t sector, uint8_t pixel, uint8_t color)
{
  return display[sector * num_pixels * num_colors + pixel * num_colors + color];
}

/***Constructors***/

DisplayDriverPOV::DisplayDriverPOV()
{
  Pixels.begin();
}

/***Public Functions***/

void DisplayDriverPOV::print()
{
  Serial.print("Display Matrix: ");
  Serial.print(num_sectors);
  Serial.print(", ");
  Serial.print(num_pixels);
  Serial.print(", ");
  Serial.print(num_colors);
  Serial.println("");

  for (uint8_t i = 0; i < num_sectors; i++)
  {
    for (uint8_t j = 0; j < num_pixels; j++)
    {
      for (uint8_t k = 0; k < num_colors; k++)
      {
        Serial.print(get_pixel_color(i, j, k));
        Serial.print(",");
      }
      Serial.print(" : ");
    }
    Serial.println("");
  }
  Serial.println("");
}

void DisplayDriverPOV::set_display_array(uint8_t *array, uint8_t x, uint8_t y, uint8_t z)
{
  display = array;
  num_sectors = x;
  num_pixels = y;
  num_colors = z;
}

void DisplayDriverPOV::show_sector(bool flip)
{
  // uint8_t r = (sin(count * 2 * PI / cnt_per_cycle) + 1) / 2 * brightness;
  // uint8_t g = (sin(count * 2 * PI / cnt_per_cycle + (2 * PI / 3)) + 1) / 2 * brightness;
  // uint8_t b = (sin(count * 2 * PI / cnt_per_cycle + (4 * PI / 3)) + 1) / 2 * brightness;

  //      uint8_t r = 0;
  //      uint8_t g = 0;
  //      uint8_t b = brightness;
  if (display == nullptr)
  {
    return;
  }

  for (uint8_t i = 0; i < num_pixels; i++)
  {
    uint8_t pixel_idx = i;
    if (flip)
    {
      pixel_idx = num_pixels - i - 1;
    }
    //        uint8_t state = displayLed[curr_sector][pixel_idx]; //0 or 1

    uint8_t r = get_pixel_color(curr_sector, i, 0);
    uint8_t g = get_pixel_color(curr_sector, i, 1);
    uint8_t b = get_pixel_color(curr_sector, i, 2);
    Pixels.setPixelColor(i, Pixels.Color(r, g, b));
  }
  Pixels.show();
}

void DisplayDriverPOV::show_line(int brightness)
{
  for (uint8_t i = 0; i < num_pixels; i++)
  {
    Pixels.setPixelColor(i, Pixels.Color(0, brightness, 0));
  }
  Pixels.show();
}

void DisplayDriverPOV::show_boot_up_sequence()
{
  for (uint8_t i = 0; i < num_pixels; i++)
  {
    Pixels.setPixelColor(i, Pixels.Color(0, 50, 0));
    Pixels.show();
    delay(25);
  }
  for (uint8_t i = 0; i < num_pixels; i++)
  {
    Pixels.setPixelColor(i, Pixels.Color(0, 0, 0));
    Pixels.show();
    delay(25);
  }
}
void DisplayDriverPOV::cycle_display(int cycle_time)
{
  for (uint8_t i = 0; i < num_sectors; i++)
  {
    for (uint8_t j = 0; j < num_pixels; j++)
    {
      uint8_t r = get_pixel_color(curr_sector, i, 0);
      uint8_t g = get_pixel_color(curr_sector, i, 1);
      uint8_t b = get_pixel_color(curr_sector, i, 2);
      Pixels.setPixelColor(i, Pixels.Color(r, g, b));
    }
    Pixels.show();
    delay(cycle_time / num_sectors);
  }
  for (uint8_t i = 0; i < num_pixels; i++)
  {
    Pixels.setPixelColor(i, Pixels.Color(0, 0, 0));
  }
  Pixels.show();
}

bool DisplayDriverPOV::set_curr_sector(uint8_t value)
{
  if (value > num_sectors)
  {
    return false;
  }
  else
  {
    curr_sector = value;
    return true;
  }
}

uint8_t DisplayDriverPOV::get_curr_sector()
{
  return curr_sector;
}

void ICACHE_RAM_ATTR DisplayDriverPOV::increment_sector()
{
  curr_sector = (curr_sector + 1) % num_sectors;
}

void ICACHE_RAM_ATTR DisplayDriverPOV::decrement_sector()
{
  if (curr_sector == 0)
  {
    curr_sector = num_sectors;
  }
  curr_sector--;
}

void ICACHE_RAM_ATTR DisplayDriverPOV::increment_count()
{
  count++;
}

void DisplayDriverPOV::set_brightness(uint8_t value)
{
  brightness = value;
}

float ICACHE_RAM_ATTR DisplayDriverPOV::time_in_sector_ms(float dps)
{
  return THETA * 1000 / abs(dps); //time inside the sector in ms
}
