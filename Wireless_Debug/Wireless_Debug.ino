/*
 * Adapts blink rate to rpm
 * App changes RPM and angle, led should be on for the entirety of those angles
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
char ssid[32] = "";
char pass[32] = "";

const uint8_t ledPinR = 17;
const uint8_t ledPinY = 18;
const uint8_t ledPinG = 19;

BlynkTimer timer;
MPU6050 imu;
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
float accel[3];
float gyro = 0;
float prevGyro = 0;
float rpm;

int tick_ms = 100;

const int X_SECTORS = 32; //circumference
const int Y_LED = 1;  //radius

int thetaDuration = 90; //degrees to remain on for

void sendBlynkEvent(){
  Blynk.virtualWrite(3, gyro);
  Blynk.virtualWrite(8, state);   
}

double getTicks(){
 if(state){
    tick_ms = (360 - thetaDuration) * 1000 / abs(gyro);  
  } else{
     tick_ms = thetaDuration * 1000 / abs(gyro);  
  }
  return tick_ms * 1000;
}

void readSensorEvent()
{
  gyro = imu.getRotationZ() / 16.4; //converts sensor reading to degrees/second
  
  //portENTER_CRITICAL_ISR(&timerMux);
  
  rpm = gyro * 60 / 360;  //dps to rpm
  
  if(gyro && prevGyro == 0){ //gyro has reading and initiate display led events
    timerAlarmWrite(ledtimer, getTicks(), true);
    timerAlarmEnable(ledtimer);
  } else if(!gyro){  //reading is empty
    timerAlarmDisable(ledtimer);
  }
  prevGyro = gyro;
  
  //portEXIT_CRITICAL_ISR(&timerMux);
}

void IRAM_ATTR updateLedEvent()
{
  portENTER_CRITICAL_ISR(&timerMux);
  
  digitalWrite(ledPinR, state);
  //Serial.println(state);
  state = !state; //flip state
  
  timerAlarmWrite(ledtimer, getTicks(), true);

  portEXIT_CRITICAL_ISR(&timerMux);
}

void setup()
{
  // Serial Monitor
  Serial.begin(9600);
  Blynk.begin(auth, ssid, pass);

  Wire.begin();
  imu.initialize();
  // 0 = +/- 2g, +/- 250 degrees/sec
  // 1 = +/- 4g, +/- 500 degrees/sec
  // 2 = +/- 8g, +/- 1000 degrees/sec
  // 3 = +/- 16g, +/- 2000 degrees/sec
  imu.setFullScaleAccelRange(3);
  imu.setFullScaleGyroRange(3);

  timer.setInterval(10L, sendBlynkEvent); // 10 ms interval
  timer.setInterval(5L, readSensorEvent);  //reading the gyro
  ledtimer = timerBegin(0, 80, true); // 80/80MHz = 1000 ns = 1 microsecond
  timerAttachInterrupt(ledtimer, &updateLedEvent, true);
  
  //ledcSetup(ledChannel, freq, resolution);
  //ledcAttachPin(ledPinR, ledChannel);

  pinMode(ledPinR, OUTPUT);
  //  pinMode(ledPinY, OUTPUT);
  //  pinMode(ledPinG, OUTPUT);
}

void loop()
{
  Blynk.run();
  timer.run(); // Initiates BlynkTimer
}

BLYNK_WRITE(V1)
{
  // param is a member variable of the Blynk ADT. It is exposed so you can read it.
  int pinValue = param.asInt(); // assigning incoming value from pin V1 to a variable
  //state = pinValue;
  static int yeet = 0;
  yeet = !yeet; 

  if(yeet){
    Serial.println("enable");
    timerAlarmEnable(ledtimer);
  } else{
    Serial.println("disable");
    timerAlarmDisable(ledtimer);
  }
}

BLYNK_WRITE(V2)
{
  int value = param.asInt();
  thetaDuration = value;
}

BLYNK_WRITE(V4) //rpm slider for debugging
{
  //gyro = param.asFloat();
}

// This function is executed whenever Blynk pin V5 requests data from the ESP32
BLYNK_READ(V5) // Widget in the app READS Virtal Pin V5 with the certain frequency
{
  // This command writes Arduino's uptime in seconds to Virtual Pin V5
  Blynk.virtualWrite(5, millis() / 1000);
}
