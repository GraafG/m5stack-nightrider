<!-- Use this file to provide workspace-specific custom instructions to Copilot. For more details, visit https://code.visualstudio.com/docs/copilot/copilot-customization#_use-a-githubcopilotinstructionsmd-file -->

# M5Stack Atom 2 PCA9685 Nightrider Project

This project is for the M5Stack Atom 2 microcontroller using the Arduino framework via PlatformIO. The project controls a PCA9685 PWM driver to create a nightrider effect with 8 LEDs.

## Hardware Configuration
- M5Stack Atom 2 development board
- PCA9685 16-channel PWM driver (using 8 channels)
- 8 LEDs connected to channels 0-7 of the PCA9685
- I2C connections: SDA on G21, SCL on G25

## Code Guidelines
- Use the M5Atom library for hardware initialization
- Use the Adafruit PWM Servo Driver library for PCA9685 control
- Maintain proper I2C communication protocols
- Follow Arduino C++ coding standards
- Use appropriate delays for smooth LED transitions
