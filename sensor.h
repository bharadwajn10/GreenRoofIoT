#ifndef SENSOR_H // Changed from SENSOR_MANAGER_H
#define SENSOR_H

#include <Arduino.h>           // Necessary for base types (e.g., float, int) and functions (pinMode)
#include <Adafruit_BME280.h>   // Necessary for the BME280 object definition
#include <Wire.h>              // Necessary for I2C communication

// --- Configuration ---
// Sensor Pin Definitions
const int SOIL_MOISTURE_PIN = 34; // ESP32 GPIO34 (ADC1)
const int PH_SENSOR_PIN     = 35; // ESP32 GPIO35 (ADC1)
const int NUTRIENT_PIN      = 32; // ESP32 GPIO32 (ADC1)

// --- Data Structure ---

/**
 * @brief Struct to hold all collected sensor data points.
 */
struct SensorData {
  float temperature;      // *C (from BME280)
  float humidity;         // % (from BME280)
  float pressure;         // hPa (from BME280)
  float soilMoisture;     // % (Analog sensor)
  float ph;               // pH scale (Analog sensor)
  float nutrientIndex;    // Generic index (Placeholder for NSK)
};

// --- External Declarations ---

// Declare the BME280 object globally, defined (implemented) in sensor.cpp
extern Adafruit_BME280 bme;

// --- Function Prototypes ---

// The extern "C" block is retained as a safety measure for mixed-language compilation
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes all sensors (pin modes and I2C devices).
 */
void initSensors();

/**
 * @brief Reads data from all sensors and populates the SensorData struct.
 * @return A SensorData struct containing all current readings.
 */
SensorData readAllSensors();

#ifdef __cplusplus
}
#endif

#endif // SENSOR_H // Changed from SENSOR_MANAGER_H
