#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiManager.h>  
#include <Preferences.h>  
#include "test.h"
#include <nvs_flash.h>
#include <time.h> // Required for syncing real-world time (NTP)

// --- Global Variables ---
Preferences preferences;
String serverName; 
unsigned long timerDelay; // Now dynamic, no longer a hardcoded constant

// We moved WiFiManager to the global scope so it stays alive forever
WiFiManager wm; 
WiFiManagerParameter* custom_server; 
WiFiManagerParameter* custom_interval;

// Time tracking for clock-aligned readings
int lastTriggerMinute = -1;
int lastTriggerSecond = -1;
unsigned long fallbackLastTime = 0; 

// Static placeholders for missing sensors
float nutrientIndex = 500.0; 
const float localAltitude = 910.0; // Altitude in meters
float finalPressure = 0.0;

// --- Callback triggered when you hit "Save" on the Captive Portal ---
void saveParamCallback() {
    Serial.println("[PORTAL] Saving new parameters...");
    
    serverName = custom_server->getValue();
    String intervalStr = custom_interval->getValue();
    timerDelay = intervalStr.toInt() * 1000;
    
    if(timerDelay < 5000) timerDelay = 5000; // Enforce minimum 5 second interval
    
    preferences.begin("agri-app", false);
    preferences.putString("server_url", serverName);
    preferences.putString("interval_sec", intervalStr);
    preferences.end();
    
    Serial.printf("Saved -> Server: %s, Interval: %lu ms\n", serverName.c_str(), timerDelay);
}

void setup() {
    // 1. Initialize Serial Communication
    Serial.begin(115200);
    while(!Serial); 
    delay(1000);    
    
    Serial.println("\n--- Starting Smart Agriculture System ---");
    
    // 2. NVS Flash Management (Do not erase!)
    nvs_flash_init();
    
    // 3. Load Saved Settings from Preferences
    preferences.begin("agri-app", false);
    serverName = preferences.getString("server_url", "http://192.168.1.173:5000/insert_data");
    String savedInterval = preferences.getString("interval_sec", "300"); // Default 300 seconds (5 mins)
    timerDelay = savedInterval.toInt() * 1000;
    preferences.end();

    // 4. Setup WiFiManager for ALWAYS-ON Non-Blocking Mode
    Serial.println("DEBUG: Preparing Wi-Fi Settings...");
    
    // Put WiFiManager in background mode so it doesn't freeze the loop
    wm.setConfigPortalBlocking(false);
    
    // Add custom fields
    custom_server = new WiFiManagerParameter("server_url", "Python Server URL", serverName.c_str(), 100);
    custom_interval = new WiFiManagerParameter("interval_sec", "Reading Interval (seconds)", savedInterval.c_str(), 10);
    
    wm.addParameter(custom_server);
    wm.addParameter(custom_interval);
    wm.setSaveParamsCallback(saveParamCallback);

    Serial.println("Attempting connection to saved Wi-Fi...");

    // 5. Attempt Connection
    wm.autoConnect("SmartAgri_Setup", "admin123"); 

    // 6. ⚠️ THE FIX: FORCE AP TO STAY ALIVE ⚠️
    // Even if autoConnect succeeds, it tries to hide the AP. We force it back on here.
    WiFi.mode(WIFI_AP_STA); 
    WiFi.softAP("SmartAgri_Setup", "admin123"); 
    wm.startWebPortal(); // Keeps the settings page available at 192.168.4.1

    Serial.println("\n--- Wi-Fi System Ready ---");
    Serial.println("Hotspot 'SmartAgri_Setup' is broadcasting 24/7.");
    Serial.print("Local IP (For Python Server): ");
    Serial.println(WiFi.localIP());

    // 7. Setup Real-World Time (NTP) for Bengaluru (IST is GMT+5:30)
    // 19800 seconds = 5.5 hours
    configTime(19800, 0, "pool.ntp.org");
    Serial.println("Synchronizing exact time...");

    // 8. Initialize Sensors
    Serial.println("DEBUG: Calling initSensors()...");
    initSensors();
}

void loop() {
    // MUST BE CALLED CONSTANTLY: This keeps the configuration webpage responsive in the background
    wm.process(); 

    bool triggerReading = false;
    struct tm timeinfo;
    
    // Check if we successfully grabbed the real time from the internet (non-blocking)
    bool hasTime = getLocalTime(&timeinfo, 10); 

    // --- LOGIC TO EVENLY DISTRIBUTE READINGS BASED ON THE CLOCK ---
    if (hasTime && timerDelay >= 60000) {
        // Interval is 1 minute or more (e.g., 300s = 5 mins). Sync to exact minutes (HH:05, HH:10)
        int intervalMins = (timerDelay / 1000) / 60;
        
        if (timeinfo.tm_min % intervalMins == 0 && timeinfo.tm_sec == 0) {
            if (lastTriggerMinute != timeinfo.tm_min) {
                lastTriggerMinute = timeinfo.tm_min;
                triggerReading = true;
            }
        }
    } 
    else if (hasTime && timerDelay < 60000) {
        // Interval is less than 1 minute. Sync to exact seconds.
        int intervalSecs = timerDelay / 1000;
        
        if (timeinfo.tm_sec % intervalSecs == 0) {
            if (lastTriggerSecond != timeinfo.tm_sec) {
                lastTriggerSecond = timeinfo.tm_sec;
                triggerReading = true;
            }
        }
    } 
    else {
        // FALLBACK: If Wi-Fi is down or time hasn't synced yet, rely on standard millis() timer
        if ((millis() - fallbackLastTime) > timerDelay) {
            fallbackLastTime = millis();
            triggerReading = true;
        }
    }

    // --- SENSOR READING AND TRANSMISSION BLOCK ---
    if (triggerReading) {
        
        Serial.println("\n--- Taking Scheduled Sensor Reading ---");

        // 1. Read All Sensors
        BME280Data bmeData = readBMESensor(); 
        DHTData dhtData = readDHT();          
        MoistureData moistureData = readSoilMoisture();
        PHData phData = readPH();

        // 2. Prepare Final Data Variables
        float finalTemp = bmeData.isValid ? bmeData.temperature : (dhtData.isValid ? dhtData.temperature : 0.0);
        
        float absolutePressure = bmeData.isValid ? bmeData.pressure : 0.0;     
        if (bmeData.isValid) {
            finalPressure = absolutePressure / pow(1.0 - (localAltitude / 44330.0), 5.255);
        }
        
        float finalHumidity = dhtData.isValid ? dhtData.humidity : 0.0;
        float finalMoisture = moistureData.isValid ? moistureData.percent : 0.0;
        float finalPH = phData.isValid ? phData.phValue : 0.0;

        // Print status
        Serial.printf("Temperature: %.2f C\n", finalTemp);
        Serial.printf("Pressure: %.2f hPa\n", finalPressure);
        Serial.printf("Humidity: %.2f %%\n", finalHumidity);
        Serial.printf("Soil Moisture: %.2f %%\n", finalMoisture);
        Serial.printf("pH Value: %.2f\n", finalPH);

        // 3. Transmit Data via Wi-Fi
        if (WiFi.status() == WL_CONNECTED) {
            HTTPClient http;
            
            http.begin(serverName.c_str());
            http.addHeader("Content-Type", "application/json");

            String httpRequestData = "{\"temperature\":" + String(finalTemp, 2) + 
                                     ",\"humidity\":" + String(finalHumidity, 2) + 
                                     ",\"pressure\":" + String(finalPressure, 2) + 
                                     ",\"soilMoisture\":" + String(finalMoisture, 2) + 
                                     ",\"ph\":" + String(finalPH, 2) + 
                                     ",\"nutrientIndex\":" + String(nutrientIndex, 2) + "}";
            
            Serial.print("Sending Payload: ");
            Serial.println(httpRequestData);

            int httpResponseCode = http.POST(httpRequestData);

            if (httpResponseCode > 0) {
                Serial.printf("HTTP Response code: %d\n", httpResponseCode);
            } else {
                Serial.printf("HTTP Error: %s\n", http.errorToString(httpResponseCode).c_str());
            }
            
            http.end();
        } else {
            Serial.println("Wi-Fi Disconnected. ESP32 is attempting to auto-reconnect in the background...");
        }
        
        Serial.println("---------------------------------");
    }
}