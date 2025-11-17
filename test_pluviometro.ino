const byte PIN_HALL = 2;                 // sensor (A1104/KY-003) → GPIO2
const unsigned long DEBOUNCE_MS = 200;   // antirrebote
volatile unsigned long tipCount = 0;
volatile unsigned long lastTipMs = 0;

#if defined(ESP32) || defined(ESP8266)
void IRAM_ATTR onTip() {
#else
void onTip() {
#endif
  unsigned long now = millis();
  if (now - lastTipMs >= DEBOUNCE_MS) {
    tipCount++;
    lastTipMs = now;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("Test Pluviometro (balancin)"));
  pinMode(PIN_HALL, INPUT_PULLUP);       // colector abierto → PULLUP
  attachInterrupt(digitalPinToInterrupt(PIN_HALL), onTip, FALLING);
}

void loop() {
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 1000) {
    lastPrint = millis();
    noInterrupts();
    unsigned long c = tipCount;
    interrupts();
    Serial.print(F("Tips acumulados: "));
    Serial.println(c);
  }
}
