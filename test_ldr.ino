const int   LDR_PIN       = 34;   // ADC
const int   LDR_ADC_MAX   = 4095;
const uint8_t LDR_SAMPLES = 10;

const int TH_NOCHE     = 3600;
const int TH_AMANECER  = 3000;
const int TH_NUBLADO   = 2000;
const int TH_DIA_CLARO = 1000;

const char* describirPorADC(int adc) {
  if (adc >= TH_NOCHE)          return "Noche_/_muy_oscuro";
  else if (adc >= TH_AMANECER)  return "Amanecer/Atardecer";
  else if (adc >= TH_NUBLADO)   return "Nublado";
  else if (adc >= TH_DIA_CLARO) return "Dia_claro";
  else                          return "Sol_directo";
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("Test LDR"));
}

void loop() {
  unsigned long acc = 0;
  for (uint8_t i=0; i<LDR_SAMPLES; i++) { acc += analogRead(LDR_PIN); delay(2); }
  int adc = acc / LDR_SAMPLES;
  adc = constrain(adc, 0, LDR_ADC_MAX);

  Serial.print(F("ADC=")); Serial.print(adc);
  Serial.print(F("  -> ")); Serial.println(describirPorADC(adc));
  delay(500);
}
