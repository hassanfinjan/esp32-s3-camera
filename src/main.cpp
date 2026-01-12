#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <USB.h>
#include <USBHIDKeyboard.h>
#include <USBHIDConsumerControl.h>

const char* ssid = "Hasony";
const char* password = "Msyr8437";

USBHIDKeyboard Keyboard;
USBHIDConsumerControl ConsumerControl;
WebServer server(80);

#define LED_PIN 2
bool usbReady = false;

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<style>";
  html += "body{font-family:Arial;text-align:center;padding:20px;background:#1a1a1a;color:#fff;}";
  html += "h1{color:#00ff88;}";
  html += ".status{padding:10px;margin:20px;border-radius:5px;background:#333;}";
  html += "button{padding:15px 30px;margin:5px;font-size:16px;cursor:pointer;";
  html += "background:#00ff88;border:none;border-radius:5px;color:#000;font-weight:bold;}";
  html += "button:active{background:#00cc66;}";
  html += "input{padding:10px;width:250px;font-size:14px;border-radius:5px;border:2px solid #00ff88;background:#2a2a2a;color:#fff;}";
  html += ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:10px;max-width:800px;margin:20px auto;}";
  html += "</style></head><body>";
  html += "<h1>📱 ESP32-S3 Phone Control</h1>";
  
  if(usbReady) {
    html += "<div class='status' style='background:#004d00;'>✅ USB Connected</div>";
  } else {
    html += "<div class='status' style='background:#4d0000;'>⚠️ USB Not Ready - Connect phone via USB</div>";
  }
  
  html += "<div class='grid'>";
  html += "<button onclick=\"fetch('/type?text=Hello World')\">💬 Say Hello</button>";
  html += "<button onclick=\"fetch('/enter')\">↵ Enter</button>";
  html += "<button onclick=\"fetch('/space')\">␣ Space</button>";
  html += "<button onclick=\"fetch('/backspace')\">⌫ Backspace</button>";
  html += "<button onclick=\"fetch('/tab')\">⇥ Tab</button>";
  html += "<button onclick=\"fetch('/esc')\">⎋ Escape</button>";
  html += "</div>";
  
  html += "<h3>📊 Volume & Media</h3>";
  html += "<div class='grid'>";
  html += "<button onclick=\"fetch('/volume_up')\">🔊 Vol Up</button>";
  html += "<button onclick=\"fetch('/volume_down')\">🔉 Vol Down</button>";
  html += "<button onclick=\"fetch('/mute')\">🔇 Mute</button>";
  html += "<button onclick=\"fetch('/play_pause')\">⏯️ Play/Pause</button>";
  html += "<button onclick=\"fetch('/next_track')\">⏭️ Next</button>";
  html += "<button onclick=\"fetch('/prev_track')\">⏮️ Previous</button>";
  html += "</div>";
  
  html += "<h3>🔢 Quick Actions</h3>";
  html += "<div class='grid'>";
  html += "<button onclick=\"fetch('/arrow_up')\">⬆️ Up</button>";
  html += "<button onclick=\"fetch('/arrow_down')\">⬇️ Down</button>";
  html += "<button onclick=\"fetch('/arrow_left')\">⬅️ Left</button>";
  html += "<button onclick=\"fetch('/arrow_right')\">➡️ Right</button>";
  html += "</div>";
  
  html += "<h3>✍️ Custom Text</h3>";
  html += "<form onsubmit=\"event.preventDefault();fetch('/type?text='+encodeURIComponent(document.getElementById('txt').value));document.getElementById('txt').value='';\">";
  html += "<input id='txt' placeholder='Type anything...'>";
  html += "<button type='submit'>📤 Send</button></form>";
  
  html += "<br><br><p style='color:#666;'>IP: " + WiFi.localIP().toString() + "</p>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleType() {
  if(!usbReady) {
    server.send(503, "text/plain", "USB not ready");
    return;
  }
  if (server.hasArg("text")) {
    String text = server.arg("text");
    Keyboard.print(text);
    server.send(200, "text/plain", "Typed: " + text);
    Serial.println("Typed: " + text);
  } else {
    server.send(400, "text/plain", "Missing text");
  }
}

void sendKey(uint8_t key, const char* name) {
  if(!usbReady) {
    server.send(503, "text/plain", "USB not ready");
    return;
  }
  Keyboard.press(key);
  delay(50);
  Keyboard.releaseAll();
  server.send(200, "text/plain", String(name) + " pressed");
  Serial.println(String(name) + " pressed");
}

void sendConsumer(uint16_t code, const char* name) {
  if(!usbReady) {
    server.send(503, "text/plain", "USB not ready");
    return;
  }
  ConsumerControl.press(code);
  delay(50);
  ConsumerControl.release();
  server.send(200, "text/plain", String(name) + " pressed");
  Serial.println(String(name) + " pressed");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  
  Serial.println("\nESP32-S3 Phone Controller Starting...");
  
  // Initialize USB
  Keyboard.begin();
  ConsumerControl.begin();
  USB.begin();
  Serial.println("USB Keyboard initialized");
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("Control Panel: http://");
    Serial.println(WiFi.localIP());
    
    for(int i=0; i<3; i++) {
      digitalWrite(LED_PIN, LOW);
      delay(200);
      digitalWrite(LED_PIN, HIGH);
      delay(200);
    }
  }
  
  // Setup routes
  server.on("/", handleRoot);
  server.on("/type", handleType);
  server.on("/enter", []() { sendKey(KEY_RETURN, "Enter"); });
  server.on("/space", []() { sendKey(' ', "Space"); });
  server.on("/backspace", []() { sendKey(KEY_BACKSPACE, "Backspace"); });
  server.on("/tab", []() { sendKey(KEY_TAB, "Tab"); });
  server.on("/esc", []() { sendKey(KEY_ESC, "Escape"); });
  server.on("/arrow_up", []() { sendKey(KEY_UP_ARROW, "Up"); });
  server.on("/arrow_down", []() { sendKey(KEY_DOWN_ARROW, "Down"); });
  server.on("/arrow_left", []() { sendKey(KEY_LEFT_ARROW, "Left"); });
  server.on("/arrow_right", []() { sendKey(KEY_RIGHT_ARROW, "Right"); });
  
  server.on("/volume_up", []() { sendConsumer(CONSUMER_CONTROL_VOLUME_INCREMENT, "Volume Up"); });
  server.on("/volume_down", []() { sendConsumer(CONSUMER_CONTROL_VOLUME_DECREMENT, "Volume Down"); });
  server.on("/mute", []() { sendConsumer(CONSUMER_CONTROL_MUTE, "Mute"); });
  server.on("/play_pause", []() { sendConsumer(CONSUMER_CONTROL_PLAY_PAUSE, "Play/Pause"); });
  server.on("/next_track", []() { sendConsumer(CONSUMER_CONTROL_SCAN_NEXT, "Next Track"); });
  server.on("/prev_track", []() { sendConsumer(CONSUMER_CONTROL_SCAN_PREVIOUS, "Previous Track"); });
  
  server.onNotFound([]() {
    server.send(404, "text/plain", "Not Found");
  });
  
  server.begin();
  Serial.println("Web server started!");
  Serial.println("\nReady! Connect phone via USB and control from web interface.");
}

void loop() {
  server.handleClient();
  
  // Check USB status every 2 seconds
  static unsigned long lastCheck = 0;
  if(millis() - lastCheck > 2000) {
    usbReady = USB;
    lastCheck = millis();
  }
  
  delay(1);
}
