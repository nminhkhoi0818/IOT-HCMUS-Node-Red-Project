#include <WiFi.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>
#include "DHTesp.h"

// Wifi and MQTT
const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqttServer = "test.mosquitto.org";
int port = 1883;

// DHT
const int DHT_PIN = 15;
DHTesp dhtSensor;

// PIR motion
const int PIR_PIN = 4;
int pirState = LOW;

// LCD
LiquidCrystal_I2C LCD = LiquidCrystal_I2C(0x27, 20, 4);

// Buzzer
const int BUZ_PIN = 18;

// Led
const int LED_PIN = 5;


// Wifi
WiFiClient espClient;
PubSubClient client(espClient);

void wifiConnect() {
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected");
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.println(topic);
  String stMessage;
  for(int i = 0; i < length; i++) {
    stMessage += (char)message[i];
  }
  Serial.println(stMessage);
  if(String(topic) == "21CLC05N02/BUZZER") {
    if(stMessage == "true") {
      tone(BUZ_PIN, 200);
    }
    else {
      noTone(BUZ_PIN);
    }
  }
  else if(String(topic) == "21CLC05N02/LOCK") {
    if(stMessage == "true") {
      digitalWrite(LED_PIN, HIGH); 
      LCD.clear();
      LCD.setCursor(0, 0);
      LCD.print("Door is opened");
      delay(2000);
      digitalWrite(LED_PIN, LOW); 
    }
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  // Set up LCD
  LCD.init();
  LCD.backlight();
  LCD.setCursor(0, 0);
  LCD.print("Connecting to WiFi ...");
  wifiConnect();
  LCD.clear();
  LCD.setCursor(0, 0);
  LCD.print("Online");
  client.setServer(mqttServer, port);
  client.setCallback(callback);

  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
  pinMode(BUZ_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
}

void mqttReconnect() {
  while(!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if(client.connect("21CLC05N02")) {
      Serial.println(" connected");
      client.subscribe("21CLC05N02/BUZZER");
      client.subscribe("21CLC05N02/LOCK");
    } else {
      Serial.println(" try again in 5 seconds");
      delay(3000);
    }
  }
}

void output2LCD() {
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  int pir_state = digitalRead(PIR_PIN);

  if(data.temperature > 70 || pir_state == HIGH)
    tone(BUZ_PIN, 200);
  else noTone(BUZ_PIN);

  LCD.clear();
  LCD.setCursor(0, 0);
  LCD.print("Temp: " + String(data.temperature, 2) + "%C");
  LCD.setCursor(0, 1);
  LCD.print("Humidity: " + String(data.humidity, 1) + "%");
  LCD.setCursor(0, 2);
  if (pir_state == HIGH) LCD.print("Motion detected");
  else LCD.print("No motion detected");

  char buffer[50];
  String msg = String(data.temperature, 2) + "," + String(data.humidity, 1)
    + "," + String(pir_state);
  msg.toCharArray(buffer,50);
  client.publish("21CLC05N02/INFO", buffer);
}


void loop() {
  if(!client.connected()) {
    mqttReconnect();
  }
  client.loop();
  output2LCD();
  delay(2000);
}
