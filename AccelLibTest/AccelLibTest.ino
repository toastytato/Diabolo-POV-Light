#include <MPU6050.h>

MPU6050 accelgyro;

int16_t ax, ay, az;
int16_t gx, gy, gz;

void setup() {
  // put your setup code here, to run once:
  Wire.begin();

  Serial.begin(9600);
  accelgyro.initialize();
  // 0 = +/- 2g, +/- 250 degrees/sec
  // 1 = +/- 4g, +/- 500 degrees/sec
  // 2 = +/- 8g, +/- 1000 degrees/sec
  // 3 = +/- 16g, +/- 2000 degrees/sec
  accelgyro.setFullScaleAccelRange(3);
  accelgyro.setFullScaleGyroRange(3);
}

void loop() {
  // put your main code here, to run repeatedly:
  accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  Serial.print("a/g:\t");
  Serial.print(ax); Serial.print("\t");
  Serial.print(ay); Serial.print("\t");
  Serial.print(az); Serial.print("\t");
  Serial.print(gx); Serial.print("\t");
  Serial.print(gy); Serial.print("\t");
  Serial.println(gz);

  delay(100);
}
