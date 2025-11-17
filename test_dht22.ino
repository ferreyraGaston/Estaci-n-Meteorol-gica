#include "DHT.h"

#define DHTPIN   14
#define DHTTYPE  DHT22
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  Serial.println(F("Test DHT22"));
  dht.begin();
}

void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println(F("Error leyendo DHT22"));
  } else {
    Serial.print(F("Temp: ")); Serial.print(t, 1); Serial.print(F(" °C  |  Hum: "));
    Serial.print(h, 1); Serial.println(F(" %"));
  }
  delay(2000);
}
