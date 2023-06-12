// Libraries
#include <max6675.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>

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

// Global setting
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

// Pins for MAX6675
int pinSO  = 1;
int pinCS  = 2;
int pinSCK = 3;

// Instance for MAX6675
MAX6675 thermocouple(pinSCK, pinCS, pinSO);

// Create a list of certificates with the server certificate
X509List cert(IRG_Root_X1);

void setup_wifi() {
  Serial.println();
  Serial.println("********** Controller for water temperature **********");
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
  Serial.println();
  Serial.println();
}

void loop() {
  WiFiClientSecure client;

  // wait for WiFi connection
  if ((WiFi.status() == WL_CONNECTED)) {
    client.setTrustAnchors(&cert);
    HTTPClient https;

    float temperatureCelsius = thermocouple.readCelsius();
    Serial.println("Water temperature: " + String(temperatureCelsius) + "°C");
    Serial.println();

    if (https.begin(client, String(WEB_URL) + "/water-temp/" + String(APP_KEY) + "/" + String(temperatureCelsius))) {  // HTTPS
      Serial.println(String(WEB_URL) + "/water-temp/" + String(APP_KEY) + "/" + String(temperatureCelsius));

      Serial.print("[HTTPS] GET...\n");

      // start connection and send HTTP header
      int httpCode = https.GET();

      if (httpCode == 200) {
        String payload = https.getString();
        Serial.println("200 OK!");
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }

      https.end();
    }
  } else {
    setup_wifi();
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