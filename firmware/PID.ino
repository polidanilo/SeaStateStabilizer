#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "PID.h"

// Hardware configuration
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40, Wire);
const int MPU_ADDR = 0x68;
int16_t AcX, AcY, AcZ;

// PID controllers (roll = upper axis, pitch = base axis)
PIDController pidRoll(1.3, 0.0, 0.0);
PIDController pidPitch(0.5, 0.0, 0.05);

// Offsets (tare) and filtering
float offset_roll = 5.0f;
float offset_pitch = 0.0f;
double roll_filtered = 0.0;
double pitch_filtered = 0.0;

// Servo configuration
int servo_center = 375;
int servo_min = 275;
int servo_max = 475;

unsigned long previous_time_ms = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  // Wake MPU6050
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  // Initialize PCA9685
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(50);

  // Center both servos
  pwm.setPWM(0, 0, servo_center);
  pwm.setPWM(1, 0, servo_center);
  delay(2000);

  previous_time_ms = millis();
  Serial.println("Gimbal control loop starting...");
}

void loop() {
  unsigned long now_ms = millis();
  double dt = (now_ms - previous_time_ms) / 1000.0;
  previous_time_ms = now_ms;
  if (dt <= 0.0) {
    return;
  }

  // Read accelerometer
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6, true);
  AcX = Wire.read() << 8 | Wire.read();
  AcY = Wire.read() << 8 | Wire.read();
  AcZ = Wire.read() << 8 | Wire.read();

  // Raw angles
  double roll_raw = atan2(AcY, AcZ) * 180.0 / PI;
  double pitch_raw = atan2(-AcX, sqrt((double)AcY * AcY + (double)AcZ * AcZ)) * 180.0 / PI;

  // Apply tare
  double roll_real = roll_raw - offset_roll;
  double pitch_real = pitch_raw - offset_pitch;

  // Low-pass filtering
  const double alpha = 0.2;  // new sample weight
  roll_filtered = (1.0 - alpha) * roll_filtered + alpha * roll_real;
  pitch_filtered = (1.0 - alpha) * pitch_filtered + alpha * pitch_real;

  // PID setpoint is 0° for both axes
  double setpoint = 0.0;
  double roll_correction = pidRoll.compute(setpoint, roll_filtered, dt);
  double pitch_correction = pidPitch.compute(setpoint, pitch_filtered, dt);

  int pwm_roll = servo_center + (int)roll_correction;
  int pwm_pitch = servo_center + (int)pitch_correction;

  pwm_roll = constrain(pwm_roll, servo_min, servo_max);
  pwm_pitch = constrain(pwm_pitch, servo_min, servo_max);

  // Write to servos: channel 0 = pitch (base), channel 1 = roll (upper)
  pwm.setPWM(0, 0, pwm_pitch);
  pwm.setPWM(1, 0, pwm_roll);

  Serial.print("roll_filt=");
  Serial.print(roll_filtered);
  Serial.print(" pitch_filt=");
  Serial.print(pitch_filtered);
  Serial.print(" pwm_pitch=");
  Serial.print(pwm_pitch);
  Serial.print(" pwm_roll=");
  Serial.println(pwm_roll);

  delay(15);
}
