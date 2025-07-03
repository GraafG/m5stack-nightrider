# M5Stack Atom 2 Nightrider Effect

This project creates a classic nightrider LED effect using an M5Stack Atom 2 microcontroller and a PCA9685 PWM driver controlling 8 LEDs.

## Hardware Requirements

- M5Stack Atom 2 development board
- PCA9685 16-channel PWM driver board
- 8 LEDs
- 8 current-limiting resistors (220Ω - 470Ω depending on LED type)
- Jumper wires
- Breadboard (optional)

## Wiring

### I2C Connection (M5Stack Atom 2 to PCA9685):
- **SDA**: G21 → SDA on PCA9685
- **SCL**: G25 → SCL on PCA9685
- **VCC**: 5V → VCC on PCA9685
- **GND**: GND → GND on PCA9685

### LED Connections:
Connect LEDs to PCA9685 channels 0-7 with appropriate current-limiting resistors.

## Features

- Classic nightrider sweeping effect
- Smooth LED fading with trailing effect
- Uses all 8 channels of the PCA9685
- Button interaction (press the M5Atom button for effects)
- Serial output for debugging
- Visual feedback via M5Atom's built-in LED

## Building and Uploading

### Prerequisites
1. Install PlatformIO IDE or PlatformIO Core
2. Install the PlatformIO extension for VS Code (recommended)

### Build Commands
```bash
# Build the project
pio run

# Upload to device
pio run --target upload

# Open serial monitor
pio device monitor
```

### VS Code Integration
- Use Ctrl+Alt+B to build
- Use Ctrl+Alt+U to upload
- Use Ctrl+Alt+S to open serial monitor

## Configuration

You can adjust the following parameters in `src/main.cpp`:

- `DELAY_MS`: Speed of the effect (lower = faster)
- `FADE_STEPS`: How many steps to fade out LEDs
- `MAX_BRIGHTNESS`: Maximum LED brightness (0-4095)
- `SDA_PIN` and `SCL_PIN`: I2C pin assignments

## Libraries Used

- M5Atom: Official M5Stack Atom library
- Adafruit PWM Servo Driver Library: For PCA9685 control
- Wire: For I2C communication

## Troubleshooting

1. **LEDs not lighting up**: Check I2C connections and power supply
2. **Erratic behavior**: Verify I2C pull-up resistors (usually built-in on PCA9685 boards)
3. **Serial output issues**: Check baud rate is set to 115200
4. **Upload issues**: Ensure correct board is selected and USB cable supports data transfer

## License

This project is open source and available under the MIT License.
