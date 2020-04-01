/* ------------------------------------------------- */
/*
 
*/
/* ------------------------------------------------- */
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
#include "WiFiManager.h"    // https://github.com/tzapu/WiFiManager
#include "ESPTelnet.h"      // https://github.com/LennartHennigs/ESPTelnet

// #include <NTPClient.h>
// #include <Timezone.h>
/* ------------------------------------------------- */

#define SERIAL_SPEED    9600
#define DEVICE_NAME     "wemos01"

#define AP_TIMEOUT      10 * 60 // seconds
#define AP_NAME         "MY CAPTIVE PORTAL"
#define AP_PASSWORD     ""

#define OTA_NAME        DEVICE_NAME
#define OTA_PASSWORD    "pwd"
#define OTA_TIMEOUT     10 * 60 // seconds

/* ------------------------------------------------- */
template<class T> inline Print &operator <<(Print &obj, T arg) { obj.print(arg); return obj; } 
/* ------------------------------------------------- */

WiFiManager wifiManager;
ESPTelnet telnet;

IPAddress ip_addr;
String wifi;

/* ------------------------------------------------- */
void connectToWifi() {
  wifiManager.setDebugOutput(false);
  wifiManager.setConfigPortalTimeout(AP_TIMEOUT);
  wifiManager.setAPCallback([] (WiFiManager *myWiFiManager) {
    Serial << "[WiFi]\tNo known WiFi found" << "\n";
    Serial << "[WiFi]\tStarting AP: " << myWiFiManager->getConfigPortalSSID() << "\n";
    Serial << "[WiFi]\t" << WiFi.softAPIP() << "\n";
  });
  // enable autoconnect
  if (!(AP_PASSWORD == "" ? 
    wifiManager.autoConnect(AP_NAME) : 
    wifiManager.autoConnect(AP_NAME, AP_PASSWORD))
   ) {
    Serial << "[WiFi] Failed to connect and hit timeout"  << "\n";
    ESP.restart();
    delay(1000); 
  // known wifi found & connected
  }  else {
    ip_addr = WiFi.localIP();

    Serial << "[WiFi]\t" << WiFi.SSID() << "\n";
    Serial << "[MAC]\t" << WiFi.macAddress() << "\n";
    Serial << "[IP]\t" << ip_addr.toString() << "\n";
  }
}
/* ------------------------------------------------- */
void setupSerial() {
  Serial.begin(SERIAL_SPEED);
  while (!Serial) {
    delay(10);
  }
  Serial << "\n\n";
}
/* ------------------------------------------------- */
void setupTelnet() {
if (telnet.begin()) {
      telnet.onConnect([]() {
        Serial << "[telnet]\t" << telnet.getIP() << " connected" << "\n";

        telnet << "\n" << "Welcome" << telnet.getIP() << "\n";
        telnet << "(Use ^] + q  to disconnect.)" << "\n";
      });
      telnet.onConnectionAttempt([]() {
        Serial << "[telnet]\t" << telnet.getLastAttemptIP() << " tried to connect" << "\n";
      });
      telnet.onReconnect([]() {
        Serial << "[telnet]\t" << telnet.getIP() << " reconnected" << "\n";
      });
      telnet.onDisconnect([]() {
        Serial << "[telnet]\t" << telnet.getIP() << " disconnected" << "\n";
      });
        Serial << "[telnet] Started" << "\n";
//        Serial << "[telnet] Connect via: telnet " << ip_addr.toString() << "\n";
    } else {
      Serial << "[telnet] Error creating server" << "\n";
      delay(10000);
      ESP.reset();
      delay(1000); 
    }
}
/* ------------------------------------------------- */
void setupOTA() {
    // setup internal LED to notify upload
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    // setup OTA
    ArduinoOTA.onStart([](){
      digitalWrite(LED_BUILTIN, LOW);
    });
    ArduinoOTA.onEnd([]() {
      digitalWrite(LED_BUILTIN, HIGH);
      ESP.restart();
      delay(1000);
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      int p = (progress / (total / 100));
      digitalWrite(LED_BUILTIN, (p % 2 == 1) ? HIGH : LOW); 
      // Serial.printf("Progress: %u%%\r", p);
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.setHostname(OTA_NAME);
    if (OTA_PASSWORD != "") {
      ArduinoOTA.setPassword(OTA_PASSWORD);
    }
    ArduinoOTA.begin();    
    Serial << "[OTA]\t" << "Started" << "\n";
}
/* ------------------------------------------------- */
void setupMDNS() {
  if (MDNS.begin(DEVICE_NAME)) {
    Serial << "[mDNS]\t" << DEVICE_NAME << ".local" << "\n";
  } else {
    Serial << "[mDNS]\t" << "Error creating server" << "\n";

  }
}

/* ------------------------------------------------- */

void setup() {
  setupSerial();  
//   wifiManager.resetSettings();  // this will delete all credentials
  connectToWifi();
  setupMDNS();    // must come before OTA
  setupTelnet();
  setupOTA();
}
/* ------------------------------------------------- */
int counter = 0;

void loop() {
  ArduinoOTA.handle();
  telnet.loop();

  telnet.print("." + (counter++ % 10 == 0) ? "\n" : "");
  delay(1000);
}
//* ------------------------------------------------- */
