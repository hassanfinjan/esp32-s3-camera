#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// Create WiFi AP instead of connecting
const char* ap_ssid = "ESP32-Control";
const char* ap_password = "12345678";

WebServer server(80);
#define LED_PIN 2

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<style>";
  html += "body{font-family:Arial;text-align:center;padding:20px;background:#1a1a1a;color:#fff;}";
  html += "h1{color:#00ff88;}";
  html += "button{padding:20px;margin:10px;font-size:18px;background:#00ff88;";
  html += "border:none;border-radius:5px;color:#000;font-weight:bold;width:200px;}";
  html += "</style></head><body>";
  html += "<h1>🎮 ESP32 Control</h1>";
  html += "<button onclick=\"fetch('/led_on')\">💡 LED ON</button><br>";
  html += "<button onclick=\"fetch('/led_off')\">🌑 LED OFF</button><br>";
  html += "<button onclick=\"fetch('/blink')\">✨ BLINK</button>";
  html += "<p id='msg'></p>";
  html += "<script>function show(t){document.getElementById('msg').innerText=t;}</script>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  
  Serial.println("\nESP32-S3 AP Mode");
  
  // Create Access Point
  WiFi.softAP(ap_ssid, ap_password);
  Serial.print("AP IP: http://");
  Serial.println(WiFi.softAPIP());
  
  server.on("/", handleRoot);
  server.on("/led_on", []() {
    digitalWrite(LED_PIN, HIGH);
    server.send(200, "text/plain", "LED ON");
  });
  server.on("/led_off", []() {
    digitalWrite(LED_PIN, LOW);
    server.send(200, "text/plain", "LED OFF");
  });
  server.on("/blink", []() {
    for(int i=0; i<5; i++) {
      digitalWrite(LED_PIN, LOW);
      delay(200);
      digitalWrite(LED_PIN, HIGH);
      delay(200);
    }
    server.send(200, "text/plain", "Blinked!");
  });
  
  server.begin();
  Serial.println("Ready!");
}

void loop() {
  server.handleClient();
}
