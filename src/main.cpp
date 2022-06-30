#include <Arduino.h>
#include <Wire.h>
#include <HTTPClient.h>
#include <WiFi.h>
// Sensors
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <BH1750.h>
#include <DHT.h>

// Macros
#define TEMPCORRECTION(t) (t - 1.0) // enter your corrections
#define PRESSURECORRECTION(p) (p - 3) // enter your corrections
#define SLEEP_TIME_IN_MINUTES(t) (t * 60000000)

#define DHTPIN 4
#define DHTTYPE DHT11
#define TIME_TO_SLEEP 5

// Functions
void connectToWifi();
void goToSleep();
void initSensors();
void getSensorData();
void ifConnectedSendData();
void bootUp();

// Struct to hold all data
typedef struct wxData
{
    int pressure;
    float temperature;
    int lux;
    float humidity;
} wxData_t;

// Variables to save in deepsleep
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR wxData_t wx;

// sensors init global
Adafruit_BMP280 bmp;
BH1750 luxMeter(0x23);
DHT dht(DHTPIN, DHTTYPE);

// Login credentials 
const char *serverName = "local host";
const char *ssid = "Wifi name";
const char *password = "Wifi password";

// Main program
void setup()
{
    bootUp();
    initSensors();
}

void loop()
{
    getSensorData();
    connectToWifi();
    ifConnectedSendData();
    goToSleep();
}

// Initiate Serial and enable deepsleep
void bootUp()
{
    Serial.begin(9600);
    delay(2000); // Take some time to open up the Serial Monitor
    ++bootCount;
    Serial.println("Boot number: " + String(bootCount));
    esp_sleep_enable_timer_wakeup(SLEEP_TIME_IN_MINUTES(5));
}

// Initiate sensors
void initSensors()
{
    Wire.begin();
    dht.begin();
    luxMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
    bmp.begin(0x76);
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                    Adafruit_BMP280::SAMPLING_X2, 
                    Adafruit_BMP280::SAMPLING_X16,
                    Adafruit_BMP280::FILTER_X16,  
                    Adafruit_BMP280::STANDBY_MS_500);
}

// Extract sensor data and saves them to data struct
void getSensorData()
{
    delay(1000);
    wx.pressure = bmp.readPressure() / 100;
    wx.temperature = TEMPCORRECTION(bmp.readTemperature());
    wx.lux = 0;
    wx.humidity = PRESSURECORRECTION(dht.readHumidity());
    if (luxMeter.measurementReady())
    {
        wx.lux = luxMeter.readLightLevel();
    }
}

// Connect to wifi. After 10 unsuccessfully tries goes to deepsleep
void connectToWifi()
{
    int tries = 0;
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    // Checks if WiFIi has connected 
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.print(".");
        ++tries;
        if (tries > 10)
        {
            Serial.println("Could not connect!!!");
            goToSleep();
        }
    }
    Serial.println("WiFi connected..!");
}

// If connected to WiFi then send http post request.
void ifConnectedSendData()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        WiFiClient client;
        HTTPClient http;
        http.begin(client, serverName);
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        String httpRequestData = "api_key=tPmAT5Ab3j7F9&sensor=&temp=" + String(wx.temperature) + "&lux=" + String(wx.lux) + "&pressure=" + String(wx.pressure) + "&humidity=" + String(wx.humidity);
        http.POST(httpRequestData);
        http.end();
    }
    else
    {
        Serial.println("Not Connected to WiFi!!");
    }
}

// Initiate sleep
void goToSleep()
{
    Serial.println("Going to sleep now");
    // All rows down to esp_deep_sleep is a try to make my ESP32 draw less power during deep sleep
    Serial.flush();
    Serial.end();
    Wire.end();
    esp_sleep_pd_config(ESP_PD_DOMAIN_MAX, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
    delay(1000);
    esp_deep_sleep_start();
}