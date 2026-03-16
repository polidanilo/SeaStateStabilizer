#include <Wire.h>

const int MPU_ADDR = 0x68;
int16_t AcX, AcY, AcZ;

double roll_filtered = 0.0;
double pitch_filtered = 0.0;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  // Wake MPU6050
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  Serial.println("MPU6050 ready, starting read loop...");
  delay(1000);
}

void loop() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6, true);

  // Read 6 bytes and rebuild 16-bit values
  AcX = Wire.read() << 8 | Wire.read();
  AcY = Wire.read() << 8 | Wire.read();
  AcZ = Wire.read() << 8 | Wire.read();

  double roll_raw = atan2(AcY, AcZ) * 180.0 / PI;
  double pitch_raw = atan2(-AcX, sqrt((double)AcY * AcY + (double)AcZ * AcZ)) * 180.0 / PI;

  // Simple low-pass filter: 80% previous, 20% new
  const double alpha = 0.2;
  roll_filtered = (1.0 - alpha) * roll_filtered + alpha * roll_raw;
  pitch_filtered = (1.0 - alpha) * pitch_filtered + alpha * pitch_raw;

  Serial.print("roll_raw=");
  Serial.print(roll_raw);
  Serial.print(" roll_filt=");
  Serial.print(roll_filtered);
  Serial.print(" pitch_raw=");
  Serial.print(pitch_raw);
  Serial.print(" pitch_filt=");
  Serial.println(pitch_filtered);

  delay(100);
}
