// Libraries
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <WEMOS_SHT3X.h>

// Root certificate
const char IRG_Root_X1 [] PROGMEM = R"CERT(
-----BEGIN CERTIFICATE-----
---------------your certificate---------------
---------------your certificate---------------
---------------your certificate---------------
---------------your certificate---------------
---------------your certificate---------------
-----END CERTIFICATE-----
)CERT";

// Sensor ID
#define ROOM_ID         "1"         //Define the room ID (number OR string "outside" to define outside sensor)
#define APP_KEY         "app_key"   //Our key from web application

#define WEB_URL         "https://domain.com"  //URL include protocol without ending slash

// WiFi setup
#define WIFI_SSID       "SSID"      //your wifi name - SSID
#define WIFI_PASSWORD   "password"  //your WiFi password

// Sleep and loop setup
#define PUBLISH_RATE    10*60       // publishing rate in seconds (minute*60), def.: 10*60 - 10 minut
#define SLEEP_MODE      true        // deep sleep then reset or idling (D0 must be connected to RST)

// Debug true/false
#define DEBUG           false       // debug to serial port

// Variables to temperature and humidity
float t;
float h;

// Inicialization library
SHT3X sht30(0x45);

// Create a list of certificates with the server certificate
X509List cert(IRG_Root_X1);

void setup_wifi() {
  Serial.println();
  Serial.println("********** Temperature sensor **********");
  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("WiFi connected to: ");
  Serial.println(WIFI_SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());
  Serial.println();
  Serial.println();
}

void setup() {
  if (DEBUG) Serial.begin(115200);
  setup_wifi();

  // Set time via NTP, as required for x.509 validation
  configTime(2 * 3600, 0, "0.cz.pool.ntp.org", "1.cz.pool.ntp.org", "2.cz.pool.ntp.org");

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}

void loop() {
  WiFiClientSecure client;

  // wait for WiFi connection
  if ((WiFi.status() == WL_CONNECTED)) {

    // Reading SHT30
    sht30.get();
    float t = sht30.cTemp;
    float h = sht30.humidity;

    // IF DEBUG - Reading SHT30
    Serial.print("Temperature [°C]: ");
    Serial.print(t);
    Serial.print(" | Humidity [%]: ");
    Serial.println(h);

    client.setTrustAnchors(&cert);

    HTTPClient https;

    Serial.print("[HTTPS] begin...\n");
    if (https.begin(client, String(WEB_URL) + "/get-valve-value/" + String(APP_KEY) + "/" + String(ROOM_ID) + "/" + String(t) + "/" + String(h))) {  // HTTPS
      Serial.print(String(WEB_URL) + "/get-valve-value/" + String(APP_KEY) + "/" + String(ROOM_ID) + "/" + String(t) + "/" + String(h));
      int httpCode = https.GET();
      // httpCode will be negative on error
      if (httpCode == 200) {
        String payload = https.getString();
        Serial.println(payload);
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
      https.end();
      Serial.println();
    } else {
      setup_wifi();
    }
  }

  //DeepSleep mode
  if (SLEEP_MODE) {
    if (DEBUG) {
      Serial.print("Sleeping ");
      Serial.print(PUBLISH_RATE);
      Serial.println(" seconds...");
      Serial.println();
      Serial.println();
    }
    ESP.deepSleep(PUBLISH_RATE * 1e6); //translate seconds to us
  } else {
    if (DEBUG) {
      Serial.print("Wating ");
      Serial.print(PUBLISH_RATE);
      Serial.println(" seconds...");
      Serial.println();
      Serial.println();
    }
    delay(PUBLISH_RATE * 1e3); //translate seconds to ms
  }
}