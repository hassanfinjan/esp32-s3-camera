#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <Adafruit_NeoPixel.h>
#include <NimBLEDevice.h>

// --- Config ---
const char* ssid = "ESP32-S3-ULTIMATE";
IPAddress local_IP(192, 168, 0, 141);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

#define LED_PIN 2
#define RGB_PIN 48
Adafruit_NeoPixel pixel(1, RGB_PIN, NEO_GRB + NEO_KHZ800);
AsyncWebServer server(80);

// --- Storage Helper ---
String listFiles() {
  String list = "<ul>";
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  while(file){
    list += "<li>" + String(file.name()) + " (" + String(file.size()) + " bytes)</li>";
    file = root.openNextFile();
  }
  list += "</ul>";
  return list;
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pixel.begin();
  
  if(!LittleFS.begin(true)) Serial.println("LittleFS Mount Failed");

  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, "password123");
  BLEDevice::init("S3_SCANNER");

  // --- HTML DASHBOARD ---
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = "<html><head><style>body{font-family:sans-serif;background:#222;color:#eee;padding:20px;}"
                  ".card{background:#333;padding:15px;margin-bottom:10px;border-radius:8px;}"
                  "button{padding:10px;margin:5px;cursor:pointer;}</style></head><body>"
                  "<h1>ESP32-S3 Master Controller</h1>"
                  "<div class='card'><h3>Hardware Control</h3>"
                  "<button onclick=\"fetch('/led/toggle')\">Toggle Blue LED (GPIO 2)</button>"
                  "<button onclick=\"fetch('/rgb/cycle')\">Cycle RGB (GPIO 48)</button></div>"
                  "<div class='card'><h3>System Sensors</h3>"
                  "<p>Internal Temp: " + String((temprature_sens_read() - 32) / 1.8) + " C</p>"
                  "<p>Heap Free: " + String(ESP.getFreeHeap()/1024) + " KB</p></div>"
                  "<div class='card'><h3>Filesystem</h3>" + listFiles() + "</div>"
                  "<div class='card'><h3>Radio</h3>"
                  "<button onclick=\"location.href='/scan/wifi'\">WiFi Scan</button>"
                  "<button onclick=\"location.href='/scan/ble'\">BLE Scan</button></div>"
                  "</body></html>";
    request->send(200, "text/html", html);
  });

  // LED Toggle Logic
  server.on("/led/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    request->send(200, "text/plain", "OK");
  });

  // RGB Cycle Logic
  server.on("/rgb/cycle", HTTP_GET, [](AsyncWebServerRequest *request){
    pixel.setPixelColor(0, pixel.Color(random(255), random(255), random(255)));
    pixel.show();
    request->send(200, "text/plain", "OK");
  });

  // WiFi Scan (Hardware Radio Task)
  server.on("/scan/wifi", HTTP_GET, [](AsyncWebServerRequest *request){
    int n = WiFi.scanNetworks();
    String results = "Found: " + String(n) + "<br>";
    for(int i=0; i<n; i++) results += WiFi.SSID(i) + " (" + WiFi.RSSI(i) + ")<br>";
    request->send(200, "text/html", results + "<br><a href='/'>Back</a>");
  });

  // File Upload Handler
  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Upload Success");
  }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
    if(!index){
      request->_tempFile = LittleFS.open("/" + filename, "w");
    }
    if(len){
      request->_tempFile.write(data, len);
    }
    if(final){
      request->_tempFile.close();
    }
  });

  server.begin();
}

void loop() {
  // Main loop remains clear for async response
}
