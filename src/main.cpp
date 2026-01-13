#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <Adafruit_NeoPixel.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// AP Configuration
const char* ap_ssid = "ESP32-S3";
const char* ap_password = "12345678";

WebServer server(80);

// Pin Definitions (from your pinout)
#define LED_PIN 2        // LED_ON
#define RGB_PIN 48       // WS2812
#define RGB_COUNT 1      // Single RGB LED

// NeoPixel setup
Adafruit_NeoPixel strip(RGB_COUNT, RGB_PIN, NEO_GRB + NEO_KHZ800);

// BLE Scan
BLEScan* pBLEScan;
std::vector<String> bleDevices;

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    String device = String(advertisedDevice.toString().c_str());
    bleDevices.push_back(device);
  }
};

// State variables
bool ledState = false;
uint8_t rgbR = 0, rgbG = 0, rgbB = 0;

const char* HTML_HEADER = R"(
<!DOCTYPE html>
<html>
<head>
  <meta name='viewport' content='width=device-width,initial-scale=1'>
  <title>ESP32-S3 Control Center</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { 
      font-family: 'Segoe UI', Tahoma, sans-serif;
      background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
      color: #fff;
      padding: 20px;
      min-height: 100vh;
    }
    .container { max-width: 1200px; margin: 0 auto; }
    h1 { 
      text-align: center;
      color: #00d9ff;
      margin-bottom: 30px;
      font-size: 2.5em;
      text-shadow: 0 0 20px rgba(0,217,255,0.5);
    }
    .grid { 
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
      gap: 20px;
      margin-bottom: 20px;
    }
    .card {
      background: rgba(255,255,255,0.05);
      border: 1px solid rgba(0,217,255,0.2);
      border-radius: 15px;
      padding: 20px;
      backdrop-filter: blur(10px);
    }
    .card h2 {
      color: #00d9ff;
      margin-bottom: 15px;
      font-size: 1.5em;
    }
    button {
      width: 100%;
      padding: 15px;
      margin: 5px 0;
      border: none;
      border-radius: 10px;
      font-size: 16px;
      font-weight: bold;
      cursor: pointer;
      transition: all 0.3s;
      background: linear-gradient(135deg, #00d9ff 0%, #0099cc 100%);
      color: #000;
    }
    button:hover {
      transform: translateY(-2px);
      box-shadow: 0 5px 20px rgba(0,217,255,0.4);
    }
    button.danger {
      background: linear-gradient(135deg, #ff0055 0%, #cc0044 100%);
      color: #fff;
    }
    button.success {
      background: linear-gradient(135deg, #00ff88 0%, #00cc66 100%);
    }
    input[type="range"], input[type="color"] {
      width: 100%;
      padding: 10px;
      margin: 5px 0;
      border: 2px solid rgba(0,217,255,0.3);
      border-radius: 8px;
      background: rgba(255,255,255,0.1);
      color: #fff;
      font-size: 14px;
    }
    .status {
      padding: 10px;
      margin: 10px 0;
      border-radius: 8px;
      text-align: center;
      font-weight: bold;
    }
    .status.on { background: rgba(0,255,136,0.2); color: #00ff88; }
    .status.off { background: rgba(255,0,85,0.2); color: #ff0055; }
    #scanResults {
      max-height: 300px;
      overflow-y: auto;
      background: rgba(0,0,0,0.3);
      border-radius: 8px;
      padding: 10px;
      margin-top: 10px;
    }
    .scan-item {
      padding: 10px;
      margin: 5px 0;
      background: rgba(0,217,255,0.1);
      border-radius: 5px;
      border-left: 3px solid #00d9ff;
      font-size: 12px;
    }
    .file-item {
      padding: 8px;
      margin: 5px 0;
      background: rgba(0,217,255,0.05);
      border-radius: 5px;
      display: flex;
      justify-content: space-between;
      align-items: center;
    }
    .slider-container {
      margin: 15px 0;
    }
    .slider-label {
      display: flex;
      justify-content: space-between;
      margin-bottom: 5px;
      color: #00d9ff;
    }
    .color-preview {
      width: 100%;
      height: 50px;
      border-radius: 8px;
      margin: 10px 0;
      border: 2px solid rgba(0,217,255,0.3);
    }
    .loading {
      display: inline-block;
      width: 20px;
      height: 20px;
      border: 3px solid rgba(0,217,255,0.3);
      border-top: 3px solid #00d9ff;
      border-radius: 50%;
      animation: spin 1s linear infinite;
    }
    @keyframes spin {
      0% { transform: rotate(0deg); }
      100% { transform: rotate(360deg); }
    }
  </style>
</head>
<body>
  <div class='container'>
    <h1>🎮 ESP32-S3 Control Center</h1>
)";

const char* HTML_FOOTER = R"(
  </div>
  <script>
    function cmd(url, callback) {
      fetch(url)
        .then(r => r.text())
        .then(data => {
          if(callback) callback(data);
        })
        .catch(e => alert('Error: ' + e));
    }
    
    function updateLED(state) {
      cmd('/led?state=' + state, () => {
        document.getElementById('ledStatus').className = 'status ' + (state === '1' ? 'on' : 'off');
        document.getElementById('ledStatus').innerText = state === '1' ? '✓ LED ON' : '✗ LED OFF';
      });
    }
    
    function updateRGB() {
      const r = document.getElementById('rgbR').value;
      const g = document.getElementById('rgbG').value;
      const b = document.getElementById('rgbB').value;
      cmd(`/rgb?r=${r}&g=${g}&b=${b}`, () => {
        document.getElementById('colorPreview').style.background = `rgb(${r},${g},${b})`;
        document.getElementById('rgbStatus').className = 'status on';
        document.getElementById('rgbStatus').innerText = `RGB: ${r},${g},${b}`;
      });
    }
    
    function rgbOff() {
      cmd('/rgb?r=0&g=0&b=0', () => {
        document.getElementById('rgbR').value = 0;
        document.getElementById('rgbG').value = 0;
        document.getElementById('rgbB').value = 0;
        document.getElementById('colorPreview').style.background = 'rgb(0,0,0)';
        document.getElementById('rgbStatus').className = 'status off';
        document.getElementById('rgbStatus').innerText = 'RGB OFF';
      });
    }
    
    function scanWiFi() {
      document.getElementById('wifiResults').innerHTML = '<div class="loading"></div> Scanning...';
      cmd('/scan_wifi', data => {
        document.getElementById('wifiResults').innerHTML = data;
      });
    }
    
    function scanBLE() {
      document.getElementById('bleResults').innerHTML = '<div class="loading"></div> Scanning BLE devices...';
      cmd('/scan_ble', data => {
        document.getElementById('bleResults').innerHTML = data;
      });
    }
    
    function listFiles() {
      cmd('/files', data => {
        document.getElementById('fileList').innerHTML = data;
      });
    }
    
    function restart() {
      if(confirm('Restart ESP32-S3?')) {
        cmd('/restart');
        setTimeout(() => location.reload(), 3000);
      }
    }
    
    ['rgbR','rgbG','rgbB'].forEach(id => {
      document.getElementById(id).oninput = function() {
        document.getElementById(id+'Val').innerText = this.value;
      };
    });
    
    listFiles();
  </script>
</body>
</html>
)";

void handleRoot() {
  String html = HTML_HEADER;
  
  html += R"(
    <div class='grid'>
      <div class='card'>
        <h2>💡 LED Control</h2>
        <div id='ledStatus' class='status off'>LED OFF</div>
        <button onclick='updateLED("1")'>Turn ON</button>
        <button class='danger' onclick='updateLED("0")'>Turn OFF</button>
      </div>
      
      <div class='card'>
        <h2>🌈 WS2812 RGB LED</h2>
        <div id='rgbStatus' class='status off'>RGB OFF</div>
        <div class='color-preview' id='colorPreview' style='background:rgb(0,0,0)'></div>
        <div class='slider-container'>
          <div class='slider-label'><span>Red</span><span id='rgbRVal'>0</span></div>
          <input type='range' id='rgbR' min='0' max='255' value='0'>
        </div>
        <div class='slider-container'>
          <div class='slider-label'><span>Green</span><span id='rgbGVal'>0</span></div>
          <input type='range' id='rgbG' min='0' max='255' value='0'>
        </div>
        <div class='slider-container'>
          <div class='slider-label'><span>Blue</span><span id='rgbBVal'>0</span></div>
          <input type='range' id='rgbB' min='0' max='255' value='0'>
        </div>
        <button class='success' onclick='updateRGB()'>Apply</button>
        <button class='danger' onclick='rgbOff()'>OFF</button>
      </div>
      
      <div class='card'>
        <h2>📡 WiFi Scanner</h2>
        <button onclick='scanWiFi()'>🔍 Scan Networks</button>
        <div id='wifiResults'></div>
      </div>
      
      <div class='card'>
        <h2>📶 BLE Scanner</h2>
        <button onclick='scanBLE()'>🔍 Scan BLE</button>
        <div id='bleResults'></div>
      </div>
      
      <div class='card'>
        <h2>📁 File System</h2>
        <button onclick='listFiles()'>🔄 Refresh</button>
        <div id='fileList'></div>
      </div>
      
      <div class='card'>
        <h2>⚙️ System</h2>
        <div class='status on'>Heap: )" + String(ESP.getFreeHeap()/1024) + R"( KB</div>
        <div class='status on'>PSRAM: )" + String(ESP.getPsramSize()/1024) + R"( KB</div>
        <button onclick='restart()'>🔄 Restart</button>
        <button class='danger' onclick='if(confirm("Format?")) cmd("/format")'>⚠️ Format</button>
      </div>
    </div>
  )";
  
  html += HTML_FOOTER;
  server.send(200, "text/html", html);
}

void handleLED() {
  if(server.hasArg("state")) {
    ledState = server.arg("state") == "1";
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    server.send(200, "text/plain", ledState ? "ON" : "OFF");
  }
}

void handleRGB() {
  if(server.hasArg("r") && server.hasArg("g") && server.hasArg("b")) {
    rgbR = server.arg("r").toInt();
    rgbG = server.arg("g").toInt();
    rgbB = server.arg("b").toInt();
    
    strip.setPixelColor(0, strip.Color(rgbR, rgbG, rgbB));
    strip.show();
    
    server.send(200, "text/plain", "OK");
  }
}

void handleWiFiScan() {
  String html = "";
  int n = WiFi.scanNetworks();
  
  if(n == 0) {
    html = "<div class='scan-item'>No networks found</div>";
  } else {
    for(int i = 0; i < n; i++) {
      html += "<div class='scan-item'>";
      html += "<strong>" + WiFi.SSID(i) + "</strong><br>";
      html += "RSSI: " + String(WiFi.RSSI(i)) + " dBm | Ch: " + String(WiFi.channel(i));
      html += " | " + String((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Secure");
      html += "</div>";
    }
  }
  
  server.send(200, "text/html", html);
}

void handleBLEScan() {
  String html = "";
  bleDevices.clear();
  
  BLEScanResults foundDevices = pBLEScan->start(5, false);
  
  if(bleDevices.size() == 0) {
    html = "<div class='scan-item'>No BLE devices found</div>";
  } else {
    for(int i = 0; i < bleDevices.size(); i++) {
      html += "<div class='scan-item'>" + bleDevices[i] + "</div>";
    }
  }
  
  pBLEScan->clearResults();
  server.send(200, "text/html", html);
}

void handleFileList() {
  String html = "";
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  
  int count = 0;
  while(file) {
    html += "<div class='file-item'>";
    html += "<span>📄 " + String(file.name()) + "</span>";
    html += "<span>" + String(file.size()) + " B</span>";
    html += "</div>";
    file = root.openNextFile();
    count++;
  }
  
  if(count == 0) {
    html = "<div class='file-item'>No files</div>";
  }
  
  server.send(200, "text/html", html);
}

void handleFormat() {
  SPIFFS.format();
  server.send(200, "text/plain", "Formatted");
}

void handleRestart() {
  server.send(200, "text/plain", "Restarting...");
  delay(1000);
  ESP.restart();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\nESP32-S3 Control Center Starting...");
  
  // Setup pins
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Initialize NeoPixel
  strip.begin();
  strip.show(); // Initialize to OFF
  strip.setBrightness(50); // 50/255 brightness
  
  // Initialize BLE
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
  
  // Initialize SPIFFS
  if(!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Failed");
  }
  
  // Create AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);
  
  Serial.print("AP IP: http://");
  Serial.println(WiFi.softAPIP());
  
  // Routes
  server.on("/", handleRoot);
  server.on("/led", handleLED);
  server.on("/rgb", handleRGB);
  server.on("/scan_wifi", handleWiFiScan);
  server.on("/scan_ble", handleBLEScan);
  server.on("/files", handleFileList);
  server.on("/format", handleFormat);
  server.on("/restart", handleRestart);
  
  server.begin();
  Serial.println("Ready! Connect to: ESP32-S3 (12345678)");
  Serial.println("Open: http://192.168.4.1");
  
  // Startup animation
  for(int i=0; i<255; i+=5) {
    strip.setPixelColor(0, strip.Color(0, i, 255-i));
    strip.show();
    delay(10);
  }
  strip.setPixelColor(0, 0);
  strip.show();
}

void loop() {
  server.handleClient();
}
