#include <Adafruit_NeoPixel.h>
#include "Display_Map.h"
#include "Generated_Map.h"

class DisplayDriverPOV
{
  private:

#define LED_PIN       0
    //    #define NUM_LEDS      10 //radius (# of leds)
    //    #define NUM_SECTORS   50 //circumference (resolution in the polar axis)
#define THETA         360 / NUM_SECTORS  //angle per sector
#define abs(x) ((x)>0?(x):-(x)) //esp8266 doesn't have abs for floats for some reason, so this works for floats

    uint8_t displayLed[NUM_SECTORS][NUM_LEDS]; //matrix of led state in ram

    Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

    uint8_t symbol_cursor = 0;
    uint8_t symbol_spacing = 2;
    uint16_t count = 0;
    const int cnt_per_cycle = 50;
    volatile uint8_t currSector = 0;
    volatile uint8_t brightness = 150;


  public:

    DisplayDriverPOV() {
      pixels.begin();
      //set the matrix to be empty
      for (int i = 0; i < NUM_SECTORS; i++) {
        for (int j = 0; j < NUM_LEDS; j++) {
          this->displayLed[i][j] = 0;
        }
      }
    }

    // draw a line where symbol cursor is
    bool set_line() {
      if (symbol_cursor + symbol_spacing + CHAR_WIDTH > NUM_SECTORS) {
        return false;
      }
      for (int i = 0;  i < NUM_LEDS; i++) {
        this->displayLed[symbol_cursor][i] = 1;
      }
      symbol_cursor++;
    }

    // draw char where symbol cursor is
    bool set_char(char symbol) {
      if (symbol_cursor + symbol_spacing + CHAR_WIDTH > NUM_SECTORS) {
        return false;
      }

      switch (symbol) {
        case 'C':
          for (uint8_t j = 0; j < CHAR_HEIGHT; j++) {
            for (uint8_t k = 0; k < CHAR_WIDTH; k++) {
              this->displayLed[symbol_cursor + k][j] = C[j][k];
            }
          }
          break;
        case 'E':
          for (uint8_t j = 0; j < CHAR_HEIGHT; j++) {
            for (uint8_t k = 0; k < CHAR_WIDTH; k++) {
              this->displayLed[symbol_cursor + k][j] = E[j][k];
            }
          }
          break;
        case 'H':
          for (uint8_t j = 0; j < CHAR_HEIGHT; j++) {
            for (uint8_t k = 0; k < CHAR_WIDTH; k++) {
              this->displayLed[symbol_cursor + k][j] = H[j][k];
            }
          }
          break;
        case 'L':
          for (uint8_t j = 0; j < CHAR_HEIGHT; j++) {
            for (uint8_t k = 0; k < CHAR_WIDTH; k++) {
              this->displayLed[symbol_cursor + k][j] = L[j][k];
            }
          }
          break;
        case 'O':
          for (uint8_t j = 0; j < CHAR_HEIGHT; j++) {
            for (uint8_t k = 0; k < CHAR_WIDTH; k++) {
              this->displayLed[symbol_cursor + k][j] = O[j][k];
            }
          }
          break;
        default:
          return false;
      }

      symbol_cursor += CHAR_WIDTH + symbol_spacing;
    }

    void show_sector(bool flip) {
      uint8_t r = (sin(count * 2 * PI / cnt_per_cycle) + 1) / 2 * brightness;
      uint8_t g = (sin(count * 2 * PI / cnt_per_cycle + (2 * PI / 3)) + 1) / 2 * brightness;
      uint8_t b = (sin(count * 2 * PI / cnt_per_cycle + (4 * PI / 3)) + 1) / 2 * brightness;

      //        uint8_t r = brightness * state;
      //        uint8_t g = 0;
      //        uint8_t b = 0;

      for (uint8_t i = 0; i < NUM_LEDS; i++) {
        uint8_t pixel_idx = i;
        if (flip) {
          pixel_idx = NUM_LEDS - i - 1;
        }
        //        uint8_t state = displayLed[currSector][pixel_idx]; //0 or 1
        uint8_t state = GENERATED_DISPLAY[currSector][pixel_idx];
        pixels.setPixelColor(i, pixels.Color(r * state, g * state, b * state));
      }
      pixels.show();
    }

    void show_line(int brightness) {
      for (uint8_t i = 0; i < NUM_LEDS; i++) {
        pixels.setPixelColor(i, pixels.Color(0, brightness, 0));
      }
      pixels.show();
    }

    void show_boot_up_sequence() {
      for (uint8_t i = 0; i < NUM_LEDS; i++) {
        pixels.setPixelColor(i, pixels.Color(0, 50, 0));
        pixels.show();
        delay(25);
      }
      for (uint8_t i = 0; i < NUM_LEDS; i++) {
        pixels.setPixelColor(i, pixels.Color(0, 0, 0));
        pixels.show();
        delay(25);
      }
    }
    void cycle_display(int cycle_speed) {
      for (uint8_t i = 0; i < NUM_SECTORS; i++) {
        for (uint8_t j = 0; j < NUM_LEDS; j++) {
          uint8_t state = GENERATED_DISPLAY[i][j];
          pixels.setPixelColor(j, pixels.Color(100 * state, 0, 0));
        }
        pixels.show();
        delay(cycle_speed);
      }
      for (uint8_t i = 0; i < NUM_LEDS; i++) {
        pixels.setPixelColor(i, pixels.Color(0, 0, 0));
      }
      pixels.show();
    }
    bool set_curr_sector(uint8_t value) {
      if (value > NUM_SECTORS) {
        return false;
      } else {
        currSector = value;
        return true;
      }
    }

    uint8_t get_curr_sector() {
      return currSector;
    }

    void increment_sector() {
      currSector = (currSector + 1) % NUM_SECTORS;
      count++;
    }

    void decrement_sector() {
      if (currSector == 0) {
        currSector = NUM_SECTORS;
      }
      else {
        currSector--;
      }
      count++;

    }

    void set_brightness(uint8_t value) {
      brightness = value;
    }

    float get_time_in_sector(float dps) {
      return THETA * 1000 / abs(dps); //time inside the sector in ms
    }
};
