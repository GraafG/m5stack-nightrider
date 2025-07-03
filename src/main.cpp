#include <M5Atom.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <WebSocketsClient.h>
#include <SocketIOclient.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <Preferences.h>

// TallyArbiter Server Configuration
char tallyarbiter_host[40] = "192.168.188.47";  // Change to your TallyArbiter server IP
char tallyarbiter_port[6] = "4455";

// PCA9685 setup
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// Pin definitions for I2C (SDA, SCL)
#define SDA_PIN 21  // G21
#define SCL_PIN 25  // G25

// Tally Configuration - 4 channels, each with Red and Green
#define NUM_TALLIES 4
#define MAX_BRIGHTNESS 4095  // 12-bit PWM (0-4095)
#define MIN_BRIGHTNESS 0

// LED channel mapping (PCA9685 channels)
// Tally 1: Green=0, Red=1
// Tally 2: Green=2, Red=3  
// Tally 3: Green=4, Red=5
// Tally 4: Green=6, Red=7
const int TALLY_GREEN_CHANNELS[NUM_TALLIES] = {0, 2, 4, 6};
const int TALLY_RED_CHANNELS[NUM_TALLIES] = {1, 3, 5, 7};

// TallyArbiter variables - 4 virtual devices
SocketIOclient socket;
JsonDocument BusOptions;
JsonDocument Devices;
JsonDocument DeviceStates;

// Create 4 virtual device IDs and names
String DeviceIds[NUM_TALLIES] = {"unassigned", "unassigned", "unassigned", "unassigned"};
String DeviceNames[NUM_TALLIES];
String listenerDeviceNames[NUM_TALLIES];

// General Variables
bool networkConnected = false;
bool isReconnecting = false;
unsigned long currentReconnectTime = 0;
const unsigned long reconnectInterval = 5000;

// Tally states for each channel
enum TallyState {
  TALLY_OFF,
  TALLY_PREVIEW,  // Green
  TALLY_PROGRAM,  // Red
  TALLY_BOTH      // Red + Green (amber)
};

TallyState tallyStates[NUM_TALLIES] = {TALLY_OFF, TALLY_OFF, TALLY_OFF, TALLY_OFF};

// Preferences for storing configuration
Preferences preferences;

// WiFiManager instance
WiFiManager wifiManager;

// Forward declarations
void setAllTalliesOff();
void setTallyState(int tallyIndex, TallyState state);
void connectToNetwork();
void connectToServer();
void startReconnect();
void socket_event(socketIOmessageType_t type, uint8_t * payload, size_t length);
void socket_Connected(const char * payload, size_t length);
void socket_Disconnected(const char * payload, size_t length);
void socket_BusOptions(String payload);
void socket_Devices(String payload);
void socket_DeviceId(String payload);
void socket_DeviceStates(String payload);
void socket_Flash();
void socket_Reassign(String payload);
void setDeviceName();
void processTallyData();
String getBusTypeById(String busId);
String getBusLabelById(String busId);
int mapBusToTally(String busId, String busType, String busLabel);
void ws_emit(String event, const char *payload = NULL);

void setup() {
  Serial.begin(115200);
  delay(1000);  // Give serial time to initialize
  Serial.println();
  Serial.println("=== M5Stack Atom TallyArbiter Listener Starting ===");
  
  // Initialize M5Atom
  M5.begin(true, false, true);  // Init serial, disable I2C (we'll use custom pins), enable display
  Serial.println("M5Atom initialized");
  
  // Generate unique device names using MAC address
  byte mac[6];
  WiFi.macAddress(mac);
  String macString = String(mac[3], HEX) + String(mac[4], HEX) + String(mac[5], HEX);
  macString.toUpperCase();
  
  // Create 4 virtual device names
  for (int i = 0; i < NUM_TALLIES; i++) {
    DeviceNames[i] = "M5Atom-Tally" + String(i + 1) + "-" + macString;
    listenerDeviceNames[i] = "M5Atom-Tally" + String(i + 1) + "-" + macString;
    Serial.println("Device " + String(i + 1) + " name: " + DeviceNames[i]);
  }
  
  // Initialize custom I2C with specified pins
  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.println("I2C initialized on SDA:" + String(SDA_PIN) + " SCL:" + String(SCL_PIN));
  
  // Initialize PCA9685
  Serial.println("Initializing PCA9685...");
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);  // Set to 27MHz internal oscillator
  pwm.setPWMFreq(1600);  // High frequency for smooth dimming
  Serial.println("PCA9685 initialized");
  
  // Test all LEDs briefly
  Serial.println("Testing all LEDs...");
  for (int i = 0; i < 8; i++) {
    Serial.println("Testing LED channel " + String(i));
    pwm.setPWM(i, 0, MAX_BRIGHTNESS);
    delay(200);
    pwm.setPWM(i, 0, MIN_BRIGHTNESS);
    delay(100);
  }
  Serial.println("LED test complete");
  
  // Turn off all LEDs initially
  setAllTalliesOff();
  Serial.println("All tallies turned off");
  
  // Set M5Atom LED to indicate system is starting
  M5.dis.drawpix(0, 0x000010);  // Blue color - starting up
  Serial.println("M5Atom LED set to blue (starting up)");
  
  // Load preferences
  preferences.begin("tally-arbiter", false);
  if (preferences.getString("taHost").length() > 0) {
    String newHost = preferences.getString("taHost");
    Serial.println("Setting TallyArbiter host as " + newHost);
    newHost.toCharArray(tallyarbiter_host, 40);
  }
  if (preferences.getString("taPort").length() > 0) {
    String newPort = preferences.getString("taPort");
    Serial.println("Setting TallyArbiter port as " + newPort);
    newPort.toCharArray(tallyarbiter_port, 6);
  }
  preferences.end();
  
  // Connect to WiFi
  connectToNetwork();
  
  while (!networkConnected) {
    delay(200);
    M5.update();
  }
  
  // Set M5Atom LED to indicate WiFi connected
  M5.dis.drawpix(0, 0x001000);  // Green color - WiFi connected
  
  Serial.println("Device Names:");
  for (int i = 0; i < NUM_TALLIES; i++) {
    Serial.println("  Tally " + String(i + 1) + ": " + DeviceNames[i]);
  }
  Serial.println("Connecting to TallyArbiter server...");
  
  // Connect to TallyArbiter server
  connectToServer();
  
  delay(100);
}

void loop() {
  // Handle WebSocket communication
  socket.loop();
  
  // Update M5Atom button state
  M5.update();
  
  // Handle button press for WiFi reset
  if (M5.Btn.pressedFor(5000)) {
    Serial.println("WiFi Reset triggered!");
    wifiManager.resetSettings();
    ESP.restart();
  }
  
  // Handle reconnecting if disconnected
  if (isReconnecting) {
    unsigned long currentTime = millis();
    if (currentTime - currentReconnectTime >= reconnectInterval) {
      Serial.println("Trying to re-connect to TallyArbiter server...");
      connectToServer();
      currentReconnectTime = millis();
    }
  }
  
  // Quick button press shows device info
  if (M5.Btn.wasPressed()) {
    Serial.println("=== Device Info ===");
    for (int i = 0; i < NUM_TALLIES; i++) {
      Serial.println("Device " + String(i + 1) + " ID: " + DeviceIds[i]);
      Serial.println("Device " + String(i + 1) + " Name: " + DeviceNames[i]);
    }
    Serial.println("TallyArbiter Server: " + String(tallyarbiter_host) + ":" + String(tallyarbiter_port));
    Serial.println("IP Address: " + WiFi.localIP().toString());
    
    // Flash M5Atom LED to indicate button press
    M5.dis.drawpix(0, 0x100000);  // Red color
    delay(100);
    M5.dis.drawpix(0, 0x001000);  // Back to green
  }
  
  delay(50);
}

void setAllTalliesOff() {
  for (int i = 0; i < NUM_TALLIES; i++) {
    pwm.setPWM(TALLY_GREEN_CHANNELS[i], 0, MIN_BRIGHTNESS);
    pwm.setPWM(TALLY_RED_CHANNELS[i], 0, MIN_BRIGHTNESS);
    tallyStates[i] = TALLY_OFF;
  }
}

void setTallyState(int tallyIndex, TallyState state) {
  if (tallyIndex >= NUM_TALLIES) {
    Serial.println("Invalid tally index: " + String(tallyIndex));
    return;
  }
  
  tallyStates[tallyIndex] = state;
  
  int greenChannel = TALLY_GREEN_CHANNELS[tallyIndex];
  int redChannel = TALLY_RED_CHANNELS[tallyIndex];
  
  Serial.println("Setting Tally " + String(tallyIndex + 1) + " (Green:" + String(greenChannel) + ", Red:" + String(redChannel) + ") to state: " + String(state));
  
  switch (state) {
    case TALLY_OFF:
      pwm.setPWM(greenChannel, 0, MIN_BRIGHTNESS);
      pwm.setPWM(redChannel, 0, MIN_BRIGHTNESS);
      Serial.println("  -> OFF");
      break;
      
    case TALLY_PREVIEW:  // Green for Preview
      pwm.setPWM(greenChannel, 0, MAX_BRIGHTNESS);
      pwm.setPWM(redChannel, 0, MIN_BRIGHTNESS);
      Serial.println("  -> PREVIEW (Green ON)");
      break;
      
    case TALLY_PROGRAM:  // Red for Program/Live
      pwm.setPWM(greenChannel, 0, MIN_BRIGHTNESS);
      pwm.setPWM(redChannel, 0, MAX_BRIGHTNESS);
      Serial.println("  -> PROGRAM (Red ON)");
      break;
      
    case TALLY_BOTH:     // Both Red and Green (amber effect)
      pwm.setPWM(greenChannel, 0, MAX_BRIGHTNESS);
      pwm.setPWM(redChannel, 0, MAX_BRIGHTNESS);
      Serial.println("  -> BOTH (Red + Green ON)");
      break;
  }
}

void connectToNetwork() {
  // Set WiFi mode
  WiFi.mode(WIFI_STA);
  
  // Create custom parameters for TallyArbiter server configuration
  WiFiManagerParameter custom_taHost("taHost", "TallyArbiter Server IP", tallyarbiter_host, 40);
  WiFiManagerParameter custom_taPort("taPort", "TallyArbiter Port", tallyarbiter_port, 6);
  
  // Add custom parameters
  wifiManager.addParameter(&custom_taHost);
  wifiManager.addParameter(&custom_taPort);
  
  // Set save config callback
  wifiManager.setSaveConfigCallback([&custom_taHost, &custom_taPort]() {
    Serial.println("Configuration saved!");
    // Save the custom parameters
    preferences.begin("tally-arbiter", false);
    preferences.putString("taHost", custom_taHost.getValue());
    preferences.putString("taPort", custom_taPort.getValue());
    preferences.end();
    
    // Update global variables
    strcpy(tallyarbiter_host, custom_taHost.getValue());
    strcpy(tallyarbiter_port, custom_taPort.getValue());
  });
  
  // Set configuration portal timeout
  wifiManager.setConfigPortalTimeout(120);
  
  // Try to connect
  if (!wifiManager.autoConnect(listenerDeviceNames[0].c_str(), "tallyarbiter")) {
    Serial.println("Failed to connect to WiFi");
    ESP.restart();
  } else {
    Serial.println("WiFi connected!");
    Serial.println("IP address: " + WiFi.localIP().toString());
    networkConnected = true;
  }
}

void connectToServer() {
  Serial.println("Connecting to TallyArbiter host: " + String(tallyarbiter_host) + ":" + String(tallyarbiter_port));
  socket.onEvent(socket_event);
  socket.begin(tallyarbiter_host, atoi(tallyarbiter_port));
}

void startReconnect() {
  if (!isReconnecting) {
    isReconnecting = true;
    currentReconnectTime = millis();
    // Set M5Atom LED to yellow to indicate connection issues
    M5.dis.drawpix(0, 0x101000);  // Yellow color
  }
}

// WebSocket event handler
void socket_event(socketIOmessageType_t type, uint8_t * payload, size_t length) {
  String eventMsg = "";
  String eventType = "";
  String eventContent = "";

  switch (type) {
    case sIOtype_CONNECT:
      socket_Connected((char*)payload, length);
      break;

    case sIOtype_DISCONNECT:
      socket_Disconnected((char*)payload, length);
      break;

    case sIOtype_EVENT:
      eventMsg = (char*)payload;
      eventType = eventMsg.substring(2, eventMsg.indexOf("\"", 2));
      eventContent = eventMsg.substring(eventType.length() + 4);
      eventContent.remove(eventContent.length() - 1);

      Serial.println("Got event '" + eventType + "', data: " + eventContent);

      if (eventType == "bus_options") socket_BusOptions(eventContent);
      if (eventType == "deviceId") socket_DeviceId(eventContent);
      if (eventType == "devices") socket_Devices(eventContent);
      if (eventType == "device_states") socket_DeviceStates(eventContent);
      if (eventType == "flash") socket_Flash();
      if (eventType == "reassign") socket_Reassign(eventContent);

      break;

    default:
      break;
  }
}

void socket_Connected(const char * payload, size_t length) {
  Serial.println("Connected to TallyArbiter server!");
  isReconnecting = false;
  
  // Set M5Atom LED to solid green
  M5.dis.drawpix(0, 0x001000);  // Green color
  
  // Register all 4 virtual devices
  for (int i = 0; i < NUM_TALLIES; i++) {
    String deviceObj = "{\"deviceId\": \"" + DeviceIds[i] + "\", \"listenerType\": \"" + listenerDeviceNames[i] + "\", \"canBeReassigned\": true, \"canBeFlashed\": true, \"supportsChat\": false }";
    char charDeviceObj[1024];
    strcpy(charDeviceObj, deviceObj.c_str());
    ws_emit("listenerclient_connect", charDeviceObj);
    delay(100); // Small delay between registrations
  }
}

void socket_Disconnected(const char * payload, size_t length) {
  Serial.println("Disconnected from TallyArbiter server, attempting to reconnect...");
  setAllTalliesOff();  // Turn off all tallies when disconnected
  startReconnect();
}

void socket_BusOptions(String payload) {
  deserializeJson(BusOptions, payload);
}

void socket_Devices(String payload) {
  deserializeJson(Devices, payload);
  setDeviceName();
}

void socket_DeviceId(String payload) {
  String newDeviceId = payload.substring(1, payload.length() - 1);  // Remove quotes
  
  // Find which virtual device this ID belongs to and update it
  for (int i = 0; i < NUM_TALLIES; i++) {
    if (DeviceIds[i] == "unassigned") {
      DeviceIds[i] = newDeviceId;
      Serial.println("Assigned Device ID " + newDeviceId + " to Tally " + String(i + 1));
      break;
    }
  }
  setDeviceName();
}

void socket_DeviceStates(String payload) {
  deserializeJson(DeviceStates, payload);
  processTallyData();
}

void socket_Flash() {
  Serial.println("Flash command received!");
  
  // Flash all tallies white briefly
  for (int i = 0; i < NUM_TALLIES; i++) {
    pwm.setPWM(TALLY_GREEN_CHANNELS[i], 0, MAX_BRIGHTNESS);
    pwm.setPWM(TALLY_RED_CHANNELS[i], 0, MAX_BRIGHTNESS);
  }
  
  delay(200);
  setAllTalliesOff();
  delay(200);
  
  // Repeat flash 2 more times
  for (int flash = 0; flash < 2; flash++) {
    for (int i = 0; i < NUM_TALLIES; i++) {
      pwm.setPWM(TALLY_GREEN_CHANNELS[i], 0, MAX_BRIGHTNESS);
      pwm.setPWM(TALLY_RED_CHANNELS[i], 0, MAX_BRIGHTNESS);
    }
    delay(200);
    setAllTalliesOff();
    delay(200);
  }
  
  // Restore previous tally states
  processTallyData();
}

void socket_Reassign(String payload) {
  // Handle device reassignment
  Serial.println("Reassign command received: " + payload);
  // Implementation would depend on specific reassignment protocol
}

void setDeviceName() {
  for (JsonVariant device : Devices.as<JsonArray>()) {
    if (device["id"].as<String>() == DeviceId) {
      DeviceName = device["name"].as<String>();
      break;
    }
  }
  Serial.println("Device Name: " + DeviceName);
}

void processTallyData() {
  Serial.println("=== Processing Tally Data ===");
  
  // Reset all tallies first
  for (int i = 0; i < NUM_TALLIES; i++) {
    setTallyState(i, TALLY_OFF);
  }
  
  // The DeviceStates contains bus information for ALL devices
  // Each entry has: busId, deviceId, and sources array
  // We need to find buses that have sources assigned to ANY device
  
  for (JsonVariant deviceState : DeviceStates.as<JsonArray>()) {
    String busId = deviceState["busId"].as<String>();
    String deviceIdInBus = deviceState["deviceId"].as<String>();
    JsonArray sources = deviceState["sources"].as<JsonArray>();
    
    // If this bus has sources (i.e., is active), process it
    if (sources.size() > 0) {
      String busType = getBusTypeById(busId);
      String busLabel = getBusLabelById(busId);
      
      Serial.println("Active bus: " + busId + " Type: " + busType + " Label: " + busLabel + " Sources: " + String(sources.size()));
      
      // Map this active bus to a tally channel
      int tallyIndex = mapBusToTally(busId, busType, busLabel);
      
      if (tallyIndex >= 0 && tallyIndex < NUM_TALLIES) {
        Serial.println("Mapping to Tally " + String(tallyIndex + 1));
        
        // Remove quotes from busType for comparison
        String cleanBusType = busType;
        cleanBusType.replace("\"", "");
        
        if (cleanBusType == "preview") {
          if (tallyStates[tallyIndex] == TALLY_PROGRAM) {
            setTallyState(tallyIndex, TALLY_BOTH);  // Both preview and program
          } else {
            setTallyState(tallyIndex, TALLY_PREVIEW);  // Just preview
          }
        } else if (cleanBusType == "program") {
          if (tallyStates[tallyIndex] == TALLY_PREVIEW) {
            setTallyState(tallyIndex, TALLY_BOTH);  // Both preview and program
          } else {
            setTallyState(tallyIndex, TALLY_PROGRAM);  // Just program
          }
        }
      } else {
        Serial.println("No tally mapping for this bus");
      }
    }
  }
  
  Serial.println("=== Tally Data Processing Complete ===");
}

String getBusTypeById(String busId) {
  for (JsonVariant busOption : BusOptions.as<JsonArray>()) {
    if (busOption["id"].as<String>() == busId) {
      return busOption["type"].as<String>();
    }
  }
  return "invalid";
}

String getBusLabelById(String busId) {
  for (JsonVariant busOption : BusOptions.as<JsonArray>()) {
    if (busOption["id"].as<String>() == busId) {
      return busOption["label"].as<String>();
    }
  }
  return "unknown";
}

int mapBusToTally(String busId, String busType, String busLabel) {
  // Map buses to tallies based on their label/type
  // This gives you predictable control over which bus controls which tally
  
  // Remove quotes from busType for comparison
  busType.replace("\"", "");
  
  if (busType == "preview") {
    return 0;  // Preview always goes to Tally 1 (Green LED)
  } else if (busType == "program") {
    return 0;  // Program always goes to Tally 1 (Red LED) - same tally as preview
  } else if (busLabel == "Aux 1") {
    return 1;  // Aux 1 goes to Tally 2
  } else if (busLabel == "Aux 2") {
    return 2;  // Aux 2 goes to Tally 3
  } else if (busLabel == "Aux 3") {
    return 3;  // Aux 3 goes to Tally 4
  }
  
  // For any other buses, create a simple hash from busId
  unsigned long hash = 0;
  for (int i = 0; i < busId.length(); i++) {
    hash = hash * 31 + busId.charAt(i);
  }
  return hash % NUM_TALLIES;
}

void ws_emit(String event, const char *payload) {
  if (payload) {
    String msg = "[\"" + event + "\"," + payload + "]";
    socket.sendEVENT(msg);
  } else {
    String msg = "[\"" + event + "\"]";
    socket.sendEVENT(msg);
  }
}
