#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <MPU6050.h>

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "sUiVLDEdz785MPaqng-pNojrN3_wLcw9";

// Your WiFi credentials.
// Set password to "" for open networks.
// The EE IOT network is hidden. You might not be able to see it.
// But you should be able to connect with these credentials.
char ssid[32] = "I like Elmo cuz he's red";
char pass[32] = "idonotlikewholewheatbread";

BlynkTimer timer;
MPU6050 imu1(0x68); //AD0 low
MPU6050 imu2(0x69); //AD0 high

int16_t a1[3];
int16_t a2[3];

float ay1 = 0;
float ay2 = 0;
float calib = 0;

float ang_vel = 0;
float rpm = 0;
float dps = 0;

void sendBlynkEvent(){
  imu1.getAcceleration(&a1[0], &a1[1], &a1[2]);
  imu2.getAcceleration(&a2[0], &a2[1], &a2[2]);
  
  ay1 = a1[1] / 2048.0;
  ay2 = calib * a2[1] / 2048.0;
  
  // Serial.println(a2[0]);
  //Serial.print(a1[1] + "\t");
  //Serial.print(a1[2] + "\t");
  //Serial.print(a2[0] + "\t");
  //Serial.print(a2[1] + "\t");
  //Serial.print(a2[2] + "\n");
  
  // D = 104 mm = .104 m
  // accel = g = m/s^2 
  ang_vel = sqrt(abs(ay1 - ay2) * 9.8 / 0.104);
  rpm = 60 * ang_vel / (2 * 3.141592);
  dps = rpm * 60;
  Blynk.virtualWrite(3, rpm);
  Serial.print(ay1);
  Serial.print(" - ");
  Serial.print(ay2);
  Serial.print(" - ");
  Serial.print(ay2-ay1);
  Serial.println();
}

void setup() {
  Serial.begin(9600);
  Blynk.begin(auth, ssid, pass);

  Wire.begin();
  imu1.initialize();
  imu2.initialize();
  imu1.setFullScaleAccelRange(3);
  imu2.setFullScaleAccelRange(3);
  
  imu1.getAcceleration(&a1[0], &a1[1], &a1[2]);
  imu2.getAcceleration(&a2[0], &a2[1], &a2[2]);
  
  ay1 = a1[1];
  ay2 = a2[1];

  calib = ay1 / ay2;
  
  timer.setInterval(10L, sendBlynkEvent); // 10 ms interval
}

void loop() {
   Blynk.run();
   timer.run();
}

BLYNK_READ(V5) // Widget in the app READS Virtal Pin V5 with the certain ang_veluency
{
  // This command writes Arduino's uptime in seconds to Virtual Pin V5
  Blynk.virtualWrite(5, millis() / 1000);
}
