# espphish

This repository contains several ESP8266 sketches demonstrating how a captive portal (Wi-Fi AP + DNS redirection + simple web forms) works.

IMPORTANT: These examples are provided strictly for educational and defensive testing purposes (e.g., security training, awareness, lab simulations). Do not deploy in public networks or use to collect, store, transmit, or otherwise handle real user data. You are solely responsible for complying with applicable laws and policies.

- For learners and testers: Only use in an isolated lab with hardware you own.
- For end users: Do not enter real credentials or phone numbers into demo pages.

## Projects in this repo
- InstagramLoginCaptivePortal:
  ESP8266 AP + DNS captive portal serving an Instagram-like login page. Submitted username/password are written to on-device LittleFS for demonstration. Endpoints: `/logs`, `/clear`.
- PhoneVerificationPortal_UZ:
  Uzbek UI flow that asks for a phone number and a short verification code; both are written to LittleFS. Endpoints: `/logs`, `/clear`.
- PhoneVerificationPortal_EN:
  English variant of the phone/code flow; writes to LittleFS. Endpoints: `/logs`, `/clear`.
- PhoneVerificationPortal_Simple_EN:
  Simpler English phone/code variant; writes to LittleFS.

Again: these capture inputs locally on the device. They are demos to show what a captive portal can technically do, not for real-world use.

## Requirements
- Arduino IDE (or Arduino CLI)
- ESP8266 core for Arduino (e.g., 3.x)
- Libraries (install via Library Manager or from source):
  - ESPAsyncWebServer
  - ESPAsyncTCP (ESP8266)
  - DNSServer (bundled with ESP8266 core)
  - LittleFS (bundled with ESP8266 core)

## How to build/run (Arduino IDE)
1. Open one project folder (e.g., `InstagramLoginCaptivePortal/InstagramLoginCaptivePortal.ino`).
2. Tools → Board: select an ESP8266 board (e.g., NodeMCU 1.0 / Wemos D1 mini).
3. Connect the board and select the correct port.
4. Upload.
5. The device creates an AP (e.g., `Bepul_WiFi` or the sketch-defined SSID) and a DNS server that redirects client requests to the captive page.

## Demo endpoints
- `/` serves the page for the given sketch.
- `/logs` shows submitted inputs stored on LittleFS.
- `/clear` clears the on-device log file.

## Ethics and legal notice
- Do not perform credential harvesting.
- Do not deploy on networks you do not own or manage.
- Comply with all applicable laws, platform policies, and terms of service.

## Repository structure
- InstagramLoginCaptivePortal/InstagramLoginCaptivePortal.ino
- PhoneVerificationPortal_UZ/PhoneVerificationPortal_UZ.ino
- PhoneVerificationPortal_EN/PhoneVerificationPortal_EN.ino
- PhoneVerificationPortal_Simple_EN/PhoneVerificationPortal_Simple_EN.ino

## License
MIT License (see LICENSE)
