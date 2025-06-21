#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>
#include <EEPROM.h>

// WiFi credentials - replace with your network details
const char* ssid = "Redding Flower Farm_2G";
const char* password = "27Peonies";
//const char* ssid = "Nick's Pixel 5";
//const char* password = "D1gby4321.";

// Create servo and web server objects
Servo myServo;
ESP8266WebServer server(80);

// Pin definitions (ESP8266 safe pins)
const int servoPin = 2;  // GPIO 2 (D4 on NodeMCU) - safe for servo
const int button1Pin = 14; // GPIO 14 (D5) - ON button
const int button2Pin = 12; // GPIO 12 (D6) - OFF button
const int wifiLedPin = 4; // GPIO 4 (D2) - WiFi status LED
const int healthyLedPin = 16;  // GPIO 16 (D0 on NodeMCU)

// EEPROM settings
#define EEPROM_SIZE 4    // We only need 4 bytes
#define SERVO_POSITION_ADDR 0  // Address to store servo position

// Servo position tracking (since we can't read actual position)
int currentServoPosition = 0;  // Track the last commanded position
bool button1LastState = HIGH;
bool button2LastState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

void setup() {
  Serial.begin(115200);
  
  // Initialize button pins
  pinMode(button1Pin, INPUT_PULLUP);  // ON button
  pinMode(button2Pin, INPUT_PULLUP);  // OFF button
  pinMode(wifiLedPin, OUTPUT);        // WiFi status LED
  pinMode(healthyLedPin, OUTPUT);
  
  // Turn on WiFi LED to indicate no connection
  digitalWrite(wifiLedPin, HIGH);
  
  // Tunr off the healthyLED pin to indicate not ready/healthy
  digitalWrite(healthyLedPin, HIGH);

  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);
  
  // Read saved servo position from EEPROM
  currentServoPosition = EEPROM.read(SERVO_POSITION_ADDR);
  
  // Validate the saved position (in case EEPROM is uninitialized)
  if (currentServoPosition != 0 && currentServoPosition != 90) {
    currentServoPosition = 0;  // Default to 0 if invalid value
    saveServoPosition();       // Save the corrected default
  }
  
  Serial.print("Restored servo position from EEPROM: ");
  Serial.println(currentServoPosition);
  
  // Attach servo to pin and restore last position
  myServo.attach(servoPin, 1000, 2000);
  int actualAngle = currentServoPosition * 2;  // Multiply by 2 for your servo
  myServo.write(actualAngle);  // Restore to saved position
  Serial.print("Servo moved to restored position - Logical: ");
  Serial.print(currentServoPosition);
  Serial.print(", Actual: ");
  Serial.println(actualAngle);
  
  Serial.println("testing angles...");
  myServo.write(90 * 2);
  delay(1000);
  myServo.write(0);
  delay(1000);
  myServo.write(120 * 2);

  // Enhanced WiFi connection with timeout and cleanup
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);  // Clear any previous connection
    delay(1000);

    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 50) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.print("Connected! IP address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println();
        Serial.println("WiFi connection failed!");
        // Reset and try again
        ESP.restart();
    }
  
  // Define web server routes
  server.on("/", handleRoot);
  server.on("/servo/on", handleServoOn);
  server.on("/servo/off", handleServoOff);
  server.on("/servo/custom", handleServoCustom);
  server.on("/status", handleStatus);
  
  // Start server
  server.begin();
  Serial.println("Web server started");
  
  /*
  myServo.write(45 * 2);
  delay(200);
  myServo.write(0);
  delay(200);
  myServo.write(45 * 2);
  delay(1000);
  myServo.write(0);
  */

}

// Function to check button presses
void checkButtons() {
  bool button1State = digitalRead(button1Pin);
  bool button2State = digitalRead(button2Pin);
  
  // Button 1 (ON) - Check for press (HIGH to LOW transition)
  if (button1LastState == HIGH && button1State == LOW) {
    if (millis() - lastDebounceTime > debounceDelay) {
      // Button 1 pressed - turn servo ON
      Serial.println("Physical button: Servo ON");
      moveServoTo(90);
      lastDebounceTime = millis();
    }
  }
  
  // Button 2 (OFF) - Check for press (HIGH to LOW transition)
  if (button2LastState == HIGH && button2State == LOW) {
    if (millis() - lastDebounceTime > debounceDelay) {
      // Button 2 pressed - turn servo OFF
      Serial.println("Physical button: Servo OFF");
      moveServoTo(0);
      lastDebounceTime = millis();
    }
  }
  
  button1LastState = button1State;
  button2LastState = button2State;
}

// Centralized servo movement function
void moveServoTo(int logicalAngle) {
  int actualAngle = logicalAngle * 2;  // Apply 2x multiplier
  myServo.write(actualAngle);
  currentServoPosition = logicalAngle;
  saveServoPosition();
  
  Serial.print("Servo moved - Logical: ");
  Serial.print(logicalAngle);
  Serial.print(", Actual: ");
  Serial.println(actualAngle);
}

// Function to save servo position to EEPROM
void saveServoPosition() {
  EEPROM.write(SERVO_POSITION_ADDR, currentServoPosition);
  EEPROM.commit();  // Important: commit changes to flash memory
  Serial.print("Servo position saved to EEPROM: ");
  Serial.println(currentServoPosition);
}

void loop() {
  server.handleClient();
  
  // Check physical buttons
  checkButtons();
  
  // Check WiFi status and update LED
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(wifiLedPin, HIGH);  // Turn on LED if WiFi disconnected
    digitalWrite(healthyLedPin, HIGH);  // Turn off healthy LED if WiFi disconnected
  } else {
    digitalWrite(wifiLedPin, LOW);   // Turn off LED if WiFi connected
    digitalWrite(healthyLedPin, LOW);  // Turn on healthy LED if WiFi disconnected
  }
  
  // ESP8266 needs this for WiFi stability
  yield();
}

// Serve the main web page
void handleRoot() {
 String html = "<!DOCTYPE html>"
   "<html>"
   "<head>"
   "<title>ESP8266 Servo Control</title>"
   "<meta name='viewport' content='width=device-width, initial-scale=1'>"
   "<style>"
   "body { font-family: Arial, sans-serif; max-width: 600px; margin: 50px auto; padding: 20px; background-color: #f0f0f0; }"
   ".container { background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); text-align: center; }"
   "h1 { color: #333; margin-bottom: 30px; }"
   ".status { font-size: 18px; margin: 20px 0; padding: 15px; border-radius: 5px; font-weight: bold; }"
   ".status.on { background-color: #d4edda; color: #155724; border: 1px solid #c3e6cb; }"
   ".status.off { background-color: #f8d7da; color: #721c24; border: 1px solid #f5c6cb; }"
   ".button { font-size: 16px; padding: 15px 30px; margin: 10px; border: none; border-radius: 5px; cursor: pointer; transition: background-color 0.3s; }"
   ".button.on { background-color: #28a745; color: white; }"
   ".button.on:hover { background-color: #218838; }"
   ".button.off { background-color: #dc3545; color: white; }"
   ".button.off:hover { background-color: #c82333; }"
   "</style>"
   "</head>"
   "<body>"
   "<div class='container'>"
   "<h1>ESP8266 Servo Control</h1>"
   "<div id='status' class='status'>Loading status...</div>"
   "<div>"
   "<button class='button on' onclick='controlServo(\"on\")'>Turn ON (90 degrees)</button>"
   "<button class='button off' onclick='controlServo(\"off\")'>Turn OFF (0 degrees)</button>"
   "</div>"
   "<div style='margin-top: 20px;'>"
   "<input type='number' id='customAngle' min='0' max='180' value='90' style='padding: 10px; font-size: 16px; width: 80px; text-align: center; border: 2px solid #ddd; border-radius: 5px;'>"
   "<button class='button' onclick='setCustomAngle()' style='background-color: #007bff; margin-left: 10px;'>Set Angle</button>"
   "</div>"
   "</div>"
   "<script>"
   "updateStatus();"
   "setInterval(updateStatus, 2000);"
   "function controlServo(action) {"
   "fetch('/servo/' + action)"
   ".then(response => response.text())"
   ".then(data => { console.log('Success:', data); updateStatus(); })"
   ".catch(error => { console.error('Error:', error); alert('Failed to control servo'); });"
   "}"
   "function setCustomAngle() {"
   "const angle = document.getElementById('customAngle').value;"
   "if (angle < 0 || angle > 180) { alert('Please enter an angle between 0 and 180'); return; }"
   "fetch('/servo/custom?angle=' + angle)"
   ".then(response => response.text())"
   ".then(data => { console.log('Success:', data); updateStatus(); })"
   ".catch(error => { console.error('Error:', error); alert('Failed to set custom angle'); });"
   "}"
   "function updateStatus() {"
   "fetch('/status')"
   ".then(response => response.json())"
   ".then(data => {"
   "const statusDiv = document.getElementById('status');"
   "if (data.position == 90) {"
   "statusDiv.innerHTML = 'Servo Status: ON (90 degrees)';"
   "statusDiv.className = 'status on';"
   "} else {"
   "statusDiv.innerHTML = 'Servo Status: OFF (' + data.position + ' degrees)';"
   "statusDiv.className = 'status off';"
   "}"
   "})"
   ".catch(error => {"
   "console.error('Error fetching status:', error);"
   "document.getElementById('status').innerHTML = 'Status: Error';"
   "});"
   "}"
   "</script>"
   "</body>"
   "</html>";
 
 server.send(200, "text/html", html);
}

// Handle servo ON command
void handleServoOn() {
  myServo.write(90 * 2);  // Move to 90 degrees
  currentServoPosition = 90;  // Update position tracking
  saveServoPosition();        // Save to EEPROM
  Serial.println("Servo turned ON (90)");
  server.send(200, "text/plain", "Servo turned ON");
}

// Handle servo OFF command
void handleServoOff() {
  myServo.write(0);   // Move to 0 degrees
  currentServoPosition = 0;   // Update position tracking
  saveServoPosition();        // Save to EEPROM
  Serial.println("Servo turned OFF (0)");
  server.send(200, "text/plain", "Servo turned OFF");
}

// Handle custom angle command
void handleServoCustom() {
 if (server.hasArg("angle")) {
   int logicalAngle = server.arg("angle").toInt();
   
   // Validate angle range
   if (logicalAngle < 0 || logicalAngle > 180) {
     server.send(400, "text/plain", "Invalid angle. Must be 0-180.");
     return;
   }
   
   int actualAngle = logicalAngle * 2;  // Multiply by 2 for your servo
   myServo.write(actualAngle);
   currentServoPosition = logicalAngle;  // Store the logical position
   saveServoPosition();                  // Save to EEPROM
   
   Serial.print("Servo set to custom angle - Logical: ");
   Serial.print(logicalAngle);
   Serial.print(", Actual: ");
   Serial.println(actualAngle);
   
   server.send(200, "text/plain", "Servo moved to " + String(logicalAngle) + " degrees");
 } else {
   server.send(400, "text/plain", "Missing angle parameter");
 }
}

// Handle status request
void handleStatus() {
  String json = "{\"position\":" + String(currentServoPosition) + 
                ",\"state\":\"" + String(currentServoPosition == 90 ? "on" : "off") + "\"}";
  server.send(200, "application/json", json);
}