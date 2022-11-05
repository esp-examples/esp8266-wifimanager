#include <ESP8266WiFi.h> //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
#include "LittleFS.h"
#include <ESP_DoubleResetDetector.h>

// Number of seconds after reset during which a
// subseqent reset will be considered a double reset.
#define DRD_TIMEOUT 10

// RTC Memory Address for the DoubleResetDetector to use
#define DRD_ADDRESS 0

// We use the DoubleResetDetector so that we can reset all
// configurations that were made via the portal
DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);

// The config is stored in the root of our littlefs
const char *config_path = "/config.data";

String mqtt_server_val;
String mqtt_port_val;
String device_name_val;

File file;

/*
  This function reads the 3 config parameters from the file specified by config_path in the correct order.
*/
void readConfig()
{
  file = LittleFS.open(config_path, "r");
  mqtt_server_val = file.readStringUntil('\n');
  mqtt_port_val = file.readStringUntil('\n');
  device_name_val = file.readStringUntil('\n');
  Serial.println("Config loaded!");
  file.close();
}

/*
  This function writes the 3 config parameters to the file specified by config_path.
*/
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

/*
  This function writes the default config to the littlefs.
  It does this by wrapping writeConfig().
*/
void writeDefaultConfig()
{
  writeConfig("localhost", "1883", "test_device");
}

void setup()
{
  Serial.begin(9600);

  WiFiManager wifiManager;
  // The wifiManager can print many messages when running in debug mode
  wifiManager.setDebugOutput(false);

  if (!LittleFS.begin())
  {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  file = LittleFS.open(config_path, "r");
  if (!file)
  {
    // If no config is found the default config will be written to the fs.
    // This ensures, that there is always a config in place.
    Serial.println("No config data found.");
    writeDefaultConfig();
  }
  file.close();
  if (drd.detectDoubleReset())
  {
    // If a doubleReset was detexcted, we override our config with the
    // default an reset the wifiManager. This will remove information such
    // as SSID and password.
    Serial.println("Double reset -> resetting to default config!");
    writeDefaultConfig();
    wifiManager.resetSettings();
  }

  readConfig();

  // We add our parameters and a few html tags to the wifiManager.
  // This allows a user to enter these information in the portal.
  WiFiManagerParameter config_head("<h2>Additional Config</h2>");
  WiFiManagerParameter mqtt_server_head("<h3>MQTT Server</h3>");
  WiFiManagerParameter mqtt_server("server", "localhost", mqtt_server_val.c_str(), 40);
  WiFiManagerParameter mqtt_port_head("<h3>MQTT Port</h3>");
  WiFiManagerParameter mqtt_port("port", "1883", mqtt_port_val.c_str(), 5);
  WiFiManagerParameter device_name_head("<h3>Device Name</h3>");
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

  // Here we just print some information about the network
  // the device is now connected to.
  Serial.printf("The device is now connected to: %s\n", WiFi.SSID().c_str());
  Serial.printf("Device IP: \t%s\n", WiFi.localIP().toString().c_str());
  Serial.printf("Netmask: \t%s\n", WiFi.subnetMask().toString().c_str());
  Serial.printf("Gateway: \t%s\n", WiFi.gatewayIP().toString().c_str());
  Serial.printf("DNS IP: \t%s\n", WiFi.dnsIP().toString().c_str());

  Serial.println("Additional Config");
  Serial.printf("MQTT Server:\t%s\n", mqtt_server.getValue());
  Serial.printf("MQTT Port:\t%s\n", mqtt_port.getValue());
  Serial.printf("Device Name:\t%s\n", device_name.getValue());
}

void loop()
{
  drd.loop();
}