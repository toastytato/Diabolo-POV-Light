
/*
   Adapts blink rate to rpm
   App changes RPM and angle, led should be on for the entirety of those angles
*/

/* Comment this out to disable prints and save space */
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

const uint8_t ledPinR = 19;
const uint8_t ledPinY = 18;
const uint8_t ledPinG = 17;

const int C[3][3] = {
  {1, 1, 1},
  {1, 0, 0},
  {1, 1, 1},
};
const int O[3][3] = {
  {1, 1, 1},
  {1, 0, 1},
  {1, 1, 1},
};
const int L[3][3] = {
  {1, 0, 0},
  {1, 0, 0},
  {1, 1, 1},
};

BlynkTimer timer;
MPU6050 imu1(0x68);
MPU6050 imu2(0x69);
hw_timer_t * ledtimer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

const int freq = 1;
const int ledChannel = 0;
const int resolution = 10;
int duty_cycle = 0;
int state = 0;

int time_count = 0; // timer counter global variable
String content = "";  // null string constant ( an empty string )

//0,1,2 --> x,y,z
float ay1_vect[3];
float ay2_vect[3];
float calib = 0;

int16_t a1[3];
int16_t a2[3];
float ang_vel = 0;
float dps = 0;
float gyro_dps = 0;
float prevDiff = 0;
float rpm = 0;
float correction = 1;
int threshold = 1;

#define SENSOR_DISTANCE 0.104
#define GRAVITY_ACCEL 9.8

int ticks;

const int NUM_SECTORS = 64; //circumference (resolution in the polar axis)
const int NUM_LED = 3;  //radius (# of leds)
const float theta = 360 / NUM_SECTORS;  //angle per sector

uint8_t displayLed[NUM_SECTORS][NUM_LED]; //matrix for displaying led
uint8_t sectorStage = 0;  //curr sector

void sendBlynkEvent() {
  String msg = String(dps) + " : " + String(gyro_dps);
  Blynk.virtualWrite(3, msg);
  Blynk.virtualWrite(8, state);
}

//get the number of ticks (microseconds) based on dps
double getTicks() {
  ticks = theta * 1000000 / abs(dps);
  return ticks;
}

//gets called every 1 ms
//updates the current rpm value
void readSensorEvent()
{

  //  dps = correction * imu.getRotationZ() / 16.4; //converts sensor reading to degrees/second
  //  rpm = dps * 60 / 360;  //dps to rpm
  imu1.getAcceleration(&a1[0], &a1[1], &a1[2]);
  imu2.getAcceleration(&a2[0], &a2[1], &a2[2]);

  for (int i = 0; i < 3; i++) {
    ay1_vect[i] = a1[i] / 2048.0;
    ay2_vect[i] = calib * a2[i] / 2048.0; //reads sensor while still and tries to make sure diff is as closer to 0
  }

  float diff = abs(ay1_vect[1] - ay2_vect[1]);

  // diff * gravity b/c diff is in g's, normalize into m/s
  ang_vel = sqrt(diff * GRAVITY_ACCEL / SENSOR_DISTANCE);
  rpm = 60 * ang_vel / (2 * 3.141592);
  dps = correction * rpm * 6; //correction changes the dps value

  gyro_dps = imu1.getRotationZ() / 16.4;  // testing the gyro here

  portENTER_CRITICAL_ISR(&timerMux);  //Won't be interrupted during this process

  getTicks();

  if (diff > threshold && prevDiff < threshold) { //on beginning of rotation
    timerAlarmWrite(ledtimer, ticks, true);
    timerAlarmEnable(ledtimer);
  } else if (diff < threshold) { //module is not rotating
    timerAlarmDisable(ledtimer);
  }
  prevDiff = diff;

  portEXIT_CRITICAL_ISR(&timerMux);

  Serial.print(dps);
  Serial.print(" : ");
  Serial.println(gyro_dps);
}

//displays the led
void IRAM_ATTR updateLedEvent()
{
  portENTER_CRITICAL_ISR(&timerMux);

  for (int i = 0; i < NUM_LED; i++) {
    state = displayLed[sectorStage][i];
    digitalWrite(19 - i, state);
  }
  sectorStage = (sectorStage + 1) % NUM_SECTORS;  // progress the current sector

  timerAlarmWrite(ledtimer, ticks, true); // use the ticks from readSensorEvent to determine when next updateLedEvent should be for next stage

  portEXIT_CRITICAL_ISR(&timerMux);
}

//sets up the display led arrays with the image to show
void initiateLedMatrix() {
  for (int i = 0; i < NUM_SECTORS; i++) {
    for (int j = 0; j < NUM_LED; j++) {
      if (i == 0) {
        displayLed[i][j] = C[j][0];
        displayLed[i + 1][j] = C[j][1];
        displayLed[i + 2][j] = C[j][2];
      }
      if (i == 4) {
        displayLed[i][j] = O[j][0];
        displayLed[i + 1][j] = O[j][1];
        displayLed[i + 2][j] = O[j][2];
      }
      if (i == 8) {
        displayLed[i][j] = O[j][0];
        displayLed[i + 1][j] = O[j][1];
        displayLed[i + 2][j] = O[j][2];
      }
      if (i == 12) {
        displayLed[i][j] = L[j][0];
        displayLed[i + 1][j] = L[j][1];
        displayLed[i + 2][j] = L[j][2];
      }
    }
  }

  // Instantiate the intial position of the display
  //currSector / NumSectors = angle / 360
  sectorStage = NUM_SECTORS * atan(a1[0] / a1[1]) / (2 * 3.14159);
  Serial.println(sectorStage);
}

void setup()
{
  // Serial Monitor
  Serial.begin(9600);
  Blynk.begin(auth, ssid, pass);

  Wire.begin();
  imu1.initialize();
  imu2.initialize();
  imu1.setFullScaleAccelRange(3);
  imu2.setFullScaleAccelRange(3);
  imu1.getAcceleration(&a1[0], &a1[1], &a1[2]);
  imu2.getAcceleration(&a2[0], &a2[1], &a2[2]);
  ay1_vect[1] = a1[1];
  ay2_vect[1] = a2[1];
  calib = ay1_vect[1] / ay2_vect[1];

  // Blynk timer for less time sensitive non-critical events
  timer.setInterval(10L, sendBlynkEvent); // 10 ms interval
  timer.setInterval(1L, readSensorEvent);  //reading the dps
  // Hardware timer for time sensitive light updates
  ledtimer = timerBegin(0, 80, true); // 80/80MHz = 1000 ns = 1 microsecond
  timerAttachInterrupt(ledtimer, &updateLedEvent, true);

  //ledcSetup(ledChannel, freq, resolution);
  //ledcAttachPin(ledPinR, ledChannel);
  initiateLedMatrix();

  pinMode(ledPinR, OUTPUT);
  pinMode(ledPinY, OUTPUT);
  pinMode(ledPinG, OUTPUT);
}

void loop()
{
  Blynk.run();
  timer.run(); // Initiates BlynkTimer
}

BLYNK_WRITE(V1)
{
  // param is a member variable of the Blynk ADT. It is exposed so you can read it.
  int onoff = param.asInt(); // assigning incoming value from pin V1 to a variable

  if (onoff) {
    Serial.println("enable");
    timerAlarmEnable(ledtimer);
  } else {
    Serial.println("disable");
    timerAlarmDisable(ledtimer);
    for (int i = 0; i < NUM_LED; i++) {
      digitalWrite(17 + i, LOW);
    }
  }
}

BLYNK_WRITE(V2)
{
  int value = param.asInt();
}

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
