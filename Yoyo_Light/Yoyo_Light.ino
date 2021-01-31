#define BLYNK_PRINT Serial
// These define's must be placed at the beginning before #include "ESP8266TimerInterrupt.h"
#define TIMER_INTERRUPT_DEBUG         0
#define _TIMERINTERRUPT_LOGLEVEL_     0

#include <MPU6050.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include "ESP8266TimerInterrupt.h"
#include "ESP8266_ISR_Timer.h"
#include "Display_Driver.h"

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "iaRtdId-t0a7NHn1YeD_1eHz-Vx4x-Kd";

// Your WiFi credentials.
// Set password to "" for open networks.
// The EE IOT network is hidden. You might not be able to see it.
// But you should be able to connect with these credentials.
char ssid[32] = "I like Elmo cuz he's red";
char pass[32] = "idonotlikewholewheatbread";

ESP8266Timer ITimer;
ESP8266_ISR_Timer ISR_Timer;
BlynkTimer Blynk_Timer;
DisplayDriverPOV Driver;

MPU6050 imu1(0x68); //AD0 low
MPU6050 imu2(0x69); //AD0 high

const float SENSOR_DISTANCE = 0.0272;
const float GRAVITY_ACCEL = 9.8;

float a1_c; //centripetal acceleration
float a2_c;
float calib = 0;

//0,1,2 --> x,y,z
int16_t a1[3];
int16_t a2[3];
// volatile for when accessed by interrupts
volatile float diff = 0;
volatile float ang_vel = 0;
volatile float dps = 0;
float rpm = 0;

float prevDiff = 0; //prev accel difference to determine change in state
float threshold = 1; //diff threshold (based on ang vel) to start POV sequence
volatile float correction = 1;

int hw_timer_interval = 1;
int update_sensor_interval = 1; //in ms
int send_blynk_interval = 500;

//moving average variables
const int NUM_READINGS = 3;
int readIndex = 0;
float readings[NUM_READINGS];
float total = 0;
volatile float avg_dps = 0;

#define abs(x) ((x)>0?(x):-(x)) //esp8266 doesn't have abs for floats for some reason, so this works for floats

void sendBlynkEvent() {
  String msg = String(millis() / 1000) + " " + String(avg_dps) + ":" + String(diff);
  Blynk.virtualWrite(3, msg);
}

//Updates the ISR Timer.
//Make sure hw_timer_interval is less than other intervals to have accurate interrupts
void ICACHE_RAM_ATTR TimerHandler() {
  ISR_Timer.run();
}

void updateDisplay() {
  Driver.show_sector(true);
  Driver.decrement_sector();
  ISR_Timer.changeInterval(0, Driver.get_time_in_sector(avg_dps)); // update timer 0
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
  // 2048 normalize to 1g when perpendicular
  a1_c = -a1[0] / 2048.0;
  a2_c = a2[0] / 2048.0;
  //adjustments to make it exactly 1g when undisturbed
  //idk if I should add or multiply to get to 1 ehh who knows
  a1_c += 0.01;
  a2_c -= 0.03;

  // diff * gravity to convert from g to m/s
  diff = abs(a1_c - a2_c) * GRAVITY_ACCEL;
  ang_vel = correction * sqrt(diff / SENSOR_DISTANCE);
  rpm = 60 * ang_vel / (2 * 3.141592);
  dps = rpm * 6;

  //create a moving average for dps
  total = total - readings[readIndex];
  readings[readIndex] = dps;
  total += readings[readIndex];
  readIndex = (readIndex + 1) % NUM_READINGS;

  avg_dps = total / NUM_READINGS;

  //  Serial.print(diff);
  //  Serial.print("\t");
  //  Serial.print(a1[0]);
  //  Serial.print("\t");
  //  Serial.print(a1_c);
  //  Serial.print("\t");
  //  Serial.print(a2[0]);
  //  Serial.print("\t");
  //  Serial.print(a2_c);
  //  Serial.print("\t");
  //  Serial.println(dps);

  if (diff > threshold && prevDiff < threshold) { //on beginning of rotation
    ISR_Timer.enableAll();
    ISR_Timer.changeInterval(0, 50); //timer 0 trigger in 1 ms (calls updateDisplay almost immediately)
  } else if (diff < threshold) { //module is not rotating
    ISR_Timer.disableAll();
  }
  prevDiff = diff;
  //  Serial.println("Update Sensor event");
}


//sets up the display led arrays with the image to show
void initiateLedMatrix() {
    Driver.set_char('H');
    Driver.set_char('E');
    Driver.set_char('L');
    Driver.set_char('L');
    Driver.set_char('O');
//  Driver.set_line();
 // Driver.set_line();
//  Driver.set_line();
}

void setup() {
  for (uint8_t i = 0; i < NUM_READINGS; i++) {
    readings[i] = 0;
  }
  initiateLedMatrix();

  Serial.begin(9600);
  Blynk.begin(auth, ssid, pass);
  Driver.show_boot_up_sequence(); //indicate connected to wifi

  Wire.begin();
  imu1.initialize();
  imu2.initialize();
  imu1.setFullScaleAccelRange(3);
  imu2.setFullScaleAccelRange(3);

  //  Timer based on hardware interrupts for time critical tasks, won't be blocked by Blynk
  ITimer.attachInterruptInterval(hw_timer_interval * 500, TimerHandler);
  ISR_Timer.setInterval(50, updateDisplay);  //Timer 0
  //  ISR_Timer.setInterval(update_sensor_interval, readSensorEvent); //Timer 1
  //  ISR_Timer.setInterval(send_blynk_interval, sendBlynkEvent);     //Timer 2

  // Blynk Timer for non time critical events
  Blynk_Timer.setInterval(update_sensor_interval, readSensorEvent);
  Blynk_Timer.setInterval(send_blynk_interval, sendBlynkEvent);
}

void loop() {
  Blynk.run();
  Blynk_Timer.run();
}

BLYNK_WRITE(V1)
{
  // param is a member variable of the Blynk ADT. It is exposed so you can read it.
  int onoff = param.asInt(); // assigning incoming value from pin V1 to a variable
  updateDisplay();
  if (onoff) {
    Serial.println("enable");
    ISR_Timer.enableAll();
  } else {
    Serial.println("disable");
    ISR_Timer.disableAll();
    Driver.show_line(0);  //show a line of 0 brightness pixels
  }
}

//Angle slider
BLYNK_WRITE(V2)
{
  int value = param.asInt();
  Driver.set_brightness(value);
}

//Step values buttons
BLYNK_WRITE(V6)
{
  int value = param.asInt();
  Driver.set_curr_sector(value);
  Driver.show_sector(true);
  Serial.println(Driver.get_curr_sector());
  Driver.increment_sector();
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
