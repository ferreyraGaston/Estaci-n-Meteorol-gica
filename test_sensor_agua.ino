const int   RAIN_PIN  = 35;    // ADC1
const int   ADC_MAX   = 4095;  // 12 bits
const uint8_t SAMPLES = 10;

int  ADC_SECO   = 3500;        // calibra con tu sensor
int  ADC_MOJADO = 1200;
bool INVERT     = false;

const int UMB_SECO     = 5;
const int UMB_ROCIO    = 20;
const int UMB_LLOVIZNA = 40;
const int UMB_LIGERA   = 60;
const int UMB_MODERADA = 80;

int leerADCsuavizado() {
  uint32_t acc=0; for (uint8_t i=0;i<SAMPLES;i++){ acc+=analogRead(RAIN_PIN); delay(2); }
  int adc = acc / SAMPLES;
  return constrain(adc, 0, ADC_MAX);
}

int calcularHumedadSuperficie(int adc) {
  int a=ADC_SECO, b=ADC_MOJADO; if (a==b) b=a+1;
  int pct = map(adc, a, b, 0, 100);
  if (INVERT) pct = 100 - pct;
  return constrain(pct, 0, 100);
}

const char* clasificarLluvia(int pct) {
  if (pct <= UMB_SECO)          return "Seco";
  else if (pct <= UMB_ROCIO)    return "Rocio/Salpicado";
  else if (pct <= UMB_LLOVIZNA) return "Llovizna";
  else if (pct <= UMB_LIGERA)   return "Lluvia_ligera";
  else if (pct <= UMB_MODERADA) return "Lluvia_moderada";
  else                          return "Lluvia_fuerte";
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("Test Sensor de lluvia superficie"));
}

void loop() {
  int adc = leerADCsuavizado();
  int pct = calcularHumedadSuperficie(adc);
  const char* tipo = clasificarLluvia(pct);

  Serial.print(F("ADC=")); Serial.print(adc);
  Serial.print(F("  |  ")); Serial.print(pct); Serial.print(F("%  |  "));
  Serial.println(tipo);

  delay(500);
}
const int   RAIN_PIN  = 35;    // ADC1
const int   ADC_MAX   = 4095;  // 12 bits
const uint8_t SAMPLES = 10;

int  ADC_SECO   = 3500;        // calibra con tu sensor
int  ADC_MOJADO = 1200;
bool INVERT     = false;

const int UMB_SECO     = 5;
const int UMB_ROCIO    = 20;
const int UMB_LLOVIZNA = 40;
const int UMB_LIGERA   = 60;
const int UMB_MODERADA = 80;

int leerADCsuavizado() {
  uint32_t acc=0; for (uint8_t i=0;i<SAMPLES;i++){ acc+=analogRead(RAIN_PIN); delay(2); }
  int adc = acc / SAMPLES;
  return constrain(adc, 0, ADC_MAX);
}

int calcularHumedadSuperficie(int adc) {
  int a=ADC_SECO, b=ADC_MOJADO; if (a==b) b=a+1;
  int pct = map(adc, a, b, 0, 100);
  if (INVERT) pct = 100 - pct;
  return constrain(pct, 0, 100);
}

const char* clasificarLluvia(int pct) {
  if (pct <= UMB_SECO)          return "Seco";
  else if (pct <= UMB_ROCIO)    return "Rocio/Salpicado";
  else if (pct <= UMB_LLOVIZNA) return "Llovizna";
  else if (pct <= UMB_LIGERA)   return "Lluvia_ligera";
  else if (pct <= UMB_MODERADA) return "Lluvia_moderada";
  else                          return "Lluvia_fuerte";
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("Test Sensor de lluvia superficie"));
}

void loop() {
  int adc = leerADCsuavizado();
  int pct = calcularHumedadSuperficie(adc);
  const char* tipo = clasificarLluvia(pct);

  Serial.print(F("ADC=")); Serial.print(adc);
  Serial.print(F("  |  ")); Serial.print(pct); Serial.print(F("%  |  "));
  Serial.println(tipo);

  delay(500);
}
