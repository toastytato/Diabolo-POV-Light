#include <Adafruit_NeoPixel.h>
#include <MPU6050.h>


#define NUM_PIXELS 10
#define LED_PIN 0
#define DELAY 4

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

MPU6050 imu1(0x68); //AD0 low
MPU6050 imu2(0x69); //AD0 high

int16_t a1[3];
int16_t a2[3];

float ay1 = 0;
float ay2 = 0;
float calib = 0;

int ledLimit = 0;

void setup() {
  Serial.begin(9600);

  Wire.begin();
  imu1.initialize();
  imu2.initialize();
  imu1.setFullScaleAccelRange(3);
  imu2.setFullScaleAccelRange(3);

  pixels.begin();

  imu1.getAcceleration(&a1[0], &a1[1], &a1[2]);
  imu2.getAcceleration(&a2[0], &a2[1], &a2[2]);

  ay1 = a1[1];
  ay2 = a2[1];
}

void loop() {
  //  for (int i = 0; i < 255; i++) {
  //    for (int j = 0; j < NUM_PIXELS; j++) {
  //      pixels.setPixelColor(j, pixels.Color(0, i, 0));
  //    }
  //    pixels.show();
  //    delay(DELAY);
  //  }
  //  for (int i =  255; i > 0; i--) {
  //    for (int j = 0; j < NUM_PIXELS; j++) {
  //      pixels.setPixelColor(j, pixels.Color(0, i, 0));
  //    }
  //    pixels.show();
  //    delay(DELAY);
  //  }
  imu1.getAcceleration(&a1[0], &a1[1], &a1[2]);
  imu2.getAcceleration(&a2[0], &a2[1], &a2[2]);
  Serial.print(a1[1]);
  Serial.print("\t");
  Serial.println(a2[1]);

  ay1 = a1[1];

  ledLimit = map(ay1, -2000, 2000, 0, NUM_PIXELS);

  for(int i = 0; i < ledLimit; i++){
    pixels.setPixelColor(i, pixels.Color(100, 0, 0));
  }
  for(int i = ledLimit; i < NUM_PIXELS; i++){
    pixels.setPixelColor(i, pixels.Color(0,0,0));
  }
  pixels.show();
  
  delay(50);
}
