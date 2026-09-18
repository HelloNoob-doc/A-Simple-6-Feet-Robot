#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm(0x40);

void setup() {
  Wire.begin(21, 22);
  pwm.begin();
  pwm.setPWMFreq(50);
}

void loop() {
  pwm.setPWM(15, 0, 287);  // about 1.4 ms
  delay(1500);

  pwm.setPWM(15, 0, 367);  // about 1.8 ms
  delay(1500);
}