#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <sensitiveData.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <bitmaps.h>

#define MOISTURE_SENSOR 32
int delay_time = 1000 * 60 * 30;

// ===== Wifi.h =====
void ConnectionSetup();
const String SSID = SECRET_SSID;            // Nazwa sieci WiFi do ktorej MCU ma sie polaczyc
const String WIFI_PASSWD = SECRET_PASSWORD; // Haslo do tej sieci
WiFiClient espClient;

// ===== MQTT =====
void MQTTConnect();
const String MQTT_SERVER = "192.168.1.5";
const String MQTTTopic = "/home/plnt";
PubSubClient client(espClient);

// Wymiary ekranu (w pikselach)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET -1 // Numer pinu do resetowania ekranu (jeżeli jest dostępny)
#define SCREEN_ADDRESS 0x3C // Adres I2C ekranu OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void Draw_Humidity_Warning();

void setup()
{
    pinMode(MOISTURE_SENSOR, INPUT);
    Serial.begin(9600);
    ConnectionSetup();
    client.setServer(MQTT_SERVER.c_str(), 1883);
    // Pierwszy argument odpowiada za sposob zasilania ekranu  
    // SSD1306_SWITCHCAPVCC oznacza wykorzystanie wbudowanego regulatora
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
    {
        delay(5000);
        Serial.println(F("Screen init error!"));
        while(true);
    }
    // Poczatkowo buffer ekranu wypelniony jest logiem tworcow biblioteki
    display.display();
    delay(1000);
}

void loop()
{
    if (!client.connected())
    {
        MQTTConnect();
    }

    JsonDocument doc;
    char msg[16];
    int reading = analogRead(MOISTURE_SENSOR);

    // Wartosci graniczne (1685 i 3100) zostaly pozyskane przy tescie kalibracyjnym sensora.
    // Sensor zwracal wartosc ~3100 bedac calkowicie suchym
    // oraz ~1685 po calkowitym zanuzeniu w wodzie.
    if (reading < 1685)
        reading = 1685;
    else if (reading > 3100)
        reading = 3100;

    // Funkcja map() pozwala zmapowac wartosci z jednego zakresu do drugieg.
    // W tym przypadku konwertujemy wartosci zwracane przez czujnik na procentowa wilgotnosc.
    // Warto zwrocic uwage na odwrotna zaleznosc miedzy wartosciami zwracanymi przez czujnik a wilgotnoscia.
    unsigned int moisture = map(reading, 3100, 1685, 0, 100);

    doc["m"] = moisture;
    serializeJson(doc, msg);
    client.publish(MQTTTopic.c_str(), msg);
    Serial.println(msg);

    display.clearDisplay();

    if (moisture <= 25)
    {
        Draw_Humidity_Warning();
        display.drawBitmap(0, 16, sad, 48, 48, 1);
    }
    else if (moisture <= 50)
    {
        display.drawBitmap(0, 16, neutral, 48, 48, 1);
    }
    else if (moisture <= 75)
    {
        display.drawBitmap(0, 16, smile, 48, 48, 1);
    }
    else
        display.drawBitmap(0, 16, smile_wide, 48, 48, 1);

    display.setTextSize(4);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(56, 24);
    display.printf("%2u%%", moisture);

    display.display();
    delay(delay_time);
}

void Draw_Humidity_Warning()
{
    display.fillRect(0, 0, SCREEN_WIDTH, 16, SSD1306_WHITE);
    display.setTextSize(1);
    display.setTextColor(SSD1306_BLACK);
    display.setCursor(26, 0);
    display.print("!!! Niska !!!");
    display.setCursor(11, 8);
    display.println("!!! Wilgotnosc !!!");
}

void ConnectionSetup()
{
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

void MQTTConnect()
{
    while (!client.connected())
    {
        Serial.println("Connecting to MQTT Server, IP: " + String(MQTT_SERVER));

        // ID klienta
        String clientID = "ESP32-PLNT";

        if (client.connect(clientID.c_str()))
        {
            Serial.println("Connected to MQTT server!");
        }
        else
        {
            Serial.println("Connection failed, respone code " + client.state());
            delay(5000);
        }
    }
}
