#include <M5Atom.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// PCA9685 setup
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// Pin definitions for I2C (SDA, SCL)
#define SDA_PIN 21  // G21
#define SCL_PIN 25  // G25

// LED configuration
#define NUM_LEDS 8
#define MAX_BRIGHTNESS 4095  // 12-bit PWM (0-4095)
#define MIN_BRIGHTNESS 0
#define FADE_STEPS 50
#define DELAY_MS 30

// Nightrider variables
int currentLed = 0;
bool direction = true;  // true = forward, false = backward
int brightness[NUM_LEDS];

void setup() {
  // Initialize M5Atom
  M5.begin(true, false, true);  // Init serial, disable I2C (we'll use custom pins), enable display
  
  // Initialize custom I2C with specified pins
  Wire.begin(SDA_PIN, SCL_PIN);
  
  // Initialize PCA9685
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);  // Set to 27MHz internal oscillator
  pwm.setPWMFreq(1600);  // Analog servos run at ~50 Hz updates
  
  // Initialize brightness array
  for (int i = 0; i < NUM_LEDS; i++) {
    brightness[i] = MIN_BRIGHTNESS;
  }
  
  Serial.begin(115200);
  Serial.println("M5Stack Atom Nightrider Effect Started!");
  
  // Set M5Atom LED to indicate system is running
  M5.dis.drawpix(0, 0x001000);  // Green color
}

void loop() {
  // Update M5Atom button state
  M5.update();
  
  // Fade out all LEDs gradually
  for (int i = 0; i < NUM_LEDS; i++) {
    if (brightness[i] > MIN_BRIGHTNESS) {
      brightness[i] -= (MAX_BRIGHTNESS / FADE_STEPS);
      if (brightness[i] < MIN_BRIGHTNESS) {
        brightness[i] = MIN_BRIGHTNESS;
      }
    }
  }
  
  // Set current LED to maximum brightness
  brightness[currentLed] = MAX_BRIGHTNESS;
  
  // Update all LED outputs on PCA9685
  for (int i = 0; i < NUM_LEDS; i++) {
    pwm.setPWM(i, 0, brightness[i]);
  }
  
  // Move to next LED position
  if (direction) {
    currentLed++;
    if (currentLed >= NUM_LEDS - 1) {
      direction = false;  // Reverse direction
    }
  } else {
    currentLed--;
    if (currentLed <= 0) {
      direction = true;   // Forward direction
    }
  }
  
  // Check if button is pressed to change speed
  if (M5.Btn.wasPressed()) {
    // You can add speed control or other features here
    Serial.println("Button pressed!");
    // Change M5Atom LED color briefly
    M5.dis.drawpix(0, 0x100000);  // Red color
    delay(100);
    M5.dis.drawpix(0, 0x001000);  // Back to green
  }
  
  delay(DELAY_MS);
}
