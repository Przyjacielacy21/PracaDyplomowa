#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <sensitiveData.h>

#define RELAY1 19
#define RELAY2 18
#define RELAY3 17
#define RELAY4 16

// ===== Wifi.h =====
void ConnectionSetup();
const String SSID = SECRET_SSID;              // Nazwa sieci WiFi do ktorej MCU ma sie polaczyc
const String WIFI_PASSWD = SECRET_PASSWORD;   // Haslo do tej sieci
WiFiClient espClient;

// ===== MQTT =====
void MQTTConnect();
const String MQTT_SERVER = "192.168.1.5";
const String MQTT_Subscribe_Topic = "/home/relynode";
PubSubClient client(espClient);
void callback(char* topic, byte* message, unsigned int length);

void setup() {
  pinMode(RELAY1, OUTPUT);
  digitalWrite(RELAY1, HIGH);
  pinMode(RELAY2, OUTPUT);
  digitalWrite(RELAY2, HIGH);
  pinMode(RELAY3, OUTPUT);
  digitalWrite(RELAY3, HIGH);
  pinMode(RELAY4, OUTPUT);
  digitalWrite(RELAY4, HIGH);
  
  Serial.begin(9600);
  ConnectionSetup();
  client.setServer(MQTT_SERVER.c_str(), 1883);
  client.setCallback(callback);
}

void loop() {
  if(!client.connected()){
    MQTTConnect();
  }
  client.loop();
}

void ConnectionSetup(){
  Serial.println("Connecting to SSID " + SSID);
  WiFi.begin(SSID, WIFI_PASSWD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print('.');
  }

  Serial.print("Connected, assigned IP: ");
  Serial.println(WiFi.localIP());
}

void MQTTConnect(){
  while (!client.connected())   
  {
    Serial.println("Connecting to MQTT Server, IP: " + String(MQTT_SERVER));

    //Create client ID
    String clientID = "ESP32-RELYNODE";

    if(client.connect(clientID.c_str())){
      client.subscribe(MQTT_Subscribe_Topic.c_str(), 1);
      Serial.println("Connected to MQTT server!");
    }
    else{
      Serial.println("Connection failed, respone code " + client.state());
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  JsonDocument doc;
  
  for (int i = 0; i < length; i++) {
    messageTemp += (char)payload[i];
    Serial.print((char)payload[i]);
  }
  deserializeJson(doc, messageTemp);
  Serial.println();

  int r1 = 1, r2 = 1, r3 = 1, r4 = 1;
  
  if(doc.containsKey("r1")){
    int state = doc["r1"];
    if(state == 0 | state == 1){
      digitalWrite(RELAY1, state);
    }
  }

  if(doc.containsKey("r2")){
    int state = doc["r2"];
    if(state == 0 | state == 1){
      digitalWrite(RELAY2, state);
    }
  }

  if(doc.containsKey("r3")){
    int state = doc["r3"];
    if(state == 0 | state == 1){
      digitalWrite(RELAY3, state);
    }
  }

  if(doc.containsKey("r4")){
    int state = doc["r4"];
    if(state == 0 | state == 1){
      digitalWrite(RELAY4, state);
    }
  }
}