// Wireless Libraries
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>

// Project Libraries
#include <MPU6050.h>
#include <Adafruit_NeoPixel.h>
#include "ESP8266TimerInterrupt.h"
#include "ESP8266_ISR_Timer.h"
#include "Display_Driver.h"
#include "config.h"

/* Dev Notes
 *  2/13/21 - Using ITimer by itself instead of using the ISR_Timer sped things up 
 *  The light now works gucci baby let's goooo
 *  2/14/21 - Don't just upload copy+pasted code lmao. I ended up bricking the ESP8266 
 *  by uploading the <Wifi.h> library instead of the <ESP8266WiFi.h> so now I can't 
 *  upload new code and yeah... new esp8266 for 13 bucks let's gooooo
 * 
 */

const char* ssid = SSID;
const char* password = PASS;

ESP8266Timer ITimer;
ESP8266_ISR_Timer ISR_Timer;
DisplayDriverPOV Driver;

MPU6050 imu1(0x68); //AD0 low
MPU6050 imu2(0x69); //AD0 high

#define TIMER_0   0
#define TIMER_1   1

#define SENSOR_DISTANCE   0.0272
#define GRAVITY_ACCEL     9.8

#define BTN_INTERRUPT_PIN   2

volatile bool led_state = false;

float a1_c; //centripetal acceleration
float a2_c;

//0,1,2 --> x,y,z
int16_t a1[3];
int16_t a2[3];
// volatile for when accessed by interrupts
float diff = 0;
float prev_diff = 0; //prev accel difference to determine change in state

float ang_vel = 0;
float rpm = 0;
volatile float dps = 0;
volatile bool spin_CW = true;

////moving average variables
const int NUM_READINGS = 1;
int readIndex = 0;
float readings[NUM_READINGS];
float total = 0;
volatile float avg_dps = 0;

const float threshold = 2; //diff threshold (based on ang vel m/s) to start POV sequence
// < 1 : rotates CCW faster
// > 1 : rotates CW faster
float correction = .85;
float offset = 0;

static const int WIFI_POLLING_INTERVAL_MS = 5000;

float last_millis = 0;

volatile bool update_flag = false;

#define abs(x) ((x)>0?(x):-(x)) //esp8266 doesn't have abs for floats for some reason, so this works for floats

void ICACHE_RAM_ATTR TimerHandler() {
  if (spin_CW) {
    Driver.increment_sector();
  } else {
    Driver.decrement_sector();
  }
  Driver.increment_count();
  update_flag = true;
  ITimer.setInterval(Driver.time_in_sector_ms(avg_dps) * 1000, TimerHandler); // update timer 0
}

//gets called every pass on loop when possible
//updates the current rpm value
void readSensorEvent()
{
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
  ang_vel = correction * sqrt(diff / SENSOR_DISTANCE) + offset;
  rpm = 60 * ang_vel / (2 * PI);
  dps = rpm * 6;

  //  create a moving average for dps
  total = total - readings[readIndex];
  readings[readIndex] = dps;
  total += readings[readIndex];
  readIndex = (readIndex + 1) % NUM_READINGS;

  avg_dps = total / NUM_READINGS;
  //  avg_dps = dps;

  if (diff > threshold && prev_diff < threshold) { //on beginning of rotation
    prev_diff = diff;
    if (imu1.getRotationZ() > 0) {
      spin_CW = false;
    } else {
      spin_CW = true;
    }
    ITimer.enableTimer();
    ITimer.setInterval(50 * 1000, TimerHandler); //timer 0 trigger (calls updateDisplay almost immediately)

  } else if (diff < threshold && prev_diff > threshold) { //module is not rotating
    prev_diff = diff;
    ITimer.stopTimer();
  }
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
}

void ICACHE_RAM_ATTR on_click_ISR() {
  if (led_state) {
    Driver.show_line(10);
    ITimer.stopTimer();
  }
  else {
    Driver.show_line(0);
    ITimer.enableTimer();
  }
  led_state = !led_state;
}

void setup() {
  Serial.begin(115200);
  OTA_Setup();

  Wire.begin();
  imu1.initialize();
  imu2.initialize();
  // get the most broad range of readings
  imu1.setFullScaleAccelRange(3);
  imu2.setFullScaleAccelRange(3);

  pinMode(BTN_INTERRUPT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BTN_INTERRUPT_PIN), on_click_ISR, FALLING);

  Driver.show_boot_up_sequence();

  // Interval in microsecs
  ITimer.attachInterruptInterval(500 * 1000, TimerHandler);

}

void loop() {
  ArduinoOTA.handle();
  if (update_flag) {
    Driver.show_sector(false); //params: flip the sector
    update_flag = false;
  }
  readSensorEvent();
  
  int time_elapsed = millis() - last_millis;
  if (time_elapsed >= WIFI_POLLING_INTERVAL_MS) {
    String received_data = "";
    if (Request_IOT_Data(&received_data, String(avg_dps))) {
      //      Serial.println(data);
      correction = received_data.toFloat();

    } else {
      Serial.println("Failed");
    }
    last_millis = millis();
  }
}

bool Request_IOT_Data(String* inbound, String outbound) {
  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status

    HTTPClient http;  //Declare an object of class HTTPClient

    http.begin("http://192.168.0.102:8266/esp_post"); //Specify request destination
    http.addHeader("Content-Type", "text/plain");  //Specify content-type header

    int httpCode = http.POST(outbound);   //Send the request with data
    if (httpCode > 0) { //Check the returning code
      *inbound = http.getString();   //Get the request response payload
      
      http.end();
      return true;

    } else {
      Serial.println("An error ocurred");
    }

    http.end();   //Close connection
  }
  return false;
}

// The boiler plate code to allow for OTA updates
void OTA_Setup() {
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(2000);
    ESP.restart();
  }

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
