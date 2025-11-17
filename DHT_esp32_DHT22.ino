#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "DHT.h"

/* ========================== WiFi ========================== */
//const char* ssid     = "Personal-634-2.4GHz";
//const char* password = "B74B573634";

const char* ssid     = "Gestion Educativa";
const char* password = "Educacion.2021";
WiFiClientSecure client;  // HTTPS (sin cert con setInsecure())

/* ========================== DHT22 ========================= */
#define DHTPIN   14
#define DHTTYPE  DHT2 2
DHT dht(DHTPIN, DHTTYPE);
float t, h;

/* ======================== Pluviómetro ===================== */
float mm, mm_h, tips;

const byte  PIN_HALL          = 2;      // Entrada de balancín (A1104/KY-003)
const float FUNNEL_RADIUS_MM  = 8.0;    // radio (mm)
const float TIP_VOLUME_ML     = 3.0;    // volumen por vuelco (mL)
const float AREA_MM2          = PI * FUNNEL_RADIUS_MM * FUNNEL_RADIUS_MM;
const float MM_PER_TIP        = (TIP_VOLUME_ML * 1000.0) / AREA_MM2;

const unsigned long PRINT_INTERVAL_MS = 1000;       // salida serial cada 1 s
const unsigned long RATE_WINDOW_MS    = 60000;      // tasa promedio 1 min
const unsigned long DEBOUNCE_MS       = 200;        // anti-rebote
const unsigned long HOUR_RESET_MS     = 3600000UL;  // reinicio acumulado cada 1 h

volatile unsigned long tipCountTotal  = 0;
volatile unsigned long tipCountWindow = 0;
volatile unsigned long lastTipMs      = 0;

unsigned long lastPrintMs   = 0;
unsigned long windowStartMs = 0;
unsigned long lastResetMs   = 0;

#if defined(ESP32) || defined(ESP8266)
void IRAM_ATTR onTip() {
#else
void onTip() {
#endif
  unsigned long now = millis();
  if (now - lastTipMs >= DEBOUNCE_MS) {
    tipCountTotal++;
    tipCountWindow++;
    lastTipMs = now;
  }
}

/* ============================ LDR ========================= */
const char* estado;
const int   LDR_PIN       = 34;
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

/* ===================== Anemómetro A1104 =================== */
volatile uint32_t pulseCount = 0;
volatile uint32_t lastPulseUs = 0;

uint32_t pulses;
float    hz, rps, v_mps, v_kmh, gustKmh;

const int   PIN_HALL_ANENOMETRO = 27;   // OUT del A1104 a GPIO27
const int   MAGNETS_PER_REV     = 1;    // Nº imanes por vuelta
const float RADIUS_M            = 0.109f; // radio (m)
const float CALIB_K             = 1.0f;   // factor empírico

const uint32_t SAMPLE_MS      = 1000;  // periodo de reporte (ms)
const uint32_t GUST_WINDOW_MS = 3000;  // ventana de racha (ms)
const uint32_t MIN_PULSE_US   = 1500;  // anti-rebote (1.5 ms ~ 666 Hz)

uint32_t lastSampleMs = 0;
float    gustBuffer[6];                 // 3 s con SAMPLE_MS=1000
int      gustIdx = 0;
bool     gustFilled = false;

void IRAM_ATTR onPulse() {
  uint32_t nowUs = micros();
  if (nowUs - lastPulseUs >= MIN_PULSE_US) {
    pulseCount++;
    lastPulseUs = nowUs;
  }
}

/* ===================== Veleta 4× A1104 ==================== */
const uint8_t PIN_N = 5;    // N
const uint8_t PIN_E = 18;   // E
const uint8_t PIN_S = 19;   // S
const uint8_t PIN_O = 21;   // O

const uint8_t VELETA_PINS[4] = { PIN_N, PIN_E, PIN_S, PIN_O };
const char*   DIR_LABELS[4]  = { "N", "E", "S", "O" };
const int     DIR_DEGREES[4] = {   0,  90, 180, 270 };

int dir_idx      = -1;  // lectura actual (0..3) o -1 si inestable
int last_dir_idx = -1;  // última lectura válida

const uint8_t  SAMPLES_VELETA  = 9;
const uint16_t SAMPLE_DELAY_MS = 3;
const uint8_t  MIN_HITS        = (SAMPLES_VELETA + 1) / 2;

String direccion_str, grado_str;

/* ================ Sensor de lluvia superficie ============= */
const char* tipo;

const int   RAIN_PIN  = 35;    // ADC1
const int   ADC_MAX   = 4095;  // 12 bits
const uint8_t SAMPLES = 10;

int  ADC_SECO   = 3500;
int  ADC_MOJADO = 1200;
bool INVERT     = false;

const int UMB_SECO     = 5;
const int UMB_ROCIO    = 20;
const int UMB_LLOVIZNA = 40;
const int UMB_LIGERA   = 60;
const int UMB_MODERADA = 80;

/* ===================== Prototipos envío =================== */
void EnvioDatosTemperaturaGet();
void EnvioDatosPluviometroGet();
void EnvioDatosLDR_KY_018Get();
void EnvioDatosAnemometroGet();
void EnvioDatosVeletaGet();
void EnvioDatosSensorAguaGet();

/* ======================== Setup =========================== */
void setup() {
  Serial.begin(115200);
  Serial.println(F("DHT22 - Prueba de conexión al servidor"));
  dht.begin();
  delay(2000);

  Serial.print(F("Conectando a WiFi"));
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  client.setInsecure();  // HTTPS sin cert
  Serial.println(F("\nConexión OK!!"));
  Serial.print(F("IP local: "));
  Serial.println(WiFi.localIP());

  /* ----- Pluviómetro ----- */
  pinMode(PIN_HALL, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_HALL), onTip, FALLING);
  windowStartMs = lastPrintMs = lastResetMs = millis();
  Serial.println(F("Pluviometro listo"));
  Serial.print(F("mm por vuelco (calibrado): "));
  Serial.println(MM_PER_TIP, 6);

  /* ----- Anemómetro A1104 ----- */
  pinMode(PIN_HALL_ANENOMETRO, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_HALL_ANENOMETRO), onPulse, FALLING);
  lastSampleMs = millis();
  Serial.println(F("Anemómetro A1104 listo"));

  /* ----- Veleta 4× A1104 ----- */
  for (uint8_t i = 0; i < 4; i++) {
    pinMode(VELETA_PINS[i], INPUT_PULLUP);   // A1104: colector abierto → activo LOW
  }
  Serial.println(F("Veleta A1104 lista (N,E,S,O)."));
  Serial.println(F("********************************************************"));
}

/* ========================= Loop =========================== */
void loop() {
  /* DHT */
  LecturaTH();
  EnvioDatosTemperaturaGet();

  /* Pluviómetro */
  Pluviometro();
  EnvioDatosPluviometroGet();

  /* LDR */
  LDR_KY_018();
  EnvioDatosLDR_KY_018Get();

  /* Anemómetro */
  loopAnemometroA1104();
  EnvioDatosAnemometroGet();

  /* Veleta (usa último válido si no hay lectura estable) */
  loopVeletaA1104();
  EnvioDatosVeletaGet();

  /* Sensor lluvia de superficie */
  SensorAgua();
  EnvioDatosSensorAguaGet();

  /* Enviar cada 60 s */
  delay(60000);
}

/* ==================== DHT: lectura/envío ================== */
void LecturaTH() {
  h = dht.readHumidity();
  t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println(F("¡Error al leer el sensor DHT!"));
    return;
  }
  Serial.print(F("Temperatura (C): "));
  Serial.println(t, 1);
  Serial.print(F("Humedad: "));
  Serial.println(h, 1);
}

void EnvioDatosTemperaturaGet() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("WiFi no conectado."));
    return;
  }

  HTTPClient http;
  String url = "https://sandybrown-dragonfly-215740.hostingersite.com/TempHum.php"
               "?temperatura=" + String(t, 1) +
               "&humedad=" + String(h, 1);

  http.begin(client, url);
  int codigo = http.GET();
  if (codigo > 0) {
    Serial.print(F("HTTP "));
    Serial.println(codigo);
    if (codigo == 200) Serial.println(http.getString());
  } else {
    Serial.print(F("Error GET: "));
    Serial.println(codigo);
  }
  http.end();
}

/* ================ Pluviómetro: cálculo/envío ============== */
void Pluviometro() {
  unsigned long now = millis();

  // Reinicio acumulados cada hora
  if (now - lastResetMs >= HOUR_RESET_MS) {
    noInterrupts();
    tipCountTotal  = 0;
    tipCountWindow = 0;
    interrupts();

    windowStartMs = lastResetMs = now;
    Serial.println(F("[RESET 1h] Contadores en cero"));
  }

  // Reporte cada segundo (y cálculo de tasas)
  if (now - lastPrintMs >= PRINT_INTERVAL_MS) {
    lastPrintMs = now;

    // Copias atómicas
    noInterrupts();
    unsigned long tipsTotal  = tipCountTotal;
    unsigned long tipsWindow = tipCountWindow;
    interrupts();

    // Lluvia acumulada (mm)
    float rainTotalMM = tipsTotal * MM_PER_TIP;

    // Tasa mm/h en la ventana
    unsigned long windowMs = now - windowStartMs;
    if (windowMs == 0) windowMs = 1;
    float tipsPerSec  = (float)tipsWindow / (float)windowMs * 1000.0;
    float rainRateMMh = tipsPerSec * 3600.0 * MM_PER_TIP;

    Serial.print(F("Total(mm): "));
    Serial.print(rainTotalMM, 3);
    Serial.print(F(" | Tasa(mm/h): "));
    Serial.print(rainRateMMh, 3);
    Serial.print(F(" | tips: "));
    Serial.println(tipsTotal);

    mm   = rainTotalMM;
    mm_h = rainRateMMh;
    tips = tipsTotal;

    // Deslizar ventana de tasa cada minuto
    if (windowMs >= RATE_WINDOW_MS) {
      noInterrupts();
      tipCountWindow = 0;
      interrupts();
      windowStartMs = now;
    }
  }
}

void EnvioDatosPluviometroGet() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("WiFi no conectado."));
    return;
  }

  HTTPClient http;
  String url = "https://sandybrown-dragonfly-215740.hostingersite.com/Pluviometro.php"
               "?mm="   + String(mm, 1) +
               "&mm_h=" + String(mm_h, 1) +
               "&tips=" + String(tips, 1);

  http.begin(client, url);
  int codigo = http.GET();
  if (codigo > 0) {
    Serial.print(F("HTTP "));
    Serial.println(codigo);
    if (codigo == 200) Serial.println(http.getString());
  } else {
    Serial.print(F("Error GET: "));
    Serial.println(codigo);
  }
  http.end();
}

/* ===================== LDR: lectura/envío ================= */
void LDR_KY_018() {
  unsigned long acc = 0;
  for (uint8_t i = 0; i < LDR_SAMPLES; i++) {
    acc += analogRead(LDR_PIN);
    delay(2);
  }
  int adc = constrain(acc / LDR_SAMPLES, 0, LDR_ADC_MAX);
  estado = describirPorADC(adc);

  Serial.print(adc);
  Serial.print(F(", "));
  Serial.println(estado);

  delay(200);
}

void EnvioDatosLDR_KY_018Get() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("WiFi no conectado."));
    return;
  }

  HTTPClient http;
  String url = "https://sandybrown-dragonfly-215740.hostingersite.com/Ldr.php"
               "?estado=" + String(estado);

  http.begin(client, url);
  int codigo = http.GET();
  if (codigo > 0) {
    Serial.print(F("HTTP "));
    Serial.println(codigo);
    if (codigo == 200) Serial.println(http.getString());
  } else {
    Serial.print(F("Error GET: "));
    Serial.println(codigo);
  }
  http.end();
}

/* ============== Anemómetro A1104: loop/envío ============== */
void loopAnemometroA1104() {
  uint32_t nowMs = millis();
  if (nowMs - lastSampleMs >= SAMPLE_MS) {
    uint32_t dtMs = nowMs - lastSampleMs;
    lastSampleMs  = nowMs;

    // Lectura atómica
    noInterrupts();
    pulses = pulseCount;
    pulseCount = 0;
    interrupts();

    // Hz y vueltas/s
    hz  = (float)pulses / ((float)dtMs / 1000.0f);
    rps = (MAGNETS_PER_REV > 0) ? (hz / (float)MAGNETS_PER_REV) : 0.0f;

    // Velocidad lineal
    v_mps = (2.0f * PI * RADIUS_M) * rps * CALIB_K;
    v_kmh = v_mps * 3.6f;

    // Racha (máximo en ventana)
    gustBuffer[gustIdx] = v_kmh;
    gustIdx = (gustIdx + 1) % (GUST_WINDOW_MS / SAMPLE_MS);
    if (gustIdx == 0) gustFilled = true;

    gustKmh = v_kmh;
    int n = gustFilled ? (GUST_WINDOW_MS / SAMPLE_MS) : gustIdx;
    for (int i = 0; i < n; i++) {
      gustKmh = max(gustKmh, gustBuffer[i]);
    }

    // Debug
    Serial.print(F("Anemómetro: pulses=")); Serial.print(pulses);
    Serial.print(F("  hz="));   Serial.print(hz, 2);
    Serial.print(F("  v="));    Serial.print(v_mps, 2); Serial.print(F(" m/s  "));
    Serial.print(v_kmh, 2);     Serial.print(F(" km/h  gust="));
    Serial.print(gustKmh, 2);   Serial.println(F(" km/h"));
  }
}

void EnvioDatosAnemometroGet() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("WiFi no conectado."));
    return;
  }

  HTTPClient http;
  String url = "https://sandybrown-dragonfly-215740.hostingersite.com/Anemometro.php"
               "?pulses=" + String(pulses) +
               "&hz="     + String(hz) +
               "&rps="    + String(rps) +
               "&v="      + String(v_mps) +
               "&km_h="   + String(v_kmh) +
               "&gust="   + String(gustKmh);

  http.begin(client, url);
  int codigo = http.GET();
  if (codigo > 0) {
    Serial.print(F("HTTP "));
    Serial.println(codigo);
    if (codigo == 200) Serial.println(http.getString());
  } else {
    Serial.print(F("Error GET: "));
    Serial.println(codigo);
  }
  http.end();
}

/* ================ Veleta A1104: loop/envío ================ */
int leerDireccionA1104() {
  uint8_t counts[4] = { 0, 0, 0, 0 };

  for (uint8_t s = 0; s < SAMPLES_VELETA; s++) {
    for (uint8_t i = 0; i < 4; i++) {
      if (digitalRead(VELETA_PINS[i]) == LOW) counts[i]++; // LOW = imán presente
    }
    delay(SAMPLE_DELAY_MS);
  }

  uint8_t best = 255;
  uint8_t bestCount = 0;
  for (uint8_t i = 0; i < 4; i++) {
    if (counts[i] > bestCount) {
      bestCount = counts[i];
      best = i;
    }
  }
  if (best == 255 || bestCount < MIN_HITS) return -1;
  return (int)best;
}

void loopVeletaA1104() {
  dir_idx = leerDireccionA1104();

  if (dir_idx < 0) {
    // No hay lectura estable → usamos el último válido si existe
    if (last_dir_idx >= 0) {
      Serial.print(F("Veleta: sin lectura estable → uso último válido "));
      Serial.print(DIR_LABELS[last_dir_idx]);
      Serial.print(F(" ("));
      Serial.print(DIR_DEGREES[last_dir_idx]);
      Serial.println(F("°)"));
    } else {
      Serial.println(F("Veleta: sin lectura estable (aún no hay último válido)"));
    }
  } else {
    last_dir_idx = dir_idx;
    Serial.print(F("Veleta: "));
    Serial.print(DIR_LABELS[dir_idx]);
    Serial.print(F(" ("));
    Serial.print(DIR_DEGREES[dir_idx]);
    Serial.println(F("°)"));
  }
}

void EnvioDatosVeletaGet() {
  // Si no hay lectura estable, enviar último válido
  int effective_idx = (dir_idx >= 0) ? dir_idx : last_dir_idx;

  if (effective_idx < 0) {
    direccion_str = "---";
    grado_str     = "---";
  } else {
    direccion_str = DIR_LABELS[effective_idx];
    grado_str     = String(DIR_DEGREES[effective_idx]);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("WiFi no conectado."));
    return;
  }

  HTTPClient http;
  String url = "https://sandybrown-dragonfly-215740.hostingersite.com/Veleta.php"
               "?direccion=" + direccion_str +
               "&grado="     + grado_str;

  Serial.print(F("Enviando a URL: "));
  Serial.println(url);

  http.begin(client, url);
  int code = http.GET();
  if (code > 0) {
    Serial.print(F("HTTP "));
    Serial.println(code);
    if (code == 200) Serial.println(http.getString());
  } else {
    Serial.print(F("Error GET: "));
    Serial.println(code);
  }
  http.end();
}

/* ========== Sensor de lluvia superficie: lectura/envío ======== */
int leerADCsuavizado() {
  uint32_t acc = 0;
  for (uint8_t i = 0; i < SAMPLES; i++) {
    acc += analogRead(RAIN_PIN);
    delay(2);
  }
  int adc = acc / SAMPLES;
  return constrain(adc, 0, ADC_MAX);
}

int calcularHumedadSuperficie(int adc) {
  int a = ADC_SECO, b = ADC_MOJADO;
  if (a == b) b = a + 1;

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

void SensorAgua() {
  int adc = leerADCsuavizado();
  int pct = calcularHumedadSuperficie(adc);
  tipo = clasificarLluvia(pct);

  Serial.print(adc);
  Serial.print(F(", "));
  Serial.print(pct);
  Serial.print(F("%, "));
  Serial.println(tipo);

  delay(500);
}

void EnvioDatosSensorAguaGet() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("WiFi no conectado."));
    return;
  }

  HTTPClient http;
  String url = "https://sandybrown-dragonfly-215740.hostingersite.com/SensorAgua.php"
               "?estado=" + String(tipo);

  http.begin(client, url);
  int codigo = http.GET();
  if (codigo > 0) {
    Serial.print(F("HTTP "));
    Serial.println(codigo);
    if (codigo == 200) Serial.println(http.getString());
  } else {
    Serial.print(F("Error GET: "));
    Serial.println(codigo);
  }
  http.end();
}
