#include <HardwareSerial.h>
#include <HTTPClient.h>
#include <WiFi.h>

#define BLYNK_TEMPLATE_ID "TMPL2Kn4joK9_"  // Blynk template ID
#define BLYNK_TEMPLATE_NAME "Blynk"  // Blynk template name

#define BLYNK_FIRMWARE_VERSION "0.1.0"  // Firmware version
#define BLYNK_PRINT Serial  // Serial monitor for Blynk
#include "BlynkEdgent.h"  // Blynk Edgent library

HardwareSerial GPSModule(1);  // Define a new hardware serial port for the GPS module

#define echoPin 33  // Pin connected to the ultrasonic sensor echo pin
#define trigPin 32  // Pin connected to the ultrasonic sensor trigger pin

const char* ssid = "OnePlus Nord";  // WiFi SSID
const char* password = "12345678";  // WiFi password

// GPS module connection pins
const int gpsRxPin = 16;  // ESP32 Rx pin, connected to GPS Tx
const int gpsTxPin = 17;  // ESP32 Tx pin, connected to GPS Rx

long duration;  // Variable to store the duration of ultrasonic sensor pulse
int distance;  // Variable to store the calculated distance from the ultrasonic sensor
WidgetLED green(V1);  // Blynk LED widget for green LED on virtual pin V1
WidgetLED orange(V2);  // Blynk LED widget for orange LED on virtual pin V2
WidgetLED red(V3);  // Blynk LED widget for red LED on virtual pin V3

const String binID = "bin1";  // Unique identifier for the garbage bin

String latitude = "08030.51780 W";  // Placeholder latitude
String longitude = "4330.27545 N";  // Placeholder longitude

HTTPClient http;  // Create an HTTPClient object for sending HTTP requests

void setup() {
  Serial.begin(9600);  // Initialize serial communication at 9600 baud rate
  pinMode(34, INPUT);  // Set pin 34 as input for an unspecified sensor
  GPSModule.begin(9600, SERIAL_8N1, gpsRxPin, gpsTxPin);  // Initialize GPS module communication
  pinMode(trigPin, OUTPUT);  // Set the ultrasonic sensor trigger pin as output
  pinMode(echoPin, INPUT);  // Set the ultrasonic sensor echo pin as input
  BlynkEdgent.begin();  // Initialize Blynk
  delay(2000);  // Wait for 2 seconds

  WiFi.begin(ssid, password);  // Connect to WiFi
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);  // Wait for 1 second
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");  // Print success message
}

void ultrasonic() {
  digitalWrite(trigPin, LOW);  // Ensure the trigger pin is low
  delayMicroseconds(2);  // Wait for 2 microseconds
  digitalWrite(trigPin, HIGH);  // Send a 10 microsecond pulse to the trigger pin
  delayMicroseconds(10);  // Wait for 10 microseconds
  digitalWrite(trigPin, LOW);  // Set the trigger pin low again
  duration = pulseIn(echoPin, HIGH);  // Measure the pulse duration from the echo pin
  distance = duration * 0.034 / 2;  // Calculate distance based on the speed of sound
  Serial.print("Distance: ");
  Serial.println(distance);  // Print the measured distance
  delay(500);  // Wait for 500 milliseconds
}

void loop() {
  BlynkEdgent.run();  // Run Blynk background tasks
  ultrasonic();  // Call the ultrasonic function to measure distance

  // Control the LEDs and send events based on the distance
  if (distance > 25) {
    green.on();  // Turn on green LED
    orange.off();  // Turn off orange LED
    red.off();  // Turn off red LED
    Blynk.virtualWrite(V1, 0);  // Write to Blynk virtual pin V1
  } else if (distance < 25 && distance > 5) {
    green.off();  // Turn off green LED
    orange.on();  // Turn on orange LED
    red.off();  // Turn off red LED
    Blynk.logEvent("dustbin_half");  // Log a half-full event to Blynk
    Blynk.virtualWrite(V2, 1);  // Write to Blynk virtual pin V2
  } else if (distance < 5) {
    green.off();  // Turn off green LED
    orange.off();  // Turn off orange LED
    red.on();  // Turn on red LED
    Blynk.logEvent("dustbin_full");  // Log a full event to Blynk
    Blynk.virtualWrite(V3, 1);  // Write to Blynk virtual pin V3
  } else {
    green.on();  // Turn on green LED
    orange.off();  // Turn off orange LED
    red.off();  // Turn off red LED
    Blynk.virtualWrite(V4, 0);  // Write to Blynk virtual pin V4
  }

  readGPS();  // Call the function to read GPS data
  delay(10000);  // Wait for 10 seconds (change this to 60000 for a 1-minute delay)
}

void readGPS() {
  if (GPSModule.available()) {
    String gpsData = GPSModule.readStringUntil('\n');  // Read a line of GPS data
    if (gpsData.startsWith("$GPGGA,")) {
      parseGPGGA(gpsData);  // Parse the GPGGA sentence
      Serial.print("GPS DATA: ");
      Serial.println(gpsData);  // Print the raw GPS data
    }
    Serial.print("Latitude: ");
    Serial.println(latitude);  // Print the parsed latitude
    Serial.print("Longitude: ");
    Serial.println(longitude);  // Print the parsed longitude

    // Send the collected data to the server
    sendDataToServer(binID, distance, longitude, latitude, "100", "4/12/2024", "12:38");
  }
}

void parseGPGGA(String gpggaSentence) {
  // Split the sentence into parts
  int commaIndex = 0;
  String data[15];  // Array to hold the split data
  int dataIndex = 0;

  for (int i = 0; i < gpggaSentence.length(); i++) {
    if (gpggaSentence[i] == ',' || i == gpggaSentence.length() - 1) {
      data[dataIndex++] = gpggaSentence.substring(commaIndex, i);  // Extract each part between commas
      commaIndex = i + 1;
    }
  }

  // Extract latitude and longitude from the parsed data
  if (dataIndex > 5) {
    latitude = data[2] + " " + data[3];  // Combine latitude value and direction
    longitude = data[4] + " " + data[5];  // Combine longitude value and direction
  }
}

// Function to send data to the server
void sendDataToServer(String binID, int distance, String longitude, String latitude, String altitude, String date, String time) {
  if (WiFi.status() == WL_CONNECTED) {  // Check if connected to WiFi
    http.begin("http://68.183.84.88/push_bin_data");  // Specify the server URL
    http.addHeader("Content-Type", "application/json");  // Specify the content type

    // Create JSON payload
    String payload = String("{") +
                     "\"bin_id\":\"" + binID + "\"," +
                     "\"distance\":\"" + String(distance) + "\"," +
                     "\"longitude\":\"" + longitude + "\"," +
                     "\"latitude\":\"" + latitude + "\"," +
                     "\"altitude\":\"" + altitude + "\"," +
                     "\"date\":\"" + date + "\"," +
                     "\"time\":\"" + time + "\"" +
                     "}";

    Serial.println(payload);  // Print the payload for debugging

    int httpResponseCode = http.POST(payload);  // Send the POST request

    // Check the response code
    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);  // Print the response code
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);  // Print the error code
    }
    http.end();  // End the HTTP connection
  } else {
    Serial.println("WiFi Disconnected");  // Print an error message if WiFi is disconnected
  }
}
