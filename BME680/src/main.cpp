#include <Arduino.h>
#include "bsec.h"
#include "ArduinoJson.h"
#include "WiFi.h"
#include "PubSubClient.h"

#include "sensitiveData.h"

#define SCK 18
#define MISO 19
#define MOSI 23
#define CS_BME680 5

// ===== bsec =====
void checkIaqSensorStatus();
void errLeds();
Bsec BME680;
String output;

// ===== Wifi.h =====
void ConnectionSetup();
const String SSID = SECRET_SSID;              // Nazwa sieci WiFi do ktorej MCU ma sie polaczyc
const String WIFI_PASSWD = SECRET_PASSWORD;   // Haslo do tej sieci
WiFiClient espClient;

// ===== MQTT =====
void MQTTConnect();
const String MQTT_SERVER = "192.168.1.5";
const String MQTTTopic = "/home/basement/BME680";
PubSubClient client(espClient);

void setup()
{
  Serial.begin(115200);
  delay(1000);
  pinMode(LED_BUILTIN, OUTPUT);
  SPI.begin(SCK, MISO, MOSI, CS_BME680);
  BME680.begin(CS_BME680, SPI);
  checkIaqSensorStatus();

  // lista dostępnych parametrow czujnika
  bsec_virtual_sensor_t sensorList[12] = {
    BSEC_OUTPUT_IAQ,                                      // Indoor Air Quality, nizsze wartosci są lepsze
    BSEC_OUTPUT_STATIC_IAQ,                               // IAQ przed skalowaniem
    BSEC_OUTPUT_CO2_EQUIVALENT,                           // szacowany ekwiwalent wdychanego CO2 w ppm
    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,                    // szacowany ekwiwalent wdychanych lotnych zwiazkow organicznych w ppm
    BSEC_OUTPUT_RAW_TEMPERATURE,                          // bezposredni odczyt temperatury w stopniach Celsjusza
    BSEC_OUTPUT_RAW_PRESSURE,                             // bezposredni odczyt cisnienia w Paskalach 
    BSEC_OUTPUT_RAW_HUMIDITY,                             // bezposredni odczyt wilgotnosci w procentach 
    BSEC_OUTPUT_RAW_GAS,                                  // bezposredni odczyt czujnika gazow w Ohmach
    BSEC_OUTPUT_STABILIZATION_STATUS,                     // stan pierwszej stabilizacji czujnika gazow, 0 - w trakcie, 1 - skalibrowany
    BSEC_OUTPUT_RUN_IN_STATUS,                            // stan stabilizacji czujnika gazow przed odczytem, 0 - w trakcie, 1 - skalibrowany
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,      // odczytana temperatura po skalowaniu w stopniach Celsjusza
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY          // odczytana wilgotnosc po skalowaniu w procentach
    //BSEC_OUTPUT_GAS_PERCENTAGE                            
  };

  BME680.updateSubscription(sensorList, 12, BSEC_SAMPLE_RATE_LP);
  checkIaqSensorStatus();

  ConnectionSetup();
  client.setServer(MQTT_SERVER.c_str(), 1883);
  digitalWrite(BUILTIN_LED, HIGH);
}

void loop()
{
  if(!client.connected()){
    MQTTConnect();
  }

  StaticJsonDocument<120> doc;
  char msg[120];

  //sprawdzenie czy sa dostepne nowe dane
  if (BME680.run()) { 
    float temperature = BME680.temperature;
    float humidity = BME680.humidity;
    float pressure = BME680.pressure / 100.0;
    float iaq = BME680.iaq;
    float co2 = BME680.co2Equivalent;

    doc["t"] = temperature;
    doc["h"] = humidity;
    doc["p"] = pressure;
    doc["i"] = iaq;
    doc["c"] = co2;

    serializeJson(doc, msg);
    Serial.println("============================================");
    if (!BME680.iaqAccuracy) Serial.println("Czujnik w trakcie kalibracji poczatkowej!");
    else digitalWrite(BUILTIN_LED, LOW);
    Serial.println(msg);
    client.publish(MQTTTopic.c_str(), msg);

  } else {
    checkIaqSensorStatus();
  }

  delay(1000);
}

void checkIaqSensorStatus()
{
  if (BME680.bsecStatus != BSEC_OK) {
    if (BME680.bsecStatus < BSEC_OK) {
      Serial.println("BSEC error code : " + String(BME680.bsecStatus));
      for (;;)
        errLeds(); /* Halt in case of failure */
    } else {
      Serial.println("BSEC warning code : " + String(BME680.bsecStatus));
    }
  }

  if (BME680.bme68xStatus != BME68X_OK) {
    if (BME680.bme68xStatus < BME68X_OK) {
      Serial.println("BME680 error code : " + String(BME680.bme68xStatus));
      for (;;)
        errLeds(); /* Halt in case of failure */
    } else {
      Serial.println("BME680 warning code : " + String(BME680.bme68xStatus));
    }
  }
}

void errLeds()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
}

void ConnectionSetup(){
  Serial.println("Connecting to SSID " + SSID);
  WiFi.begin(SSID, WIFI_PASSWD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print('.');
  }

  Serial1.print("Connected, assigned IP: " + WiFi.localIP());
}

void MQTTConnect(){
  while (!client.connected())   
  {
    Serial.println("Connecting to MQTT Server, IP: " + String(MQTT_SERVER));

    //Create client ID
    String clientID = "ESP32-BME680";

    if(client.connect(clientID.c_str())){
      Serial.println("Connected to MQTT server!");
    }
    else{
      Serial.println("Connection failed, respone code " + client.state());
      delay(5000);
    }
  }
}