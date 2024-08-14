#include <TFT_eSPI.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <Adafruit_GFX.h>
#include "spo2_algorithm.h"
#include <ArduinoBLE.h> // Corrected the include directive

// BLE UUIDs
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// Declare display and sensors
TFT_eSPI tft = TFT_eSPI();
MAX30105 particleSensor;

// BLE service and characteristic
BLEService bleService(SERVICE_UUID);
BLECharacteristic bleCharacteristic(CHARACTERISTIC_UUID, BLERead | BLEWrite | BLENotify, 512);

// Constants and variables for BPM and SpO2 calculation
const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;

float beatsPerMinute;
int beatAvg;
float spo2; // Variable to store SpO2 value

#define BUFFER_SIZE 100
uint32_t irBuffer[BUFFER_SIZE]; // Infrared LED sensor data
uint32_t redBuffer[BUFFER_SIZE]; // Red LED sensor data
int32_t bufferLength; // Data length
int32_t spo2Value; // SpO2 value
int8_t validSPO2; // Indicator to show if the SpO2 calculation is valid
int32_t heartRate; // Heart rate value
int8_t validHeartRate; // Indicator to show if the heart rate calculation is valid

int steps_taken = 2000; // Reduced steps taken
int step_goal = 10000;
int stepsbar_len = 0;
int spx = 2; 
int spy = 2; 
int uw = 4; 
int uh = 8; 
int us = 2; 
int cell_colour = TFT_RED;

struct SensorData {
  String timestamp;
  float bpm;
  float spo2;
};

SensorData dailyData[1440]; // Assuming data collection every minute
int dataCount = 0;

// Constants for initial time
const int initialHour = 9;
const int initialMinute = 35;
const long millisInOneSecond = 1000;
const long millisInOneMinute = 60 * millisInOneSecond;
const long millisInOneHour = 60 * millisInOneMinute;

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing...");

  // Initialize the display
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_ORANGE);
  tft.setTextColor(TFT_BLACK);

  // Battery indicator
  for (int xx = 1; xx < 7; xx++) {
    tft.fillRect(spx, spy, uw, uh, cell_colour);
    spx += (uw + us);
    switch (xx) {
      case 1: case 2: cell_colour = TFT_RED; break;
      case 3: case 4: cell_colour = TFT_YELLOW; break;
      case 5: default: cell_colour = TFT_GREEN; break;
    }
  }

  // Initialize MAX30105 sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30105 was not found. Please check wiring/power.");
    tft.drawString("MAX30105 not found", 10, 30, 2);
    while (1);
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");

  particleSensor.setup(); // Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x3F); // Set Red LED to maximum for better readings
  particleSensor.setPulseAmplitudeIR(0x3F);  // Set IR LED to maximum for better readings
  particleSensor.setPulseAmplitudeGreen(0);  // Turn off Green LED

  // Initialize BLE
  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");
    while (1);
  }

  // Set BLE device name and service
  BLE.setLocalName("ESP32_BLE");
  BLE.setAdvertisedService(bleService);

  // Add characteristic to service
  bleService.addCharacteristic(bleCharacteristic);

  // Add service
  BLE.addService(bleService);

  // Start advertising
  BLE.advertise();

  Serial.println("Waiting for a client connection to notify...");
}

void loop() {
  bufferLength = BUFFER_SIZE;
  for (int i = 0; i < bufferLength; i++) {
    while (particleSensor.available() == false) // Wait until new data is available
      particleSensor.check();
    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample(); // We need to advance to the next sample after retrieving the data
  }

  // Calculate SpO2 and heart rate
  maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2Value, &validSPO2, &heartRate, &validHeartRate);

  // Check if valid readings are obtained
  if (validHeartRate && heartRate > 0 && validSPO2 && spo2Value > 0) {
    beatsPerMinute = heartRate;
    beatAvg = beatsPerMinute; // Here we are using the instantaneous BPM; you can replace it with your averaging logic
    spo2 = spo2Value;

    // Ensure SpO2 is between 90% and 100% when BPM is not zero
    spo2 = constrain(spo2, 90.0, 100.0);
  } else {
    // If no valid readings, set average BPM to 0 and SpO2 to 0
    beatAvg = 0;
    spo2 = 0;
  }

  // Log data
  if (dataCount < 1440) {
    dailyData[dataCount] = { "07/08/2024 09:35", beatAvg, spo2 };
    dataCount++;
  }

  updateDisplay(beatsPerMinute, beatAvg, spo2); // Update display with new data

  BLEDevice central = BLE.central();

  // If a central is connected
  if (central) {
    Serial.print("Connected to central: ");
    Serial.println(central.address());

    while (central.connected()) {
      String dataString = "";
      for (int i = 0; i < dataCount; i++) {
        dataString += dailyData[i].timestamp + "," +
                      String(dailyData[i].bpm) + "," +
                      String(dailyData[i].spo2) + "\n";
      }

      bleCharacteristic.writeValue(dataString.c_str());

      delay(1000); // Send data every second
    }

    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
  }
}

void updateDisplay(float bpm, int avgBpm, float spo2) {
  // Calculate current time using millis
  unsigned long currentMillis = millis();
  unsigned long totalMinutes = currentMillis / millisInOneMinute;
  unsigned long currentHour = initialHour + (totalMinutes / 60);
  unsigned long currentMinute = initialMinute + (totalMinutes % 60);

  // Adjust for overflow in hours and minutes
  currentHour = currentHour % 24;
  currentMinute = currentMinute % 60;

  // Create time string
  char timeBuffer[10];
  snprintf(timeBuffer, sizeof(timeBuffer), "%02lu:%02lu", currentHour, currentMinute);

  // Check if the heart rate is very high and display a warning
  if (avgBpm >= 150) { // Trigger alert at BPM of 150
    // Show warning message for 10 seconds
    tft.fillScreen(TFT_RED);
    tft.setTextColor(TFT_WHITE, TFT_RED); // White text on red background
    tft.setTextSize(1); // Small enough to fit the message

    // Display the warning message with BPM value
    String message = "Heart Rate very high: " + String(avgBpm) + " BPM. Take a break and relax.";
    
    // Word wrap the message manually for proper display
    tft.drawString("Heart Rate very high:", 5, 30, 2);
    tft.drawString(String(avgBpm) + " BPM", 5, 50, 2);
    tft.drawString("Take a break and relax.", 5, 70, 2);

    delay(10000); // Display message for 10 seconds
    return; // Skip the rest of the updateDisplay logic
  }

  tft.fillScreen(TFT_ORANGE);

  // Display fixed date at the top with the same font size as the time
  tft.setTextColor(TFT_BLACK, TFT_ORANGE);
  tft.setTextSize(1); // Smaller font size to match the time
  tft.drawCentreString("7th August 2024", tft.width() / 2, 5, 2); // Centered at the top

  // Display dynamic time underneath the date
  tft.setTextColor(TFT_BLACK, TFT_ORANGE);
  tft.setTextSize(1); // Smaller font size
  tft.drawCentreString(timeBuffer, tft.width() / 2, 25, 2); // Centered beneath the date

  // Draw ECG line
  drawECGLine();

  // Draw heart shape for BPM
  drawHeartShape(68, 70, 30, TFT_RED); // Heart shape centered on screen

  // Display BPM value inside the heart shape
  tft.setTextColor(TFT_WHITE, TFT_RED); // Text color with heart shape background
  tft.setTextSize(2); // Bold and larger font for BPM
  tft.drawCentreString(String(avgBpm) + " BPM", 68, 65, 2); // BPM value centered

  stepsbar_len = ((steps_taken * 129) / step_goal);

  // Display steps
  tft.setTextColor(TFT_BLACK); // Ensure text is visible without a background
  tft.setTextSize(1); // Smaller font size
  tft.drawString("Steps: " + String(steps_taken), 2, 165, 2);
  tft.fillRect(2, 185, stepsbar_len, 10, TFT_GREEN); // Filled bar for steps
  tft.drawRect(2, 185, 129, 10, TFT_WHITE); // Outline for steps bar

  // Display SpO2
  tft.setTextSize(1); // Smaller font size
  tft.drawString("SpO2: " + String(spo2, 1) + "%", 2, 200, 2);

  // Debugging output to Serial Monitor
  Serial.print("BPM: ");
  Serial.print(bpm);
  Serial.print(", Avg BPM: ");
  Serial.print(avgBpm);
  Serial.print(", SpO2: ");
  Serial.println(spo2);
}

void drawECGLine() {
  // Define ECG line points for 8 peaks and 6 troughs, closer together
  int ecgPoints[17][2] = {
    {10, 120}, {20, 110}, {30, 130}, {40, 100}, {50, 140}, {60, 110}, {70, 130},
    {80, 90}, {90, 130}, {100, 110}, {110, 140}, {120, 100}, {130, 130}, {140, 110}, {150, 140}, {160, 120}, {170, 110}
  };

  // Draw ECG line using points with thicker lines for a bolder appearance
  for (int i = 0; i < 16; i++) {
    tft.drawLine(ecgPoints[i][0], ecgPoints[i][1], ecgPoints[i + 1][0], ecgPoints[i + 1][1], TFT_RED);
    tft.drawLine(ecgPoints[i][0], ecgPoints[i][1] + 1, ecgPoints[i + 1][0], ecgPoints[i + 1][1] + 1, TFT_RED);
  }
}

void drawHeartShape(int x, int y, int size, uint16_t color) {
  // Draw a simple heart shape
  int heartSize = size / 2;
  tft.fillCircle(x - heartSize, y - heartSize, heartSize, color);
  tft.fillCircle(x + heartSize, y - heartSize, heartSize, color);
  tft.fillTriangle(x - size, y - heartSize, x + size, y - heartSize, x, y + size, color);
}
