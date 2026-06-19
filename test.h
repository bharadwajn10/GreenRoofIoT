#ifndef TEST_H
#define TEST_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h> 
#include <DHT.h> // Added for DHT22 support

// --- Pin Definitions ---
#define MOISTURE_PIN 39
#define PH_PIN 34
#define DHT_PIN 4      // Define the pin you are using for the DHT22
#define DHT_TYPE DHT22 // Define the DHT type

// --- Data Structures ---

// Struct to hold BME280/BMP280 readings
struct BME280Data {
    float temperature;
    float humidity;
    float pressure;
    bool isValid; 
};

// Struct to hold DHT readings
struct DHTData {
    float humidity;
    float temperature;
    bool isValid;
};

// Struct to hold Soil Moisture readings
struct MoistureData {
    int percent;
    int rawADC;
    bool isValid;
};

// Struct to hold pH readings
struct PHData {
    int rawADC;
    float voltage;
    float phValue;
    bool isValid;
};

// --- Function Declarations ---

// Initialization functions
void initSensors();

// Universal BME/BMP Sensor functions
int checkBMESensor();
BME280Data readBMESensor(int SCL_PIN = 22, int SDA_PIN = 21);

// DHT Sensor functions
DHTData readDHT();

// Other Analog Read functions
MoistureData readSoilMoisture();
PHData readPH();

#endif // TEST_H