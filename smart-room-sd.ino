/*
  =====================================================================
   SMART ROOM SD
  =====================================================================
   Fitur:
   - Monitoring suhu & kelembaban (sensor DHT22)
   - Kontrol relay (ON/OFF) via aplikasi Blynk
   - Kontrol relay manual via 2 push button (PB1=ON, PB2=OFF)
   - Tampilan data suhu, kelembaban, status relay, & status WiFi di OLED
   - Integrasi platform Blynk (Blynk IoT / Blynk 2.0)

   Board   : ESP32-C3 Mini (Super Mini)
   Sensor  : DHT22
   Display : OLED SSD1306 128x64 (I2C)
   Relay   : Modul relay 1 channel (aktif HIGH)
  =====================================================================
*/

// ---------------------------------------------------------------------
// 1. KREDENSIAL BLYNK
//    Ambil dari Blynk.Console -> Template -> Device Info
// ---------------------------------------------------------------------
#define BLYNK_TEMPLATE_ID   "TMPLxxxxxxx"
#define BLYNK_TEMPLATE_NAME "Smart Room"
#define BLYNK_AUTH_TOKEN    "ISI_AUTH_TOKEN_ANDA"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------------------------------------------------------------------
// 2. KREDENSIAL WIFI
// ---------------------------------------------------------------------
char ssid[] = "NAMA_WIFI_ANDA";
char pass[] = "PASSWORD_WIFI_ANDA";

// ---------------------------------------------------------------------
// 3. KONFIGURASI PIN (ESP32-C3 Mini)
//    Sesuaikan jika layout board Anda berbeda.
// ---------------------------------------------------------------------
#define DHTPIN     4      // Pin data DHT22
#define DHTTYPE    DHT22
#define RELAY_PIN  5      // Pin kontrol relay
#define PB1_PIN    6      // Push button manual ON
#define PB2_PIN    7      // Push button manual OFF

#define OLED_SDA   8      // I2C SDA untuk OLED
#define OLED_SCL   9      // I2C SCL untuk OLED

#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C

// Set true jika relay module Anda aktif LOW (kebanyakan modul relay 1-channel murah aktif LOW)
#define RELAY_ACTIVE_LOW false

// ---------------------------------------------------------------------
// 4. VIRTUAL PIN BLYNK
//    V0 = suhu (read only, value display)
//    V1 = kelembaban (read only, value display)
//    V2 = kontrol relay (switch/button)
// ---------------------------------------------------------------------
#define VPIN_TEMP   V0
#define VPIN_HUMID  V1
#define VPIN_RELAY  V2

// ---------------------------------------------------------------------
// Objek global
// ---------------------------------------------------------------------
DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
BlynkTimer timer;

bool  relayState  = false;
float currentTemp  = NAN;
float currentHumid = NAN;

const unsigned long BUTTON_DEBOUNCE_MS = 50;

// ---------------------------------------------------------------------
// Helper: set relay fisik sesuai state logis + jenis modul (active low/high)
// ---------------------------------------------------------------------
void setRelay(bool state) {
  relayState = state;
  bool pinLevel = RELAY_ACTIVE_LOW ? !state : state;
  digitalWrite(RELAY_PIN, pinLevel ? HIGH : LOW);

  if (Blynk.connected()) {
    Blynk.virtualWrite(VPIN_RELAY, state ? 1 : 0);
  }
}

// ---------------------------------------------------------------------
// Tampilkan data terbaru ke OLED
// ---------------------------------------------------------------------
void updateOLED() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.println("== SMART ROOM ==");
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

  display.setCursor(0, 16);
  display.print("Suhu   : ");
  if (isnan(currentTemp)) {
    display.println("-- C (error)");
  } else {
    display.print(currentTemp, 1);
    display.println(" C");
  }

  display.setCursor(0, 28);
  display.print("Lembab : ");
  if (isnan(currentHumid)) {
    display.println("-- % (error)");
  } else {
    display.print(currentHumid, 1);
    display.println(" %");
  }

  display.setCursor(0, 42);
  display.print("Relay  : ");
  display.println(relayState ? "ON" : "OFF");

  display.setCursor(0, 54);
  display.print("WiFi   : ");
  display.println(WiFi.status() == WL_CONNECTED ? "Terhubung" : "Terputus");

  display.display();
}

// ---------------------------------------------------------------------
// Kontrol relay manual lewat 2 push button
// ---------------------------------------------------------------------
void handleManualButtons() {
  static bool lastPb1State = HIGH;
  static bool lastPb2State = HIGH;
  static unsigned long lastDebounceTime = 0;

  unsigned long now = millis();
  if (now - lastDebounceTime < BUTTON_DEBOUNCE_MS) {
    return;
  }
  lastDebounceTime = now;

  bool pb1State = digitalRead(PB1_PIN);
  bool pb2State = digitalRead(PB2_PIN);

  if (pb1State != lastPb1State) {
    lastPb1State = pb1State;
    if (pb1State == LOW) {
      setRelay(true);
      Serial.println("[MANUAL] Relay ON via PB1");
      updateOLED();
    }
  }

  if (pb2State != lastPb2State) {
    lastPb2State = pb2State;
    if (pb2State == LOW) {
      setRelay(false);
      Serial.println("[MANUAL] Relay OFF via PB2");
      updateOLED();
    }
  }
}

// ---------------------------------------------------------------------
// Baca sensor DHT22 lalu kirim ke Blynk & OLED
// ---------------------------------------------------------------------
void readSensorAndSend() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("[DHT22] Gagal membaca sensor!");
    currentTemp  = NAN;
    currentHumid = NAN;
    updateOLED();
    return;
  }

  currentTemp  = t;
  currentHumid = h;

  Serial.printf("[DHT22] Suhu: %.1f C | Kelembaban: %.1f %%\n", t, h);

  Blynk.virtualWrite(VPIN_TEMP, t);
  Blynk.virtualWrite(VPIN_HUMID, h);

  updateOLED();
}

// ---------------------------------------------------------------------
// Dipanggil otomatis saat tombol/switch relay di app Blynk ditekan
// ---------------------------------------------------------------------
BLYNK_WRITE(VPIN_RELAY) {
  int value = param.asInt(); // 0 atau 1
  setRelay(value == 1);
  Serial.printf("[RELAY] Diset ke %s dari Blynk\n", relayState ? "ON" : "OFF");
  updateOLED();
}

// ---------------------------------------------------------------------
// Sinkronisasi state setiap kali berhasil connect/reconnect ke Blynk
// ---------------------------------------------------------------------
BLYNK_CONNECTED() {
  Blynk.syncVirtual(VPIN_RELAY);
}

// ---------------------------------------------------------------------
// SETUP
// ---------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(PB1_PIN, INPUT_PULLUP);
  pinMode(PB2_PIN, INPUT_PULLUP);
  setRelay(false); // pastikan relay OFF saat boot

  dht.begin();

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("[OLED] Tidak terdeteksi, cek wiring I2C!");
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 20);
  display.println("Menghubungkan WiFi...");
  display.display();

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Baca & kirim data sensor setiap 2 detik
  timer.setInterval(2000L, readSensorAndSend);

  updateOLED();
}

// ---------------------------------------------------------------------
// LOOP
// ---------------------------------------------------------------------
void loop() {
  handleManualButtons();
  Blynk.run();
  timer.run();
}
