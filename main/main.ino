#include <WiFi.h>
#include <ArduinoHA.h>

/* 
Create secrets.ino
#define BROKER_ADDR     IPAddress(192,168,0,1)
#define BROKER_USERNAME     "mosquitto" 
#define BROKER_PASSWORD     "password123"

#define WIFI_SSID       "MyWifiNetwork"
#define WIFI_PASSWORD   "password123"
*/

constexpr int RELAY_PIN  = 17;
constexpr int ANALOG_PIN = 1;

constexpr int SENSOR_INTERVAL  = 5*60*1000; // 5m
constexpr int MAX_PUMP_TIME  = 10*1000; // 10s

WiFiClient client;
HADevice device("device-waterer-1");
HAMqtt mqtt(client, device);

unsigned long lastUpdateAt = -SENSOR_INTERVAL;
unsigned long pumpStartAt = 0;

HASwitch pumpSwitch("water-pump-1");
HASensorNumber moistureSensor("moisture-sensor-1", HASensorNumber::PrecisionP0);

void onSwitchCommand(bool state, HASwitch* sender)
{
    if (state) {
      Serial.println("Switch triggered: on");
      pumpStartAt = millis();
    } else {
      Serial.println("Switch triggered: off");
    }
    
    digitalWrite(RELAY_PIN, (state ? HIGH : LOW));
    sender->setState(state); // report state back to the Home Assistant
}

void setup() {
    Serial.begin(9600);
    Serial.println("Starting...");

    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);

    // connect to wifi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500); // waiting for the connection
    }
    Serial.println();
    Serial.println("Connected to the network");

    // set device's details (optional)
    device.setName("Plant Waterer 1");
    device.setSoftwareVersion("1.0.0");
    device.enableSharedAvailability();

    // handle switch state
    pumpSwitch.onCommand(onSwitchCommand);
    pumpSwitch.setName("Water Pump 1"); // optional
    pumpSwitch.setIcon("mdi:water");

    moistureSensor.setIcon("mdi:flower");
    moistureSensor.setName("Moisture Level 1");
    moistureSensor.setUnitOfMeasurement("%"); 

    mqtt.begin(BROKER_ADDR, BROKER_USERNAME, BROKER_PASSWORD);
}

void loop() {
    mqtt.loop();

    if ((millis() - lastUpdateAt) > SENSOR_INTERVAL) { 
        readMoisture(); 
    }

    if ((millis() - pumpStartAt) > MAX_PUMP_TIME && pumpSwitch.getCurrentState()) { 
        Serial.println("Pump time expired");
        digitalWrite(RELAY_PIN, LOW);
        pumpSwitch.setState(false); 
    }
}

void readMoisture() {
      uint16_t raw_moisture = analogRead(ANALOG_PIN);
      uint16_t moisture = map(raw_moisture , 8500, 4000, 0, 100); 

      Serial.println("Reading moisture: " + moisture);

      moistureSensor.setValue(moisture);
      lastUpdateAt = millis();
}

