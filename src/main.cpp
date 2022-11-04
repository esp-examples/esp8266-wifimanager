#include <ESP8266WiFi.h> //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
#include "LittleFS.h"
#include <ESP_DoubleResetDetector.h>

String mqtt_server_val;
String mqtt_port_val;
String device_name_val;

WiFiManagerParameter config_head("<h2>Additional Config</h2>");
WiFiManagerParameter mqtt_server_head("<h3>MQTT Server</h3>");
WiFiManagerParameter mqtt_port_head("<h3>MQTT Port</h3>");
WiFiManagerParameter device_name_head("<h3>Device Name</h3>");

const char *config_path = "/config.data";

File file;

// Number of seconds after reset during which a
// subseqent reset will be considered a double reset.
#define DRD_TIMEOUT 10

// RTC Memory Address for the DoubleResetDetector to use
#define DRD_ADDRESS 0

DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);

void readConfig()
{
  file = LittleFS.open(config_path, "r");
  mqtt_server_val = file.readStringUntil('\n');
  mqtt_port_val = file.readStringUntil('\n');
  device_name_val = file.readStringUntil('\n');
  Serial.println("Config loaded!");
  file.close();
}

void writeConfig(const char *mqtt_server, const char *mqtt_port, const char *device_name)
{
  file = LittleFS.open(config_path, "w");
  file.write(mqtt_server);
  file.write("\n");
  file.write(mqtt_port);
  file.write("\n");
  file.write(device_name);
  Serial.println("Write successful!");
  file.close();
}

void writeDefaultConfig()
{
  writeConfig("localhost", "1883", "test_device");
}

void setup()
{
  Serial.begin(9600);
  // WiFiManager
  // Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  wifiManager.setDebugOutput(false);

  if (!LittleFS.begin())
  {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  file = LittleFS.open(config_path, "r");
  if (!file)
  {
    Serial.println("No config data found.");
    writeDefaultConfig();
  }
  file.close();
  if (drd.detectDoubleReset())
  {
    Serial.println("Double reset -> resetting to default config!");
    writeDefaultConfig();
    wifiManager.resetSettings();
  }

  readConfig();

  WiFiManagerParameter mqtt_server("server", "localhost", mqtt_server_val.c_str(), 40);
  WiFiManagerParameter mqtt_port("port", "1883", mqtt_port_val.c_str(), 5);
  WiFiManagerParameter device_name("device_name", "", device_name_val.c_str(), 30);

  wifiManager.setCustomHeadElement("<style>body {background-color: lightgrey;}</style>");

  wifiManager.addParameter(&mqtt_server_head);
  wifiManager.addParameter(&mqtt_server);
  wifiManager.addParameter(&mqtt_port_head);
  wifiManager.addParameter(&mqtt_port);
  wifiManager.addParameter(&device_name_head);
  wifiManager.addParameter(&device_name);

  wifiManager.autoConnect("AutoConnectAP");

  writeConfig(mqtt_server.getValue(), mqtt_port.getValue(), device_name.getValue());

  Serial.println("connected...yeey :)");
}

void loop()
{
  drd.loop();
}