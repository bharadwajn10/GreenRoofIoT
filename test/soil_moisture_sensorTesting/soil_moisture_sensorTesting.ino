// FC-28 / YL-69 Soil Moisture Test
// ESP32 DevKit V1

const int MOISTURE_PIN = 39;

void setup() {
  Serial.begin(115200);

  analogReadResolution(12);  // ESP32 ADC: 0-4095

  Serial.println();
  Serial.println("=================================");
  Serial.println(" SOIL MOISTURE SENSOR TEST ");
  Serial.println("=================================");
  Serial.println("Place sensor in:");
  Serial.println("1. Air");
  Serial.println("2. Soil");
  Serial.println("3. Water");
  Serial.println();
}

void loop() {

  long sum = 0;
  const int samples = 20;

  for (int i = 0; i < samples; i++) {
    sum += analogRead(MOISTURE_PIN);
    delay(10);
  }

  int raw = sum / samples;

  Serial.print("Raw ADC Value: ");
  Serial.println(raw);

  delay(1000);
}