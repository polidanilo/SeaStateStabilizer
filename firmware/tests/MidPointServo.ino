#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40, Wire);

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(50);

  Serial.println("Centering servos at 90 degrees (PWM 375)...");
  delay(1000);

  pwm.setPWM(0, 0, 375);
  pwm.setPWM(1, 0, 375);
}

void loop() {
  // Keep both servos powered and locked at center.
}
