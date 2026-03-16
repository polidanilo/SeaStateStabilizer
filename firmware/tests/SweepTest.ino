#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40, Wire);

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(50);

  Serial.println("Sweep test start...");
  delay(1000);

  // 1. Both servos to one extreme
  Serial.println("Min position (150)...");
  pwm.setPWM(0, 0, 150);
  pwm.setPWM(1, 0, 150);
  delay(2000);

  // 2. Both servos to the opposite extreme
  Serial.println("Max position (600)...");
  pwm.setPWM(0, 0, 600);
  pwm.setPWM(1, 0, 600);
  delay(2000);

  // 3. Back to center and hold
  Serial.println("Return to center (375). Servos locked and ready.");
  pwm.setPWM(0, 0, 375);
  pwm.setPWM(1, 0, 375);
}

void loop() {
  // Nothing: servos stay powered and held at center.
}

