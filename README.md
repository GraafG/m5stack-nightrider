# M5Stack Atom 2 TallyArbiter Listener

This project creates a TallyArbiter listener client using an M5Stack Atom 2 microcontroller and a PCA9685 PWM driver controlling 4 tally lights (8 LEDs total - red and green for each tally).

## Hardware Requirements

- M5Stack Atom 2 development board
- PCA9685 16-channel PWM driver board
- 8 LEDs (4 red, 4 green)
- 8 current-limiting resistors (220Ω - 470Ω depending on LED type)
- Jumper wires
- Breadboard (optional)

## Wiring

### I2C Connection (M5Stack Atom 2 to PCA9685):
- **SDA**: G21 → SDA on PCA9685
- **SCL**: G25 → SCL on PCA9685
- **VCC**: 5V → VCC on PCA9685
- **GND**: GND → GND on PCA9685

### Tally LED Connections (PCA9685 channels):
- **Tally 1**: Green LED → Channel 0, Red LED → Channel 1
- **Tally 2**: Green LED → Channel 2, Red LED → Channel 3
- **Tally 3**: Green LED → Channel 4, Red LED → Channel 5
- **Tally 4**: Green LED → Channel 6, Red LED → Channel 7

Connect LEDs with appropriate current-limiting resistors between PCA9685 channels and ground.

## Features

- 4 independent tally light channels
- TallyArbiter WebSocket client integration
- WiFi configuration via captive portal
- Red LED for Program/Live tally
- Green LED for Preview tally
- Both LEDs (amber) for Preview + Program
- WiFi credentials and server settings stored in flash memory
- Visual feedback via M5Atom's built-in LED
- Device flash command support
- Automatic reconnection on disconnect

## TallyArbiter Integration

This device acts as a listener client that connects to a TallyArbiter server:
- Receives tally state updates via WebSocket
- Responds to flash commands
- Supports device reassignment
- Compatible with TallyArbiter v3.x

## Setup and Configuration

### Initial Setup
1. **Build and Upload**: Flash the firmware to your M5Stack Atom 2
2. **WiFi Configuration**: On first boot, the device creates a WiFi access point named `M5Atom-TallyLight-XXXXXX`
3. **Connect to AP**: Connect to this access point with password `tallyarbiter`
4. **Configure Settings**: A captive portal will open where you can:
   - Select your WiFi network and enter password
   - Enter your TallyArbiter server IP address
   - Set the TallyArbiter server port (default: 4455)
5. **Save Configuration**: Click "Save" to store settings and connect

### TallyArbiter Server Setup
1. In TallyArbiter, go to the **Devices** page
2. Your M5Atom device should appear as `M5Atom-TallyLight-XXXXXX`
3. Assign the device to bus sources as needed
4. Each of the 4 tally channels will respond to different bus assignments

### Device Controls
- **Short Button Press**: Display device information in serial monitor
- **Long Button Press (5 seconds)**: Reset WiFi settings and restart
- **M5Atom LED Indicators**:
  - **Blue**: Starting up
  - **Green**: Connected to TallyArbiter server
  - **Yellow**: Connection issues/reconnecting
  - **Red Flash**: Button pressed

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

- `tallyarbiter_host`: Default TallyArbiter server IP
- `tallyarbiter_port`: Default TallyArbiter server port
- `MAX_BRIGHTNESS`: Maximum LED brightness (0-4095)
- `SDA_PIN` and `SCL_PIN`: I2C pin assignments
- `NUM_TALLIES`: Number of tally channels (currently 4)

## Libraries Used

- M5Atom: Official M5Stack Atom library
- Adafruit PWM Servo Driver Library: For PCA9685 control
- ArduinoJson: For JSON parsing of TallyArbiter messages
- WebSockets: For WebSocket client communication
- WiFiManager: For WiFi configuration portal
- Wire: For I2C communication

## Troubleshooting

1. **LEDs not lighting up**: Check I2C connections and power supply
2. **Can't connect to TallyArbiter**: Verify server IP and port in configuration
3. **WiFi connection issues**: Hold button for 5 seconds to reset WiFi settings
4. **Device not appearing in TallyArbiter**: Check WebSocket connection and server reachability
5. **Serial output issues**: Check baud rate is set to 115200
6. **Upload issues**: Ensure correct board is selected and USB cable supports data transfer

## TallyArbiter Compatibility

This listener client is compatible with:
- TallyArbiter v3.x and newer
- Standard WebSocket protocol
- JSON-based messaging format
- Device reassignment feature
- Flash command support

For more information about TallyArbiter, visit: https://github.com/josephdadams/TallyArbiter

## License

This project is open source and available under the MIT License.
