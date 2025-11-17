const uint8_t PIN_N = 5, PIN_E = 18, PIN_S = 19, PIN_O = 21;
const uint8_t VELETA_PINS[4] = { PIN_N, PIN_E, PIN_S, PIN_O };
const char*   DIR_LABELS[4]  = { "N", "E", "S", "O" };
const int     DIR_DEGREES[4] = {   0,  90, 180, 270 };

const uint8_t  SAMPLES_VELETA  = 9;
const uint16_t SAMPLE_DELAY_MS = 3;
const uint8_t  MIN_HITS        = (SAMPLES_VELETA + 1) / 2;

int last_dir_idx = -1;

int leerDireccionA1104() {
  uint8_t counts[4] = {0,0,0,0};
  for (uint8_t s=0; s<SAMPLES_VELETA; s++) {
    for (uint8_t i=0; i<4; i++) if (digitalRead(VELETA_PINS[i]) == LOW) counts[i]++;
    delay(SAMPLE_DELAY_MS);
  }
  uint8_t best = 255, bestCount = 0;
  for (uint8_t i=0; i<4; i++) if (counts[i] > bestCount) { bestCount = counts[i]; best = i; }
  if (best == 255 || bestCount < MIN_HITS) return -1;
  return (int)best;
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("Test Veleta 4xA1104 (N,E,S,O)"));
  for (uint8_t i=0; i<4; i++) pinMode(VELETA_PINS[i], INPUT_PULLUP);
}

void loop() {
  int idx = leerDireccionA1104();
  if (idx < 0) {
    if (last_dir_idx >= 0) {
      Serial.print(F("Sin lectura estable → último: "));
      Serial.print(DIR_LABELS[last_dir_idx]); Serial.print(F(" ("));
      Serial.print(DIR_DEGREES[last_dir_idx]); Serial.println(F("°)"));
    } else {
      Serial.println(F("Sin lectura estable (aún sin último válido)"));
    }
  } else {
    last_dir_idx = idx;
    Serial.print(F("Direccion: "));
    Serial.print(DIR_LABELS[idx]); Serial.print(F(" ("));
    Serial.print(DIR_DEGREES[idx]); Serial.println(F("°)"));
  }
  delay(250);
}
