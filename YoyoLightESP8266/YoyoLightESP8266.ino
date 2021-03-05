// Wireless Libraries
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>

// Project Libraries
#include <MPU6050.h>
#include "ESP8266TimerInterrupt.h"
#include "ESP8266_ISR_Timer.h"
#include "Display_Driver.h"
#include "My_Packet_Parser.h"
#include "config.h"

/* Dev Notes
 *  2/13/21 - Using ITimer by itself instead of using the ISR_Timer sped things up 
 *  The light now works gucci baby let's goooo
 *  2/14/21 - Don't just upload copy+pasted code lmao. I ended up bricking the ESP8266 
 *  by uploading the <Wifi.h> library instead of the <ESP8266WiFi.h> so now I can't 
 *  upload new code and yeah... new esp8266 for 13 bucks let's gooooo
 *  2/23/21 - So I tried uploading a Blink sketch at 57600 upload speed and it worked somehow? 
 *  I also tried grounding GPIO 0 to GND several times which didn't work, but perhaps it was 
 *  from grounding it when I plugged in my USB? ig 
 *  2/24/21 - I2C Don't work no more :( Can't communicate with IMU 
 */

ESP8266Timer ITimer;
ESP8266_ISR_Timer ISR_Timer;
DisplayDriverPOV Driver(10, 0); //Params: # Leds, Led Pin

MPU6050 imu1(0x68); //AD0 low
MPU6050 imu2(0x69); //AD0 high

WiFiUDP udp;

#define TIMER_0 0
#define TIMER_1 1

#define SENSOR_DISTANCE 0.0272
#define GRAVITY_ACCEL 9.8

#define BTN_INTERRUPT_PIN 2

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

const char *SERVER_IP = "192.168.0.102";
const char *SERVER_PORT = "8266"; //port used by the computer
const int UDP_PORT = 8266;        //port used by Flask
const int WIFI_POLLING_INTERVAL_MS = 300;

const int IN_BUFFER_SIZE = 2000; //2000 will allow for a 64x10x3 matrix and some header info
uint8_t receive_buffer[IN_BUFFER_SIZE];

const int OUT_BUFFER_SIZE = 10;
uint8_t transmit_buffer[OUT_BUFFER_SIZE];

uint8_t prev_buffer_state = 0;

float last_millis = 0;
volatile bool update_flag = false; // tells main loop when new sector is reached

#define abs(x) ((x) > 0 ? (x) : -(x)) //esp8266 doesn't have abs for floats for some reason, so this works for floats

/*** CALLBACK FUNCTIONS ***/

//callback function: has to be void and accept udp_msg param
void Update_Callback(udp_msg *packet_ptr)
{
  //casts the bitstream packet into a struct to be accessed as reference
  struct update_packet
  {
    uint8_t curr_sector;
    uint8_t num_sectors;
    uint8_t num_pixels;
    uint8_t num_colors;
    uint8_t display_start_byte;
  };

  update_packet *update_msg = (update_packet *)(&(packet_ptr->content_start_byte));
  uint8_t *display_matrix = &update_msg->display_start_byte;
  Driver.set_display_array(display_matrix,
                           update_msg->num_sectors,
                           update_msg->num_pixels,
                           update_msg->num_colors);
  Driver.set_curr_sector(update_msg->curr_sector);
  // Driver.print();
  update_flag = true;
  Serial.println("Update");
}

//timer callback function: triggered when display rotates into a new sector
void ICACHE_RAM_ATTR Timer_Handler()
{
  if (spin_CW)
  {
    Driver.increment_sector();
  }
  else
  {
    Driver.decrement_sector();
  }
  Driver.increment_count();
  update_flag = true;
  ITimer.setInterval(Driver.time_in_sector_ms(avg_dps) * 1000, Timer_Handler); // update timer 0
}

//interrupt button callback: stops the updates and shows a line when toggled
void ICACHE_RAM_ATTR On_Click_ISR()
{
  if (led_state)
  {
    Driver.show_line(10);
    ITimer.stopTimer();
  }
  else
  {
    Driver.show_line(0);
    ITimer.enableTimer();
  }
  led_state = !led_state;
}

/*** DATA PROCESSING ***/

//gets called every pass on loop when possible
//updates the current rpm value
void Read_Sensor_Event()
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

  if (diff > threshold && prev_diff < threshold)
  { //on beginning of rotation
    prev_diff = diff;
    if (imu1.getRotationZ() > 0)
    {
      spin_CW = false;
    }
    else
    {
      spin_CW = true;
    }
    ITimer.enableTimer();
    ITimer.setInterval(50 * 1000, Timer_Handler); //timer 0 trigger (calls updateDisplay almost immediately)
  }
  else if (diff < threshold && prev_diff > threshold)
  { //module is not rotating
    prev_diff = diff;
    ITimer.stopTimer();
  }
  // Serial.print(diff);
  // Serial.print("\t");
  // Serial.print(a1[0]);
  // Serial.print("\t");
  // Serial.print(a1_c);
  // Serial.print("\t");
  // Serial.print(a2[0]);
  // Serial.print("\t");
  // Serial.print(a2_c);
  // Serial.print("\t");
  // Serial.println(dps);
}

/*** MAIN CONTROL ***/

void setup()
{
  Serial.begin(115200);
  OTA_Setup();
  udp.begin(UDP_PORT);

  Wire.begin();
  imu1.initialize();
  imu2.initialize();
  // get the most broad range of readings
  imu1.setFullScaleAccelRange(3);
  imu2.setFullScaleAccelRange(3);

  pinMode(BTN_INTERRUPT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BTN_INTERRUPT_PIN), On_Click_ISR, FALLING);

  for (int i = 0; i < IN_BUFFER_SIZE; i++)
  {
    receive_buffer[i] = 0;
  }

  Assign_Function_To_Header(Update_Callback, 0);
  Driver.show_boot_up_sequence();

  // Interval in microsecs
  // ITimer.attachInterruptInterval(500 * 1000, Timer_Handler);
}

void loop()
{
  // ArduinoOTA.handle();
  if (update_flag)
  {
    Driver.show_sector(true); //params: flip the sector
    // Driver.print();
    Serial.println("Loop");
    update_flag = false;
  }

  // Driver.show_sector(false);
  // Read_Sensor_Event();

  int time_elapsed = millis() - last_millis;
  if (time_elapsed >= WIFI_POLLING_INTERVAL_MS)
  {
    Serial.println("Huh");

    transmit_buffer[0] = Driver.get_curr_sector();
    transmit_buffer[1] = '\0';

    if (Request_UDP_Data(receive_buffer, transmit_buffer)) //params: receiving buffer as array ptr, transmitting message as String
    {
      Call_Header_Function(receive_buffer);
    }
    else
    {
      Serial.println("Request to server failed");
    }
    last_millis = millis();
  }
}

/*** WIFI / WIRELESS FUNCTIONS ***/

bool Request_UDP_Data(uint8_t *buffer, uint8_t *outbound)
{
  udp.beginPacket(SERVER_IP, UDP_PORT);
  udp.write(outbound, OUT_BUFFER_SIZE);
  udp.endPacket();
  memset(buffer, 0, IN_BUFFER_SIZE);
  //processing incoming update_msg, must be called before reading the buffer
  udp.parsePacket();
  return (udp.read(buffer, IN_BUFFER_SIZE) > 0);
}

bool Request_TCP_Data(String *inbound, String outbound)
{
  if (WiFi.status() == WL_CONNECTED)
  { //Check WiFi connection status

    WiFiClient client;
    HTTPClient http; //Declare an object of class HTTPClient

    http.begin(client, "http://192.168.0.102:8266/esp_request"); //Specify request destination
    http.addHeader("Content-Type", "text/plain");                //Specify content-type header

    int httpCode = http.POST(outbound); //Send the request with data
    if (httpCode > 0)
    {                              //Check the returning code
      *inbound = http.getString(); //Get the request response payload
      http.end();
      return true;
    }
    Serial.println("An error ocurred");
    http.end(); //Close connection
  }
  return false;
}

// The boiler plate code to allow for OTA updates
void OTA_Setup()
{
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASS);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
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
    if (error == OTA_AUTH_ERROR)
      Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
      Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
      Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
      Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)
      Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
