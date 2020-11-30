#define THROTTLE_PIN A0

#include "Joystick.h"
#include <EEPROM.h>

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_GAMEPAD,
                   0, 0,                  // Button Count, Hat Switch Count
                   false, false, false,   // No X, Y, or Z Axis
                   false, false, false,   // No Rx, Ry, or Rz
                   false, true,           // No rudder, yes throttle
                   false, false, false);  // No accelerator, brake, or steering

// We need to map the hall sensor values to the range -512 to 512, but hall sensors aren't typically perfectly linear.
// So, we define five control points, correspondoing to 0%, 25%, 50%, 75%, and 100% throttle value, then interpolate our
// output value based on where the current throttle position lies in these ranges.
int CONTROL_POINTS[] = {300, 450, 600, 750, 900};

double lerp(double x0, double x1, double mu)
{
  return x0*(1-mu)+x1*mu;
}

void setup() {
  // Initialize Button Pins
  pinMode(THROTTLE_PIN, INPUT);

  Serial.begin(115200);
  // We use Serial.readBytes(), so we want to time out quickly if we wait too long for data
  Serial.setTimeout(50);

  Joystick.setThrottleRange(0, 1024);
  Joystick.begin(false);

  initEeprom();
}

// Store new EEPROM values if this is the first time the board has been used for this sketch,
// otherwise read the stored values into the CONTROL_POINTS array
void initEeprom() {
  byte is_initialized = EEPROM[0];
  if (is_initialized != 0xed) {
    for (int i = 0; i < 5; i++) {
      EEPROM.put(i + 1, CONTROL_POINTS[i]);
    }
    EEPROM.put(0, 0xed);
  } else {
    for (int i = 0; i < 5; i++) {
      CONTROL_POINTS[i] = EEPROM[i + 1];
    }
  }
}

void readControlPoint(int throttleValue) {
  if (Serial.availableForWrite() == 0) {
    // We ignore serial input if the output isn't being read in order to avoid blocking the joystick function
    return;
  }

  byte input[2];
  Serial.readBytes(input, 2);
  if (input[0] != 0x01) {
    // This wasn't valid control data
    Serial.write(0x15);
    return;
  }

  int controlPoint = input[1];

  if (controlPoint > 4) {
    Serial.write(0x15);
    return; // This wasn't a valid control point
  }

  CONTROL_POINTS[controlPoint] = throttleValue;
  EEPROM[controlPoint] = throttleValue;
  Serial.write(0x06);
}

void loop() {
  int value = analogRead(THROTTLE_PIN);

  // If there's data on the serial port, see if we need to set a control point
  if (Serial.available() >= 2) {
    readControlPoint(value);
  }

  double lerped_value = 0;

  // Find our quadrant
  for (int i = 0; i < 4; i++) {
    if (value >= CONTROL_POINTS[i] && value < CONTROL_POINTS[i + 1]) {
      int maxVal = CONTROL_POINTS[i+1] - CONTROL_POINTS[i];
      value -= CONTROL_POINTS[i];
      double mu = double(value) / double(maxVal);
      lerped_value = lerp(i * .25, (i + 1) * .25, mu);
    }
  }

  Joystick.setThrottle(lerped_value * 1024.0);
  Joystick.sendState();

  delay(10);
}
