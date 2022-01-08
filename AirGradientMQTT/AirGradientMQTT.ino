/*
This is my custom code for the AirGradient DIY Air Quality Sensor with an ESP8266 Microcontroller.

Author: Patrick Bollmann
Email: patrickbollmann00@gmail.com

Based on the original AirGradient esp software: https://github.com/airgradienthq/arduino/blob/master/examples/C02_PM_SHT_OLED_WIFI/C02_PM_SHT_OLED_WIFI.ino
In comparison to the original build the display has been removed but you can easily add the display capability by copying the corresponding lines from the ogiginal code into this sketch.

It is a high quality sensor showing PM2.5, CO2, Temperature and Humidity and can send data over Wifi with MQTT.

For build instructions please visit https://www.airgradient.com/diy/

Compatible with the following sensors:
Plantower PMS5003 (Fine Particle Sensor)
SenseAir S8 (CO2 Sensor)
SHT30/31 (Temperature/Humidity Sensor)
*/

#include <AirGradient.h>

#include <WiFiManager.h>

#include <ESP8266WiFi.h>

#include<PubSubClient.h>

#include <ArduinoOTA.h>

#include <Wire.h>

AirGradient ag = AirGradient();

const char* MQTT_BROKER = "192.168.xxx.xxx";  //your MQTT Server address

// set sensors that you do not use to false
boolean hasPM = true;
boolean hasCO2 = true;
boolean hasSHT = true;

// set to true to switch PM2.5 from ug/m3 to US AQI
boolean inUSaqi = false;

// set to true to switch from Celcius to Fahrenheit
boolean inF = false;

// set to true if you want to connect to wifi. The display will show values only when the sensor has wifi connection
boolean connectWIFI = true;

WiFiClient client;
PubSubClient mqttclient(client);
    
void setup() {
  Serial.begin(9600);
  //init sensors
  if (hasPM) ag.PMS_Init();
  if (hasCO2) ag.CO2_Init();
  if (hasSHT) ag.TMP_RH_Init(0x44);
  
  //check wifi connection
  if (connectWIFI) connectToWifi(); 

  mqttclient.setServer(MQTT_BROKER, 1883);

  ArduinoOTA.setHostname("Airgradient Sensor");
  delay(2000);
}

//connect to MQTT Server
void reconnect() {
  // Loop until we're reconnected
  while (!mqttclient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttclient.connect("airgradient")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      mqttclient.publish("airgradient", "hello world");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttclient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {

  ArduinoOTA.handle();

  if (!mqttclient.connected()) {
    reconnect();
  }

  // create payload

  String payload = "{\"wifi\":" + String(WiFi.RSSI()) + ",";

  if (hasPM) {
    int PM2 = ag.getPM2_Raw();
    payload = payload + "\"pm02\":" + String(PM2);
    delay(3000);
  }

  if (hasCO2) {
    if (hasPM) payload = payload + ",";
    int CO2 = ag.getCO2_Raw();
    payload = payload + "\"rco2\":" + String(CO2);
    delay(3000);
  }

  if (hasSHT) {
    if (hasCO2 || hasPM) payload = payload + ",";
    TMP_RH result = ag.periodicFetchData();
    payload = payload + "\"atmp\":" + String(result.t) + ",\"rhum\":" + String(result.rh);
    delay(3000);
  }

  payload = payload + "}";

  // send payload
  if (connectWIFI) {
    
    Serial.print("Publish message: ");
    Serial.println(payload);
    mqttclient.publish("airgradient",(const char * ) payload.c_str());
    
  }
}

// Wifi Manager
void connectToWifi() {
  WiFiManager wifiManager;
  //WiFi.disconnect(); //to delete previous saved hotspot
  String HOTSPOT = "AIRGRADIENT-" + String(ESP.getChipId(), HEX);
  wifiManager.setTimeout(120);
  if (!wifiManager.autoConnect((const char * ) HOTSPOT.c_str())) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    ESP.restart();
    delay(5000);
  }

}

// Calculate PM2.5 US AQI
int PM_TO_AQI_US(int pm02) {
  if (pm02 <= 12.0) return ((50 - 0) / (12.0 - .0) * (pm02 - .0) + 0);
  else if (pm02 <= 35.4) return ((100 - 50) / (35.4 - 12.0) * (pm02 - 12.0) + 50);
  else if (pm02 <= 55.4) return ((150 - 100) / (55.4 - 35.4) * (pm02 - 35.4) + 100);
  else if (pm02 <= 150.4) return ((200 - 150) / (150.4 - 55.4) * (pm02 - 55.4) + 150);
  else if (pm02 <= 250.4) return ((300 - 200) / (250.4 - 150.4) * (pm02 - 150.4) + 200);
  else if (pm02 <= 350.4) return ((400 - 300) / (350.4 - 250.4) * (pm02 - 250.4) + 300);
  else if (pm02 <= 500.4) return ((500 - 400) / (500.4 - 350.4) * (pm02 - 350.4) + 400);
  else return 500;
};
