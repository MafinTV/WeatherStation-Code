#include "DHT.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

#define DHTPIN 16
#define DHTTYPE DHT22
#define SOIL_MOIST_PIN 34
#define LEDPIN 17
#define NUMPIXELS 8
#define UPDATE_RATE 5

const char* ssid = "";
const char* password = "";
const char* serial_number = "MJ-WS-2g4H5i6J7k";
const int dry = 4095;
const int wet = 2733;

DHT dht(DHTPIN, DHTTYPE);
Adafruit_NeoPixel pixels(NUMPIXELS, LEDPIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);

  dht.begin();
  pixels.begin();

  initializePixels();
  connectToWiFi();
  checkIn();
}

void loop() {
  float temperature = dht.readTemperature();
  float soilMoisture = readSoilMoisture();
  float humidity = dht.readHumidity();

  //printSensorReadings(soilMoisture, humidity, temperature);

  updateValues(temperature, soilMoisture, humidity);
  delay(UPDATE_RATE * 60000);
}

void initializePixels() {
  pixels.clear();
  pixels.show();
}

void connectToWiFi() {
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    showConnectingAnimation();
    delay(100);
  }

  showConnectedAnimation();
}

void showConnectingAnimation() {
  static int position1 = 0;
  static int position2 = 1;

  pixels.clear();
  pixels.setBrightness(64);
  pixels.setPixelColor(position1, pixels.Color(0, 0, 255));
  pixels.setPixelColor(position2, pixels.Color(0, 0, 255));
  pixels.show();

  position1 = (position1 + 1) % NUMPIXELS;
  position2 = (position2 + 1) % NUMPIXELS;
}

void showConnectedAnimation() {
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 0, 255));
    pixels.setBrightness(1);
    pixels.show();
  }

  for (int brightness = 1; brightness <= 255; brightness++) {
    pixels.setBrightness(brightness);
    pixels.show();
    delay(3);
  }

  for (int brightness = 255; brightness >= 0; brightness--) {
    pixels.setBrightness(brightness);
    pixels.show();
    delay(3);
  }

  delay(1000);
}

float readSoilMoisture() {
  int soilMoist = analogRead(SOIL_MOIST_PIN);
  float percentage = map(soilMoist, wet, dry, 100.0, 0.0);

  if (percentage > 100.0) {
    return 100.0;
  } else {
    return percentage;
  }
}

void printSensorReadings(float soilMoisture, float humidity, float temperature) {
  Serial.print("Soil Moisture: ");
  Serial.print(soilMoisture);
  Serial.println("%");
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println("%");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" Grad");
}

void checkIn(float temperature, float soilMoisture, float humidity) {
  StaticJsonDocument<200> doc;
  doc["station_serial_number"] = String(serial_number);
  doc["station_ssid"] = String(ssid);

  String postData;
  serializeJson(doc, postData);

  HTTPClient http;
  http.begin("http://zentrale.ddns.net:3000/api/stations/checkIn");
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(postData);

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String response = http.getString();
    Serial.println(response);
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

void updateValues(float temperature, float soilMoisture, float humidity) {
  StaticJsonDocument<200> doc;
  doc["station_serial_number"] = serial_number;
  doc["station_temp_value"] = temperature;
  doc["station_soil_moist_value"] = soilMoisture;
  doc["station_air_moist_value"] = humidity;

  String postData;
  serializeJson(doc, postData);

  HTTPClient http;
  http.begin("http://zentrale.ddns.net:3000/api/stations/updateValues");
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(postData);

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String response = http.getString();
    Serial.println(response);
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}
