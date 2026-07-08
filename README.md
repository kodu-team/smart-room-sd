# Smart Room SD

Sistem monitoring & kontrol ruangan berbasis IoT menggunakan **ESP32-C3 Mini**, sensor suhu **DHT22**, **modul relay**, layar **OLED**, dan platform **Blynk**.

## Fitur

- 🌡️ Monitoring suhu & kelembaban real-time (DHT22)
- 🔌 Kontrol relay ON/OFF dari aplikasi Blynk (jarak jauh)
- 🔘 Kontrol relay manual via 2 push button (PB1 = ON, PB2 = OFF) dengan debounce 50ms
- 📟 Tampilan suhu, kelembaban, dan status relay di layar OLED
- ☁️ Sinkronisasi data ke dashboard Blynk (grafik & histori otomatis tersedia di Blynk)
- 🔄 Auto-sync status relay saat board reconnect ke Blynk (state tidak hilang)
- 🔄 Sinkronisasi otomatis ke Blynk saat tombol manual ditekan (relay state tetap akurat di aplikasi)

## Komponen yang Dibutuhkan

| Komponen | Jumlah | Keterangan |
| --- | --- | --- |
| ESP32-C3 Mini (Super Mini) | 1 | Mikrokontroler utama, WiFi built-in |
| Sensor DHT22 | 1 | Sensor suhu & kelembaban |
| Modul Relay 1 Channel | 1 | Untuk switching beban AC/DC (lampu, kipas, dll) |
| OLED SSD1306 128x64 (I2C) | 1 | Display data, alamat I2C 0x3C |
| Push Button | 2 | Untuk kontrol manual relay (PB1 ON, PB2 OFF) |
| Resistor 10kΩ | 1 (opsional) | Pull-up untuk DHT22 jika modul tidak sudah include |
| Kabel jumper + breadboard | secukupnya | |
| Adaptor 5V / kabel USB-C | 1 | Power supply |

## Skema Wiring

| Perangkat | Pin Perangkat | Pin ESP32-C3 Mini |
| --- | --- | --- |
| DHT22 | VCC | 3V3 |
| DHT22 | GND | GND |
| DHT22 | DATA | GPIO 3 |
| Relay | VCC | 5V / VIN |
| Relay | GND | GND |
| Relay | IN | GPIO 7 |
| Push Button PB1 | satu kaki | GPIO 0 |
| Push Button PB1 | kaki lain | GND |
| Push Button PB2 | satu kaki | GPIO 1 |
| Push Button PB2 | kaki lain | GND |
| OLED | VCC | 3V3 |
| OLED | GND | GND |
| OLED | SDA | GPIO 8 |
| OLED | SCL | GPIO 9 |

> ⚠️ **Catatan Penting:**
> - Beberapa varian board ESP32-C3 Mini punya pin default I2C atau strapping pin berbeda. Jika GPIO 8/9 dipakai board Anda untuk fungsi lain (mis. boot mode), sesuaikan `OLED_SDA` / `OLED_SCL` di kode dan pastikan tidak bentrok saat upload.
> - GPIO 0 dan GPIO 1 pada beberapa board ESP32-C3 mungkin berfungsi sebagai strapping pin saat boot. Jika tombol ditekan saat board dinyalakan, board bisa masuk mode download. Pastikan tombol tidak ditekan saat power-on/reset, atau pindahkan ke pin lain (sesuaikan `PB1_PIN` / `PB2_PIN` di kode).
> - Tombol manual memakai `INPUT_PULLUP`, jadi saat tombol ditekan pin akan menjadi LOW. Jika Anda menggunakan skema berbeda, sesuaikan pin dan logika di kode.
> - Relay dikonfigurasi **aktif LOW** secara default (`relayActiveLow = true`). Jika modul relay Anda aktif HIGH, ubah `relayActiveLow` menjadi `false` di kode.

## Instalasi Arduino IDE

### 1. Tambahkan Board Manager ESP32

`File > Preferences > Additional Board Manager URLs`, isi dengan:

```text
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```

Lalu buka `Tools > Board > Board Manager`, cari **esp32 by Espressif Systems**, install.

Pilih board: **ESP32C3 Dev Module** (atau varian sesuai board Anda, mis. "Super Mini ESP32C3").

### 2. Install Library (via Library Manager)

- **Blynk** (by Volodymyr Shymanskyy)
- **DHT sensor library** (by Adafruit)
- **Adafruit Unified Sensor** (dependency DHT library)
- **Adafruit GFX Library**
- **Adafruit SSD1306**

## Setup Project di Blynk

1. Buat akun di [Blynk.Console](https://blynk.cloud).
2. Buat **Template** baru, catat **Template ID** dan **Template Name**.
3. Tambahkan 3 **Datastream**:

   | Nama | Virtual Pin | Tipe Data | Keterangan |
   | --- | --- | --- | --- |
   | Suhu | V0 | Double, satuan °C | Read only dari device |
   | Kelembaban | V1 | Double, satuan % | Read only dari device |
   | Relay | V2 | Integer (0/1) | Switch, bisa ditulis dari app |

4. Buat **Device** baru dari template tersebut, salin **Auth Token**.
5. Di aplikasi Blynk (mobile), buat dashboard dengan widget:
   - **Gauge/Value Display** → bind ke V0 (Suhu)
   - **Gauge/Value Display** → bind ke V1 (Kelembaban)
   - **Switch/Button** → bind ke V2 (Relay)

## Konfigurasi Kode

Sebelum upload, ubah bagian berikut di `smart-room-sd.ino`:

```cpp
#define BLYNK_TEMPLATE_ID   "TMPLxxxxxxx"       // dari Blynk.Console
#define BLYNK_TEMPLATE_NAME "Smart Room SD"
#define BLYNK_AUTH_TOKEN    "ISI_AUTH_TOKEN_ANDA"
#define BLYNK_FIRMWARE_VERSION "0.1.0"

char ssid[33] = "NAMA_WIFI_ANDA";
char pass[65] = "PASSWORD_WIFI_ANDA";
```

> **Catatan tambahan:**
> - `relayActiveLow` diatur ke `true` secara default (cocok untuk mayoritas modul relay 1-channel murah). Jika relay Anda aktif HIGH, ubah menjadi `false`.
> - Pin I2C OLED default di GPIO 8 (SDA) dan GPIO 9 (SCL) dengan alamat `0x3C`. Jika OLED tidak terdeteksi, cek wiring atau ganti alamat sesuai modul Anda.
> - Transmit power WiFi di-set ke `WIFI_POWER_8_5dBm` untuk efisiensi daya. Bisa disesuaikan jika sinyal WiFi lemah.

## Cara Upload

1. Hubungkan ESP32-C3 Mini ke komputer via USB.
2. Pilih **Board**: ESP32C3 Dev Module, **Port** sesuai COM/tty yang muncul.
3. Klik **Upload**.
4. Buka **Serial Monitor** (baud rate `115200`) untuk melihat log koneksi WiFi/Blynk dan pembacaan sensor.
5. OLED akan menampilkan suhu, kelembaban, dan status relay secara berkala (update setiap 5 detik).
6. Jika tombol manual dipakai:
   - PB1 (GPIO 0) akan mengaktifkan relay
   - PB2 (GPIO 1) akan mematikannya
   - Status relay akan otomatis tersinkronisasi ke aplikasi Blynk
   - Tombol sudah dilengkapi debounce 50ms untuk mencegah false trigger
