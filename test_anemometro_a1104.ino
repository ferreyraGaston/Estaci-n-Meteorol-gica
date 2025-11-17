const int   PIN_HALL = 27;     // A1104 OUT → GPIO27
const int   MAGNETS_PER_REV = 1;
const float RADIUS_M = 0.109f; // radio de tu rotor (m)
const float CALIB_K  = 1.0f;   // factor empírico

const uint32_t SAMPLE_MS    = 1000;
const uint32_t MIN_PULSE_US = 1500;

volatile uint32_t pulseCount = 0;
volatile uint32_t lastPulseUs = 0;

void IRAM_ATTR onPulse() {
  uint32_t nowUs = micros();
  if (nowUs - lastPulseUs >= MIN_PULSE_US) {
    pulseCount++;
    lastPulseUs = nowUs;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("Test Anemometro A1104"));
  pinMode(PIN_HALL, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_HALL), onPulse, FALLING);
}

void loop() {
  static uint32_t last = 0;
  if (millis() - last >= SAMPLE_MS) {
    last = millis();

    noInterrupts();
    uint32_t p = pulseCount;
    pulseCount = 0;
    interrupts();

    float hz  = (float)p / (SAMPLE_MS/1000.0f);
    float rps = (MAGNETS_PER_REV > 0) ? (hz / MAGNETS_PER_REV) : 0.0f;
    float v_mps = (2.0f * PI * RADIUS_M) * rps * CALIB_K;
    float v_kmh = v_mps * 3.6f;

    Serial.print(F("pulses=")); Serial.print(p);
    Serial.print(F("  hz="));   Serial.print(hz,2);
    Serial.print(F("  v="));    Serial.print(v_mps,2); Serial.print(F(" m/s  "));
    Serial.print(v_kmh,2);      Serial.println(F(" km/h"));
  }
}
