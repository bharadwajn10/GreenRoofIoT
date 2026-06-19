#include "test.h"

// --- Global Sensor Objects & Variables ---
Adafruit_BME280 bme;
Adafruit_BMP280 bmp;
DHT dht(DHT_PIN, DHT_TYPE);

int activeSensor = 0; // 0 = None, 1 = BMP280, 2 = BME280
bool bmeIsInitialized = false;

// pH Calibration Variables
const float VREF = 3.3;
const float VOLTAGE_AT_PH7 = 2.5; 
const float VOLTAGE_AT_PH4 = 3.0; 
const float phStep = (VOLTAGE_AT_PH4 - VOLTAGE_AT_PH7) / 3.0;

// --- Initialization Functions ---

void initSensors() {
    // Set ADC resolution for ESP32
    analogReadResolution(12);
    
    // Initialize DHT sensor
    dht.begin();
}

// ---------------------------------------------------------
// Function: Check I2C and identify the chip directly
// ---------------------------------------------------------
int checkBMESensor() {
    // 1. Check if ANY device is answering at address 0x76
    Wire.beginTransmission(0x76);
    if (Wire.endTransmission() != 0) {
        Serial.println("I2C Check: FAILED. No device found at 0x76.");
        return 0; // Return 0 for 'None'
    }
    
    // 2. Ask the chip for its internal ID (Register 0xD0)
    Wire.beginTransmission(0x76);
    Wire.write(0xD0); 
    Wire.endTransmission();
    
    // Read the 1-byte reply
    Wire.requestFrom(0x76, 1);
    if (Wire.available()) {
        byte chipID = Wire.read();
        
        // 3. Display results based on the ID
        if (chipID == 0x58) {
            Serial.println("I2C Check: SUCCESS! BMP280 detected (Temp/Pressure only).");
            return 1; // Return 1 for BMP280
        } else if (chipID == 0x60) {
            Serial.println("I2C Check: SUCCESS! BME280 detected (Includes Humidity).");
            return 2; // Return 2 for BME280
        } else {
            Serial.print("I2C Check: UNKNOWN CHIP. ID is 0x");
            Serial.println(chipID, HEX);
            return 0;
        }
    }
    return 0;
}

// --- Sensor Read Functions ---

BME280Data readBMESensor(int SCL_PIN , int SDA_PIN ) {
    BME280Data data;
    data.isValid = false; 
    data.temperature = 0.0;
    data.humidity = 0.0;
    data.pressure = 0.0;
    
    // Only run the setup and check process if we haven't successfully connected yet
    if (!bmeIsInitialized) {
        Wire.begin(SDA_PIN, SCL_PIN);
        activeSensor = checkBMESensor();
        
        if (activeSensor == 1) {
            if (bmp.begin(0x76)) bmeIsInitialized = true;
        } else if (activeSensor == 2) {
            if (bme.begin(0x76, &Wire)) bmeIsInitialized = true;
        }
    }

    // If still not initialized, display error and return the empty struct
    if (!bmeIsInitialized) {
        Serial.println("TROUBLESHOOTING BME/BMP: If all the wiring and I2C connections are proper, then definitely the sensor is faulty. Please make sure you connect only 3.3V and not VIN or 5V.");
        return data; 
    }

    // --- READ DATA FOR BMP280 ---
    if (activeSensor == 1) {
        data.temperature = bmp.readTemperature();
        data.humidity = 0.0; // BMP280 does not support humidity
        data.pressure = bmp.readPressure() / 100.0F; // Convert to hPa
        
        if (isnan(data.temperature) || isnan(data.pressure)) {
            data.isValid = false;
        } else {
            data.isValid = true;
        }
    } 
    // --- READ DATA FOR BME280 ---
    else if (activeSensor == 2) {
        data.temperature = bme.readTemperature();
        data.humidity = bme.readHumidity();
        data.pressure = bme.readPressure() / 100.0F; // Convert to hPa
        
        if (isnan(data.temperature) || isnan(data.humidity) || isnan(data.pressure)) {
            data.isValid = false;
        } else {
            data.isValid = true;
        }
    }
    
    return data;
}

DHTData readDHT() {
    DHTData data;
    data.humidity = dht.readHumidity();
    data.temperature = dht.readTemperature();
    
    // Check if any reads failed
    if (isnan(data.humidity) || isnan(data.temperature)) {
        data.isValid = false;
        data.humidity = 0.0;
        data.temperature = 0.0;
        Serial.println("TROUBLESHOOTING DHT22: Failed to read from DHT sensor! Check data pin wiring, ensure power is 3.3V/5V, and verify a 10k pull-up resistor is installed if your module lacks one.");
    } else {
        data.isValid = true;
    }
    
    return data;
}

MoistureData readSoilMoisture() {
    MoistureData data;
    long soilSum = 0;
    const int samples = 20;

    // Oversampling to smooth out electrical noise
    for (int i = 0; i < samples; i++) {
        soilSum += analogRead(MOISTURE_PIN);
        delay(10);
    }
    
    data.rawADC = soilSum / samples;

    // Hardware Failure Check: Only check for a dead short to ground (<= 5)
    // We removed the >= 4090 check because 4095 is a valid "bone dry" reading.
    if (data.rawADC <= 5) {
        data.isValid = false;
        data.percent = 0;
        Serial.println("TROUBLESHOOTING MOISTURE: Sensor returning near 0. The analog pin is likely shorted to ground or the module lost power.");
    } else {
        data.isValid = true;
        // Map raw ADC (4095 dry, 1050 wet) to percentage based on your calibration
        int soilPercent = map(data.rawADC, 4095, 1050, 0, 100);
        data.percent = constrain(soilPercent, 0, 100); 
    }

    return data;
}

PHData readPH() {
    PHData data;
    long phSum = 0;
    const int samples = 20;
    
    for (int i = 0; i < samples; i++) {
        phSum += analogRead(PH_PIN);
        delay(10);
    }
    
    data.rawADC = phSum / samples;

    // Hardware Failure Check: If ADC is continuously pinned at 0 or 4095
    if (data.rawADC <= 5 || data.rawADC >= 4090) {
        data.isValid = false;
        data.voltage = 0.0;
        data.phValue = 0.0;
        Serial.println("TROUBLESHOOTING pH: Probe returning extreme limits. Check BNC connector, verify signal wire to ESP32, and ensure the module is powered correctly.");
    } else {
        data.isValid = true;
        data.voltage = data.rawADC * (VREF / 4095.0);
        data.phValue = 7.0 + ((VOLTAGE_AT_PH7 - data.voltage) / phStep);
    }
    
    return data;
}