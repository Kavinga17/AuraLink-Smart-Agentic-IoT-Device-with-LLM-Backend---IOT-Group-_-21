#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "time.h"

// =====================
// WiFi Configuration
// =====================
const char* ssid = "STARLINK-5G-100Hz";
const char* password = "star@123";
const char* mqtt_server = "broker.hivemq.com";

// =====================
// DHT22 Sensor Setup
// =====================
#define KP_DHTPIN 4
#define KP_DHTTYPE DHT22
DHT kp_dht(KP_DHTPIN, KP_DHTTYPE);

// =====================
// LED Setup
// =====================
#define KP_LED_RED 19
#define KP_LED_GREEN 18
#define KP_LED_BLUE 5   // Blue LED for email notification

// =====================
// LCD Setup (20x4)
// =====================
LiquidCrystal_I2C lcd(0x27, 20, 4);

// =====================
// MQTT + WiFi Client
// =====================
WiFiClient espClient;
PubSubClient client(espClient);

// =====================
// NTP Setup
// =====================
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800; // GMT+5:30
const int daylightOffset_sec = 0;

// =====================
// Email Display Variables
// =====================
String emailSummary = "";
unsigned long emailStartMillis = 0;
const unsigned long emailDisplayDuration = 5 * 60 * 1000UL; // 5 minutes
bool newEmailAvailable = false;

// =====================
// WiFi Setup
// =====================
void setup_wifi() {
  WiFi.begin(ssid, password);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi...");
  while (WiFi.status() != WL_CONNECTED) delay(500);
  lcd.clear();
}

// =====================
// MQTT Reconnect
// =====================
void reconnect() {
  while (!client.connected()) {
    if (client.connect("kp_ESP32ClientDHT22")) {
      client.subscribe("kp_home/email_summary");
    } else delay(5000);
  }
}

// =====================
// Setup
// =====================
void setup() {
  Serial.begin(115200);
  Wire.begin();
  kp_dht.begin();

  lcd.init();
  lcd.backlight();

  pinMode(KP_LED_RED, OUTPUT);
  pinMode(KP_LED_GREEN, OUTPUT);
  pinMode(KP_LED_BLUE, OUTPUT);
  digitalWrite(KP_LED_RED, LOW);
  digitalWrite(KP_LED_GREEN, HIGH);
  digitalWrite(KP_LED_BLUE, LOW);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqttCallback);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

// =====================
// MQTT Callback
// =====================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  if (String(topic) == "kp_home/email_summary") {
    emailSummary = "";
    for (unsigned int i = 0; i < length; i++) {
      emailSummary += (char)payload[i];
    }
    emailStartMillis = millis();
    newEmailAvailable = true;
    digitalWrite(KP_LED_BLUE, HIGH); // Turn on blue LED for new mail
  }
}

// =====================
// Get time in 12h + AM/PM
// =====================
String getTimeString() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return "No Time";

  int hour = timeinfo.tm_hour;
  String ampm = "AM";
  if (hour >= 12) {
    ampm = "PM";
    if (hour > 12) hour -= 12;
  }
  if (hour == 0) hour = 12;

  char buffer[6];
  sprintf(buffer, "%02d:%02d", hour, timeinfo.tm_min);
  return String(buffer) + " " + ampm;
}

// =====================
// Get date with day name
// =====================
String getDateString() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return "No Date";

  const char* days[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
  const char* months[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

  char buffer[20];
  sprintf(buffer, "%s %02d-%s-%04d", days[timeinfo.tm_wday], timeinfo.tm_mday, months[timeinfo.tm_mon], 1900 + timeinfo.tm_year);
  return String(buffer);
}

// =====================
// Helper: Center text on LCD
// =====================
void lcdCenterPrint(int row, String text) {
  int len = text.length();
  int start = (20 - len) / 2;
  if (start < 0) start = 0;
  lcd.setCursor(start, row);
  lcd.print(text);
}

// =====================
// Helper: Vertical Scroll Email Summary
// =====================
void verticalScrollEmail(String text, int delayTime = 600) {
  // Split text into chunks of max 20 characters per line
  int totalLines = (text.length() + 19) / 20;
  String lines[totalLines];

  for (int i = 0; i < totalLines; i++) {
    lines[i] = text.substring(i * 20, (i + 1) * 20);
  }

  // Show in vertical scroll fashion
  for (int i = 0; i < totalLines; i++) {
    lcd.clear();
    lcdCenterPrint(0, getTimeString());
    // show last 3 lines progressively
    if (i >= 0) lcd.setCursor(0, 1), lcd.print((i - 2 >= 0) ? lines[i - 2] : "                    ");
    if (i >= 1) lcd.setCursor(0, 2), lcd.print((i - 1 >= 0) ? lines[i - 1] : "                    ");
    if (i >= 2) lcd.setCursor(0, 3), lcd.print(lines[i]);
    delay(delayTime);
  }

  // After full scroll, pause a bit at the end
  delay(2000);
}

// =====================
// Main Loop
// =====================
void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  float h = kp_dht.readHumidity();
  float t = kp_dht.readTemperature();
  int rssi = WiFi.RSSI();
  if (isnan(h) || isnan(t)) return;

  bool highTemp = t > 30.0;
  digitalWrite(KP_LED_RED, highTemp ? HIGH : LOW);
  digitalWrite(KP_LED_GREEN, highTemp ? LOW : HIGH);

  // Publish sensor data
  char tempBuf[8], humBuf[8], rssiBuf[8];
  dtostrf(t, 1, 2, tempBuf);
  dtostrf(h, 1, 2, humBuf);
  dtostrf(rssi, 1, 0, rssiBuf);
  client.publish("kp_home/dht11/temperature", tempBuf);
  client.publish("kp_home/dht11/humidity", humBuf);
  client.publish("kp_home/dht11/wifi_rssi", rssiBuf);

  unsigned long currentMillis = millis();

  // ========== EMAIL DISPLAY MODE ==========
  if (newEmailAvailable && currentMillis - emailStartMillis < emailDisplayDuration) {
    // Vertical scroll for long messages
    lcd.clear();
    lcdCenterPrint(0, getTimeString());

    if (emailSummary.length() > 60)
      verticalScrollEmail(emailSummary);
    else {
      // short email → static
      lcd.setCursor(0, 1);
      lcd.print(emailSummary.substring(0, 20));
      lcd.setCursor(0, 2);
      lcd.print(emailSummary.length() > 20 ? emailSummary.substring(20, 40) : "                    ");
      lcd.setCursor(0, 3);
      lcd.print(emailSummary.length() > 40 ? emailSummary.substring(40, 60) : "                    ");
      delay(5000);
    }

    // After scroll/static show centered normal data for 5s
    lcd.clear();
    lcdCenterPrint(0, getTimeString());
    lcdCenterPrint(1, getDateString());
    lcdCenterPrint(2, "Temp: " + String(t, 1) + (char)223 + "C");
    lcdCenterPrint(3, "Hum: " + String(h, 1) + "%");
    delay(5000);

  } else {
    // ========== NORMAL DISPLAY ==========
    lcd.clear();
    lcdCenterPrint(0, getTimeString());
    lcdCenterPrint(1, getDateString());
    lcdCenterPrint(2, "Temp: " + String(t, 1) + (char)223 + "C");
    lcdCenterPrint(3, "Hum: " + String(h, 1) + "%");
    delay(1000);

    // Turn off blue LED after 5 minutes
    digitalWrite(KP_LED_BLUE, LOW);
    newEmailAvailable = false;
  }
}
