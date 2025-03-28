#include <MycilaJSY.h>

#include <esp_task_wdt.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <time.h>
#include <AsyncUDP.h>
#include <WebSerial.h>
#include <IPAddress.h>

#include "index_html.h"

// Wifi
const char* ssid = "XXXX"; 
const char* password = "YYYY";

// jsy
#define JSY_SERIAL Serial2
#define JSY_RX_PIN 16
#define JSY_TX_PIN 17

// Led
#define PIN_LED 2

// UDP port
#define UDP_PORT_TANK 8080

#define TIME_UPDATE_SECONDS 2
#define WATCHDOG_TIMEOUT_MS 10 * 1000

#define PERIODIC_UPDATE_SECONDS 60 * 60

#define POWER_CHECK_SIZE 60

//#define USE_WEB_SERIAL 1

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
// UDP server/client
AsyncUDP udp;

// jsy
Mycila::JSY jsy;

#ifdef USE_WEB_SERIAL
#define Serial WebSerial
#endif

static IPAddress water_tank_ip = INADDR_NONE;
static bool sendState = false;
static bool webState = false;
static float OK_percent = 0.0;
static int uptime = 0;
// Today + yesterday
typedef struct {
    float grid_energy;
    float grid_energy_returned;
    float solar_energy;
    float solar_energy_returned;
    float total_energy;
    float total_energy_returned;
} Energy;

static Energy yesterday_energy = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
static Energy today_energy = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
static bool is_today_energy_init = true;

void setup() {
  #ifndef USE_WEB_SERIAL
  Serial.begin(115200);
  #else
  Serial.begin(&server, "/webserial");
  #endif

  // Watchdog
  {
    esp_task_wdt_config_t config = {
      .timeout_ms = WATCHDOG_TIMEOUT_MS,
      .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,    // Bitmask of all cores
      .trigger_panic = false,
    };
    esp_task_wdt_reconfigure(&config);
    enableLoopWDT();
  }

  // JSY
  {
    jsy.begin(JSY_SERIAL, JSY_RX_PIN, JSY_TX_PIN, false);
    jsy.setBaudRate(jsy.getMaxAvailableBaudRate());
  }

  // Other pin
  
  pinMode(PIN_LED, OUTPUT);
  
  // Wifi
 
  WiFi.mode(WIFI_STA); 

  WiFi.onEvent([](WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info) {
    Serial.println("Connected to the WiFi network");
  }, ARDUINO_EVENT_WIFI_STA_CONNECTED);

  WiFi.onEvent([](WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info) {
    Serial.println("Got IP");
    Serial.println(WiFi.localIP());

    if(udp.listen(UDP_PORT_TANK)) {
      udp.onPacket([](AsyncUDPPacket packet) {
        Serial.println("Received data");
        water_tank_ip = packet.remoteIP();
        if (packet.length() == sizeof(bool)) {
           memcpy(&sendState, packet.data(), sizeof(bool));
           if (sendState) {
             Serial.println("Change to sending");
           }
           else {
             Serial.println("Change to no sending");
           }
        }
        else {
          Serial.println("Size error : " + packet.length());
        }
      });
    }
  }, ARDUINO_EVENT_WIFI_STA_GOT_IP);

  WiFi.onEvent([](WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info) {
    Serial.println("Disconnected from the WiFi");
    sendState = false;
    WiFi.begin(ssid, password);
  }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  // Web server
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Incoming HTML page request");
    request->send(200, "text/html", index_html);
  });

  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    if (request->hasParam("reset")) {
      String resetKind = request->getParam("reset")->value();
      Serial.println("Reset kind " + resetKind);
      yesterday_energy = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
      today_energy = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
      is_today_energy_init = true;
      if (resetKind == "hard") {
        Serial.println("Reset power meter");
        jsy.resetEnergy();
      }      
      request->send(200, "text/plain", "OK");
    }
    else {
      Serial.println("Unkown request (ignored)");
      request->send(200, "text/plain", "KO");
    }

  });

  server.on("/values", HTTP_GET, [] (AsyncWebServerRequest *request) {
    webState = true;

    String s = String("");
    // Grid
    s += jsy.data.channel1().activePower;
    s += ",";
    s += jsy.data.channel1().powerFactor;
    s += ",";
    s += jsy.data.channel1().activeEnergyImported;
    s += ",";
    s += jsy.data.channel1().activeEnergyReturned;
    s += ",";
     // Solar
    s += jsy.data.channel2().activePower;
    s += ",";
    s += jsy.data.channel2().powerFactor;
    s += ",";
    s += jsy.data.channel2().activeEnergyImported;
    s += ",";
    s += jsy.data.channel2().activeEnergyReturned;
    s += ",";
    // General
    s += (jsy.data.aggregate.activePower);
    s += ",";
    s += (jsy.data.aggregate.activeEnergyImported);
    s += ",";
    s += (jsy.data.aggregate.activeEnergyReturned);
    s += ",";
    // Raw
    s += (sendState ? "1" : "0");
    s += ",";
    s += OK_percent;
    s += ",";
    s += uptime;
    s += ",";
    // Today
    s += (jsy.data.channel1().activeEnergyImported - today_energy.grid_energy);
    s += ",";
    s += (jsy.data.channel1().activeEnergyReturned - today_energy.grid_energy_returned);
    s += ",";
    s += (jsy.data.channel2().activeEnergyImported - today_energy.solar_energy);
    s += ",";
    s += (jsy.data.channel2().activeEnergyReturned - today_energy.solar_energy_returned);
    s += ",";
    s += (jsy.data.aggregate.activeEnergyImported - today_energy.total_energy);
    s += ",";
    s += (jsy.data.aggregate.activeEnergyReturned - today_energy.total_energy_returned);
    s += ",";
    // Yesterday
    s += (today_energy.grid_energy - yesterday_energy.grid_energy);
    s += ",";
    s += (today_energy.grid_energy_returned - yesterday_energy.grid_energy_returned);
    s += ",";
    s += (today_energy.solar_energy - yesterday_energy.solar_energy);
    s += ",";
    s += (today_energy.solar_energy_returned - yesterday_energy.solar_energy_returned);
    s += ",";
    s += (today_energy.total_energy - yesterday_energy.total_energy);
    s += ",";
    s += (today_energy.total_energy_returned - yesterday_energy.total_energy_returned);
    s += ",";
    request->send(200, "text/plain", s);
  });

  WiFi.begin(ssid, password);

  server.begin();
}

void store_today_energy() {
  today_energy.grid_energy = jsy.data.channel1().activeEnergyImported;
  today_energy.grid_energy_returned = jsy.data.channel1().activeEnergyReturned;
  today_energy.solar_energy = jsy.data.channel2().activeEnergyImported;
  today_energy.solar_energy_returned = jsy.data.channel2().activeEnergyReturned;
  today_energy.total_energy = jsy.data.aggregate.activeEnergyImported;
  today_energy.total_energy_returned = jsy.data.aggregate.activeEnergyReturned;
}


void loop() {
  
  static int periodicCounter = 0;
  static int dayCounter = 0;

  if (sendState || webState || periodicCounter >= PERIODIC_UPDATE_SECONDS / TIME_UPDATE_SECONDS){

    unsigned long exec_delay = millis();
    bool isPowerOk = false;

    {
      static int ledStatus = LOW;
      ledStatus = !ledStatus;
      digitalWrite(PIN_LED, ledStatus);
    }
    
    // Jsy
    {
      bool isValidValue = jsy.read();
      
      // Watchdog for jsy

      static bool power_check[POWER_CHECK_SIZE];
      static int power_check_index = 0;
      static bool power_check_init = true;
      
      if (power_check_init) {
        power_check_init = false;
        for (int i = 0; i < POWER_CHECK_SIZE; i++) {
          power_check[i] = true;
        }
      }

      power_check_index++;
      if (power_check_index >= POWER_CHECK_SIZE) {
        power_check_index = 0;
      }
      isPowerOk = isValidValue && !std::isnan(jsy.data.aggregate.frequency) && jsy.data.aggregate.frequency > 0.0;
      power_check[power_check_index] = isPowerOk;

      bool is_all_KO = true;
      OK_percent = 0.0;
      for (int i = 0; i < POWER_CHECK_SIZE; i++) {
        if (power_check[i]) {
          is_all_KO = false;
          OK_percent += 1.0;
        }
      }
      OK_percent *= 100.0;
      OK_percent /= POWER_CHECK_SIZE;
      if (is_all_KO) {
        // JSY is dead, restart all...
        esp_restart();
      }
    }

    // Sending on the network
    if (sendState && water_tank_ip != INADDR_NONE) {
      AsyncUDPMessage msg;
      int32_t pow = (int) jsy.data.channel1().activePower;
      uint8_t buf[sizeof(pow)];
      memcpy(buf, &pow, sizeof(pow));
      msg.write(buf, sizeof(pow));
      udp.sendTo(msg, water_tank_ip, UDP_PORT_TANK);
    }

    // Night
    {
      static bool isNight = false;
      static unsigned long lastNightDetectionMs = 0;
      if (isPowerOk) {
        if (is_today_energy_init) {
          is_today_energy_init = false;
          store_today_energy();
          // No energy yesterday, so = to today value
          yesterday_energy = today_energy;
        }
        if (jsy.data.channel2().activePower > 0.0) {
          // Power = It's day
          isNight = false;
        }
        else {
          // No power = It's night
          if (!isNight && (uptime == 0 || dayCounter > (24 - 1) * 60 * 60 / TIME_UPDATE_SECONDS)) {
            isNight = true;
            uptime++;
            dayCounter = 0;
            yesterday_energy = today_energy;
            // Store last day values
            store_today_energy();
          }
        }
      }
    }
    
    exec_delay = millis() - exec_delay;
    if (exec_delay > TIME_UPDATE_SECONDS * 1000) {
      exec_delay = TIME_UPDATE_SECONDS * 1000;
    }

    webState = false;
    periodicCounter = 0;
    dayCounter++;
    delay(TIME_UPDATE_SECONDS * 1000 - exec_delay);

  }
  else {
    periodicCounter++;
    dayCounter++;
    webState = false;
    delay(TIME_UPDATE_SECONDS * 1000);
  }
}
