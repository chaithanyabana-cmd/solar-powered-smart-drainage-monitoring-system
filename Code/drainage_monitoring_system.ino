// ---------------- LIBRARIES ----------------
#include <WiFi.h>
#include <FirebaseESP32.h>

// ---------------- WIFI DETAILS ----------------
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
// ---------------- FIREBASE DETAILS ----------------
#define FIREBASE_HOST "locationbased-cb8f9-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "YOUR_FIREBASE_AUTH"
// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ---------------- PIN DEFINITIONS ----------------
const int trigPin = 12;
const int echoPin = 14;
const int turbidityPin = 34;
const int rainPin = 35;
const int ledPin = 2;

// ---------------- THRESHOLDS ----------------
const int HIGH_WATER_LEVEL = 15;
const int CRITICAL_WATER_LEVEL = 18;

// ---------------- SETUP ----------------
void setup() {

  Serial.begin(115200);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(turbidityPin, INPUT);
  pinMode(rainPin, INPUT);
  pinMode(ledPin, OUTPUT);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  // ---- WiFi Connect ----
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("\nWiFi Connected!");

  // ---- Firebase Config ----
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("SMART FLOOD MONITORING SYSTEM STARTED");
}

// ---------------- LOOP ----------------
void loop() {

  digitalWrite(ledPin, !digitalRead(ledPin));

  // WiFi auto reconnect
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Reconnecting WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    delay(3000);
    return;
  }

  int waterLevel = readWaterLevel();
  int turbidity = readTurbidity();
  int rain = readRain();

  String status = analyzeStatus(waterLevel, turbidity, rain);

  displayStatus(waterLevel, turbidity, rain, status);

  sendToFirebase(waterLevel, turbidity, rain, status);

  delay(5000);   // ✅ send data every 5 seconds
}

// ---------------- WATER LEVEL ----------------
int readWaterLevel() {

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);
  if (duration == 0) return -1;

  int distance = duration * 0.034 / 2;
  int waterHeight = 20 - distance;

  return constrain(waterHeight, 0, 20);
}

// ---------------- TURBIDITY ----------------
int readTurbidity() {

  long total = 0;
  for (int i = 0; i < 20; i++) {
    total += analogRead(turbidityPin);
    delay(5);
  }
  return total / 20;
}

// ---------------- RAIN SENSOR ----------------
int readRain() {

  long total = 0;
  for (int i = 0; i < 20; i++) {
    total += analogRead(rainPin);
    delay(5);
  }
  return total / 20;
}

// ---------------- ANALYSIS LOGIC ----------------
String analyzeStatus(int waterLevel, int turbidity, int rain) {

  if (waterLevel == -1)
    return "ULTRASONIC_ERROR";

  if (waterLevel >= CRITICAL_WATER_LEVEL && rain < 2000)
    return "FLOOD_WARNING";

  else if (waterLevel >= HIGH_WATER_LEVEL)
    return "HIGH_WATER_LEVEL";

  else if (turbidity < 500)
    return "SENSOR_BLOCKED";

  else if (turbidity < 1500)
    return "VERY_MUDDY";

  else if (turbidity < 2500)
    return "MUDDY";

  else if (turbidity < 3000)
    return "SLIGHTLY_MUDDY";

  else if (rain < 1500)
    return "HEAVY_RAIN";

  else if (rain < 2500)
    return "LIGHT_RAIN";

  else
    return "NORMAL";
}

// ---------------- FIREBASE SEND ----------------
void sendToFirebase(int waterLevel, int turbidity, int rain, String status) {

  String rainStatus;

  if (rain < 1500)
    rainStatus = "HIGH";
  else if (rain < 2500)
    rainStatus = "MEDIUM";
  else
    rainStatus = "LOW";

  bool blockage = (turbidity < 500);

  FirebaseJson json;

  json.set("water_level", waterLevel);
  json.set("blockage", blockage);
  json.set("rainfall", rainStatus);
  json.set("solar_voltage", 3.26);

  if (Firebase.setJSON(fbdo, "/drainage_monitor/gokaraju_college", json)) {
    Serial.println("Data sent to Firebase ✅");
  } else {
    Serial.print("Firebase Error: ");
    Serial.println(fbdo.errorReason());
  }
}

// ---------------- DISPLAY ----------------
void displayStatus(int waterLevel, int turbidity, int rain, String status) {

  Serial.println("--------------------------------");

  Serial.print("Water Level: ");
  Serial.println(waterLevel);

  Serial.print("Turbidity: ");
  Serial.println(turbidity);

  Serial.print("Rain Intensity: ");
  Serial.println(rain);

  Serial.print("System Status: ");
  Serial.println(status);

  Serial.println("--------------------------------");
}