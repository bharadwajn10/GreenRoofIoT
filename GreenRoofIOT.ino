#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "WIFI NETWORK NAME";
const char* password = "WIFI PASSWORD";

const char* serverName = "http://<PC IP ADDRESS>:5000/insert_data"; // REPLACE WITH SYSTEM IP ADDRESS


// Dummy sensor variables

// WILL INCLUDE ACTUAL READINGS WHEN SENSORS ARE PURCHASED
float temperature = 25.0;
float humidity = 60.0;
float pressure = 1013.25;
float soilMoisture = 50.0;
float ph = 7.0;
float nutrientIndex = 500.0;



void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  delay(10000); // Send data every 10 seconds

  // Update dummy values
  temperature += 0.5;
  humidity -= 0.5;
  soilMoisture += 1.0;
  
  // Construct the JSON payload
  String httpRequestData = "{\"temperature\":" + String(temperature, 2) + 
                           ",\"humidity\":" + String(humidity, 2) + 
                           ",\"pressure\":" + String(pressure, 2) + 
                           ",\"soilMoisture\":" + String(soilMoisture, 2) + 
                           ",\"ph\":" + String(ph, 2) + 
                           ",\"nutrientIndex\":" + String(nutrientIndex, 2) + "}";

  HTTPClient http;
  http.begin(serverName);
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(httpRequestData);

  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println(response);
  } else {
    Serial.printf("Error: %s\n", http.errorToString(httpResponseCode).c_str());
  }
  http.end();
}