<!-- Use this file to provide workspace-specific custom instructions to Copilot. For more details, visit https://code.visualstudio.com/docs/copilot/copilot-customization#_use-a-githubcopilotinstructionsmd-file -->

# M5Stack Atom 2 TallyArbiter Listener Project

This project is for the M5Stack Atom 2 microcontroller using the Arduino framework via PlatformIO. The project acts as a TallyArbiter listener client that controls a PCA9685 PWM driver to operate 4 tally lights (8 LEDs total).

## Hardware Configuration
- M5Stack Atom 2 development board
- PCA9685 16-channel PWM driver (using 8 channels)
- 4 tally lights, each with red and green LEDs
- I2C connections: SDA on G21, SCL on G25
- LED mapping: Tally1(G0,R1), Tally2(G2,R3), Tally3(G4,R5), Tally4(G6,R7)

## Software Architecture
- TallyArbiter WebSocket client implementation
- WiFiManager for network configuration
- ArduinoJson for message parsing
- Preferences for persistent storage
- PCA9685 PWM control for LED brightness

## Code Guidelines
- Use the M5Atom library for hardware initialization
- Use the Adafruit PWM Servo Driver library for PCA9685 control
- Follow TallyArbiter listener client protocol
- Implement proper error handling and reconnection logic
- Maintain WebSocket communication protocols
- Follow Arduino C++ coding standards
- Use appropriate PWM values for LED control

## TallyArbiter Integration
- Red LED indicates Program/Live tally state
- Green LED indicates Preview tally state
- Both LEDs (amber) indicate Preview + Program state
- Support for device flashing and reassignment
- Compatible with TallyArbiter v3.x protocol
