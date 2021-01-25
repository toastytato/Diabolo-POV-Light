#define BLYNK_PRINT Serial
// These define's must be placed at the beginning before #include "ESP8266TimerInterrupt.h"
#define TIMER_INTERRUPT_DEBUG         0
#define _TIMERINTERRUPT_LOGLEVEL_     0

#include <Adafruit_NeoPixel.h>
#include <MPU6050.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include "ESP8266TimerInterrupt.h"
#include "ESP8266_ISR_Timer.h"

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "iaRtdId-t0a7NHn1YeD_1eHz-Vx4x-Kd";

// Your WiFi credentials.
// Set password to "" for open networks.
// The EE IOT network is hidden. You might not be able to see it.
// But you should be able to connect with these credentials.
char ssid[32] = "I like Elmo cuz he's red";
char pass[32] = "idonotlikewholewheatbread";

const int LED_PIN = 0;
const int NUM_LEDS = 10;  //radius (# of leds)
const int NUM_SECTORS = 32; //circumference (resolution in the polar axis)
const float theta = 360 / NUM_SECTORS;  //angle per sector

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
ESP8266Timer ITimer;
ESP8266_ISR_Timer ISR_Timer;
BlynkTimer Blynk_Timer;

MPU6050 imu1(0x68); //AD0 low
MPU6050 imu2(0x69); //AD0 high

const float SENSOR_DISTANCE = 0.0272;
const float GRAVITY_ACCEL = 9.8;

//0,1,2 --> x,y,z
float ay1_vect[3];
float ay2_vect[3];
float calib = 0;

int16_t a1[3];
int16_t a2[3];
// volatile for when accessed by interrupts
volatile float diff = 0;
volatile float ang_vel = 0;
volatile float dps = 0;
float gyro_dps = 0;
float prevDiff = 0;
float rpm = 0;
volatile float correction = 1;
volatile int threshold = 1;

bool toggle = false;

int hw_timer_interval = 1;
int update_display_interval = 50; //doesn't really matter since it'll get updated again
int update_sensor_interval = 5; //in ms
int send_blynk_interval = 200;

uint8_t displayLed[NUM_SECTORS][NUM_LEDS]; //matrix for displaying led
uint8_t sectorStage = 0;  //curr sector

#define abs(x) ((x)>0?(x):-(x))

void sendBlynkEvent() {
  String msg = String(dps) + ":" + String(diff);
  Blynk.virtualWrite(3, msg);
  //  Serial.println("Send Blynk Event");
}

//Updates the ISR Timer. 
//Make sure hw_timer_interval is less than other intervals to have accurate interrupts
void ICACHE_RAM_ATTR TimerHandler() {
  ISR_Timer.run();
}

void updateDisplay() {
  if (sectorStage >= 0 && sectorStage <= 15) {
    for (int i = 0; i < NUM_LEDS; i++) {
      pixels.setPixelColor(i, pixels.Color(0, 50, 0));
    }
  } else {
    for (int i = 0; i < NUM_LEDS; i++) {
      pixels.setPixelColor(i, pixels.Color(0, 0, 0));
    }
  }
  //  toggle = !toggle;
  pixels.show();

  sectorStage = (sectorStage + 1) % NUM_SECTORS;

  update_display_interval = theta * 1000 / abs(dps); //time inside the sector in ms
  ISR_Timer.changeInterval(0, update_display_interval); // update timer 0
  //  Serial.println("Update Display");
}

//gets called every 1 ms
//updates the current rpm value
void readSensorEvent()
{

  //  dps = correction * imu.getRotationZ() / 16.4; //converts sensor reading to degrees/second
  //  rpm = dps * 60 / 360;  //dps to rpm
  imu1.getAcceleration(&a1[0], &a1[1], &a1[2]);
  imu2.getAcceleration(&a2[0], &a2[1], &a2[2]);

  // the two sensors are aligned 180 from each other
  // -1 on one imu will normalize the measurements to assume they're facing same direction
  for (int i = 0; i < 3; i++) {
    ay1_vect[i] = -a1[i] / 2048.0;
    ay2_vect[i] = a2[i] / 2048.0; //reads sensor while still and tries to make sure diff is as close to 0
  }

  ay1_vect[0] += 0.01;
  ay2_vect[0] -= 0.03;

  diff = abs(ay1_vect[0] - ay2_vect[0]);
  // diff * gravity b/c diff is in g's, normalize into m/s
  ang_vel = correction * sqrt(diff * GRAVITY_ACCEL / SENSOR_DISTANCE);
  rpm = 60 * ang_vel / (2 * 3.141592);
  dps = rpm * 6; //correction changes the dps value

  Serial.print(diff);
  Serial.print("\t");
  Serial.print(a1[0]);
  Serial.print("\t");
  Serial.print(ay1_vect[0]);
  Serial.print("\t");
  Serial.print(a2[0]);
  Serial.print("\t");
  Serial.print(ay2_vect[0]);
  Serial.print("\t");
  Serial.println(dps);

  //  update_display_interval = theta * 1000000 / abs(dps); // converts time till next update to

  if (diff > threshold && prevDiff < threshold) { //on beginning of rotation
    ISR_Timer.enableAll();
    ISR_Timer.changeInterval(0, update_display_interval);
  } else if (diff < threshold) { //module is not rotating
    ISR_Timer.disableAll();
  }
  prevDiff = diff;
  //  Serial.println("Update Sensor event");
}

void setup() {
  Serial.begin(9600);
  Blynk.begin(auth, ssid, pass);

  Wire.begin();
  imu1.initialize();
  imu2.initialize();
  imu1.setFullScaleAccelRange(3);
  imu2.setFullScaleAccelRange(3);

  //  Timer based on hardware interrupts for time critical tasks, won't be blocked by Blynk
  ITimer.attachInterruptInterval(hw_timer_interval * 500, TimerHandler);
  ISR_Timer.setInterval(update_display_interval, updateDisplay);  //Timer 0
  //  ISR_Timer.setInterval(update_sensor_interval, readSensorEvent); //Timer 1
  //  ISR_Timer.setInterval(send_blynk_interval, sendBlynkEvent);     //Timer 2

  // Blynk Timer for non time critical events
  Blynk_Timer.setInterval(update_sensor_interval, readSensorEvent);
  Blynk_Timer.setInterval(send_blynk_interval, sendBlynkEvent);

  pixels.begin();
  Serial.println("Setup Done");
}

void loop() {
  Blynk.run();
  Blynk_Timer.run();
}

BLYNK_WRITE(V1)
{
  // param is a member variable of the Blynk ADT. It is exposed so you can read it.
  int onoff = param.asInt(); // assigning incoming value from pin V1 to a variable

  if (onoff) {
    Serial.println("enable");
    ISR_Timer.enableAll();
  } else {
    Serial.println("disable");
    ISR_Timer.disableAll();
    for (int i = 0; i < NUM_LEDS; i++) {
      pixels.setPixelColor(i, pixels.Color(0, 0, 0));
    }
    pixels.show();
  }
}

//Angle slider
BLYNK_WRITE(V2)
{
  int value = param.asInt();
}

//
BLYNK_WRITE(V4) //dps drift correction
{
  correction = 1 + param.asInt() / 1000.0;
}

// This function is executed whenever Blynk pin V5 requests data from the ESP32
BLYNK_READ(V5) // Widget in the app READS Virtal Pin V5 with the certain frequency
{
  // This command writes Arduino's uptime in seconds to Virtual Pin V5
  Blynk.virtualWrite(5, millis() / 1000);
}
