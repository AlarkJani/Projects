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

// GPS module connection pins
const int gpsRxPin = 16;  // ESP32 Rx pin, connected to GPS Tx
const int gpsTxPin = 17;  // ESP32 Tx pin, connected to GPS Rx

long duration;
int distance;
WidgetLED green(V1);
WidgetLED orange(V2);
WidgetLED red(V3); 

const String binID = "bin1";

String latitude = "08030.51780 W";
String longitude = "4330.27545 N";

HTTPClient http;

void setup() {
  Serial.begin(9600);
  pinMode(34, INPUT);
  GPSModule.begin(9600, SERIAL_8N1, gpsRxPin, gpsTxPin);  // Begin GPS Module communication
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

void ultrasonic() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;  // Formula to calculate distance
  Serial.print("Distance: ");
  Serial.println(distance);
  delay(500);
}

void loop() {
  BlynkEdgent.run();
  ultrasonic();

  if(distance > 25)
  {
    green.on();
    orange.off();
    red.off();
    Blynk.virtualWrite(V1, 0);
     
  }
  
  else if(distance < 25 && distance > 5)
  {
    green.off();
    orange.on();
    red.off();
     Blynk.logEvent("dustbin_half");
    Blynk.virtualWrite(V2, 1);

    
    
  }
  
  else if(distance < 5 )
  {
    green.off();
    orange.off();
    red.on();
   Blynk.logEvent("dustbin_full");
   Blynk.virtualWrite(V3, 1); //Triggers URl, Sends msg 
       //Send a request every 10 seconds
  }
  else
  {
    green.on();
    orange.off();
    red.off();
    Blynk.virtualWrite(V4, 0);
    
  } 

  readGPS();
  delay(10000); // here change the delay to 60000 
}

void readGPS() {
  if (GPSModule.available()) {
    String gpsData = GPSModule.readStringUntil('\n');  // Read a line of GPS data
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
  // Split the sentence into parts
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

//{"bin_id":"bin1","distance":666,"longitude":08030.51780 W,"latitude":4330.27545 N,"altitude":0,"date":"0","time":"0"}

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
