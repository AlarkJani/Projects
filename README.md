
# Wastech
Wastech is a smart waste management system designed to monitor and report the fill levels of waste bins in real-time. 
It utilizes an ESP32 microcontroller, ultrasonic sensor, GPS module, and Blynk IoT platform to provide a comprehensive solution for efficient waste collection.

## Table of Contents
- [Overview](#overview)
- [Features](#features)
- [Hardware Requirements](#hardware-requirements)
- [Software Requirements](#software-requirements)
- [Circuit Diagram](#circuit-diagram)
- [Setup and Installation](#setup-and-installation)
- [Code Explanation](#code-explanation)
- [Usage](#usage)
- [Contributing](#contributing)
- [License](#license)

## Overview
Wastech aims to optimize waste collection by providing real-time data on the fill levels of waste bins.
This helps in timely and efficient waste collection, reducing overflow and underutilization of resources.
The system measures the fill level using an ultrasonic sensor, tracks the bin's location with a GPS module, and sends data to a server for monitoring.

## Features
- Real-time monitoring of waste bin fill levels
- GPS tracking of waste bin locations
- Alerts and notifications through Blynk
- HTTP data transmission to a remote server
- LED indicators for visual status

## Hardware Requirements
- ESP32 microcontroller
- Ultrasonic sensor (HC-SR04)
- GPS module (e.g., GPS Click)
- LEDs (Green, Orange, Red) for status indication
- Breadboard and jumper wires

## Software Requirements
- Arduino IDE
- Blynk library
- WiFi.h library
- HTTPClient.h library

## Code Explanation
### Libraries and Definitions
The code includes necessary libraries and defines pin assignments, WiFi credentials, and Blynk parameters:
```cpp
#include <HardwareSerial.h>
#include <HTTPClient.h>
#include <WiFi.h>
#define BLYNK_TEMPLATE_ID "TMPL2Kn4joK9_"
#define BLYNK_TEMPLATE_NAME "Blynk"
#define BLYNK_FIRMWARE_VERSION "0.1.0"
#define BLYNK_PRINT Serial
#include "BlynkEdgent.h"
HardwareSerial GPSModule(1);
#define echoPin 33
#define trigPin 32
const char* ssid = "OnePlus Nord";
const char* password = "12345678";
const int gpsRxPin = 16;
const int gpsTxPin = 17;
long duration;
int distance;
WidgetLED green(V1);
WidgetLED orange(V2);
WidgetLED red(V3); 
const String binID = "bin1";
String latitude = "08030.51780 W";
String longitude = "4330.27545 N";
HTTPClient http;
```

### Setup Function
Initializes serial communication, pin modes, GPS module, and WiFi connection. Begins Blynk service:
```cpp
void setup() {
  Serial.begin(9600);
  pinMode(34, INPUT);
  GPSModule.begin(9600, SERIAL_8N1, gpsRxPin, gpsTxPin);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  BlynkEdgent.begin();
  delay(2000);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}
```

### Ultrasonic Sensor Function
Measures the distance to the waste level and controls LED indicators based on the distance:
```cpp
void ultrasonic() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;
  Serial.print("Distance: ");
  Serial.println(distance);
  delay(500);
}
```

### Loop Function
Runs Blynk and sensor measurement continuously. Calls `readGPS()` to get location data and `sendDataToServer()` to send data to the server:
```cpp
void loop() {
  BlynkEdgent.run();
  ultrasonic();
  if(distance > 25) {
    green.on();
    orange.off();
    red.off();
    Blynk.virtualWrite(V1, 0);
  } else if(distance < 25 && distance > 5) {
    green.off();
    orange.on();
    red.off();
    Blynk.logEvent("dustbin_half");
    Blynk.virtualWrite(V2, 1);
  } else if(distance < 5 ) {
    green.off();
    orange.off();
    red.on();
    Blynk.logEvent("dustbin_full");
    Blynk.virtualWrite(V3, 1);
  } else {
    green.on();
    orange.off();
    red.off();
    Blynk.virtualWrite(V4, 0);
  } 
  readGPS();
  delay(10000);
}
```

### GPS Data Handling
Parses GPS data and formats it for transmission:
```cpp
void readGPS() {
  if (GPSModule.available()) {
    String gpsData = GPSModule.readStringUntil('\n');
    if (gpsData.startsWith("$GPGGA,")) {
      parseGPGGA(gpsData);
      Serial.print("GPS DATA: ");
      Serial.println(gpsData);
    }
    Serial.print("Latitude: ");
    Serial.println(latitude);
    Serial.print("Longitude: ");
    Serial.println(longitude);
    sendDataToServer(binID, distance, longitude, latitude, "100", "4/12/2024", "12:38");
  }
}

void parseGPGGA(String gpggaSentence) {
  int commaIndex = 0;
  String data[15];
  int dataIndex = 0;
  for (int i = 0; i < gpggaSentence.length(); i++) {
    if (gpggaSentence[i] == ',' || i == gpggaSentence.length() - 1) {
      data[dataIndex++] = gpggaSentence.substring(commaIndex, i);
      commaIndex = i + 1;
    }
  }
  if (dataIndex > 5) {
    latitude = data[2] + " " + data[3];
    longitude = data[4] + " " + data[5];
  }
}
```

### Data Transmission
Sends the sensor data along with GPS coordinates to the server using HTTP POST requests:
```cpp
void sendDataToServer(String binID, int distance, String longitude, String latitude, String altitude, String date, String time) {
 if (WiFi.status() == WL_CONNECTED) {
    http.begin("http://68.183.84.88/push_bin_data");
    http.addHeader("Content-Type", "application/json");
    String payload = String("{") +
                     "\"bin_id\":\"" + binID + "\"," +
                     "\"distance\":\"" + String(distance) + "\"," +
                     "\"longitude\":\"" + longitude + "\"," +
                     "\"latitude\":\"" + latitude + "\"," +
                     "\"altitude\":\"" + altitude + "\"," +
                     "\"date\":\"" + date + "\"," +
                     "\"time\":\"" + time + "\"" +
                     "}";
    Serial.println(payload);
    int httpResponseCode = http.POST(payload);
    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
 } else {
    Serial.println("WiFi Disconnected");
 }
}
```

## Usage
1. Run the system:
   - Power the ESP32 and ensure it connects to your WiFi network.
   - Monitor the Blynk dashboard for real-time updates on the bin status.

2. LED Indicators:
   - Green LED: Bin is mostly empty.
   - Orange LED: Bin is half full.
   - Red LED: Bin is full and needs attention.

3. **Server Communication:**
   - Data is sent to the configured server endpoint (`http://68.183.84.88/push_bin_data`).

## Contributing
Contributions are welcome! Please open an issue or submit a pull request
