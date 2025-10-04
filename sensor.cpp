#include "sensor.h" // Changed from SensorManager.h
#include <WiFi.h> // Included here to ensure map() is available if needed

// --- Global Sensor Objects ---
// Definition of the BME280 object (declared as 'extern' in the header)
Adafruit_BME280 bme; 

// --- Calibration Constants ---
// General ADC/Voltage reference
const float VREF = 3.3; 
const int ADC_RESOLUTION = 4095;

// Soil Moisture Calibration (Adjust these based on your specific sensor and soil conditions)
const int ADC_MIN_DRY = 3200; 
const int ADC_MAX_WET = 1200; 

// pH Calibration (Adjust these based on your specific pH sensor module)
const float PH7_VOLTAGE = 1.5;      // Voltage output at pH 7.0 (must be calibrated)
const float VOLTAGE_PER_PH = 0.18;  // Slope/Change in voltage per pH unit (must be calibrated)

// Nutrient Index Scaling
const float NUTRIENT_INDEX_MAX = 100.0;

// --- Private Helper Functions (Internal Sensor Reading) ---

float readSoilMoistureInternal() {
  int rawValue = analogRead(SOIL_MOISTURE_PIN);
  float moisture = map(rawValue, ADC_MIN_DRY, ADC_MAX_WET, 0, 100);
  return constrain(moisture, 0, 100);
}

float readPHSensorInternal() {
  int rawValue = analogRead(PH_SENSOR_PIN);
  float voltage = (float)rawValue * (VREF / ADC_RESOLUTION);
  // Conversion formula based on the calibrated PH7_VOLTAGE and slope
  float phValue = 7.0 + ((PH7_VOLTAGE - voltage) / VOLTAGE_PER_PH);
  return constrain(phValue, 0.0, 14.0);
}

float readNutrientSensorInternal() {
  int rawValue = analogRead(NUTRIENT_PIN);
  // Simple linear scaling for placeholder
  float nutrientIndex = (float)rawValue / ADC_RESOLUTION * NUTRIENT_INDEX_MAX;
  return nutrientIndex;
}

void readBME280Internal(float *temp, float *hum, float *pres) {
  *temp = bme.readTemperature();
  *hum = bme.readHumidity();
  *pres = bme.readPressure() / 100.0F; // Convert Pa to hPa

  if (isnan(*temp) || isnan(*hum) || isnan(*pres)) {
    Serial.println("Error reading BME280 data!");
    *temp = -999.0;
    *hum = -999.0;
    *pres = -999.0;
  }
}

// --- Public Interface Functions (Defined in sensor.h) ---

void initSensors() {
  // Initialize Analog Pins 
  pinMode(SOIL_MOISTURE_PIN, INPUT);
  pinMode(PH_SENSOR_PIN, INPUT);
  pinMode(NUTRIENT_PIN, INPUT);
  
  // Initialize BME280 sensor (I2C)
  if (!bme.begin(0x76)) { 
    Serial.println("Could not find BME280 sensor at 0x76, trying 0x77...");
    if (!bme.begin(0x77)) { 
        Serial.println("BME280 sensor not found! Check wiring and address.");
    }
  } else {
    Serial.println("BME280 initialized successfully.");
  }
}


SensorData readAllSensors() {
  SensorData data;
  
  // Read BME280 data
  readBME280Internal(&data.temperature, &data.humidity, &data.pressure);
  
  // Read analog soil sensors
  data.soilMoisture = readSoilMoistureInternal();
  data.ph = readPHSensorInternal();
  data.nutrientIndex = readNutrientSensorInternal();

  return data;
}
