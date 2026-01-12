#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <USB.h>
#include <USBHIDKeyboard.h>

const char* ssid = "Hasony";
const char* password = "Msyr8437";

USBHIDKeyboard Keyboard;
WebServer server(80);

#define LED_PIN 2

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<style>body{font-family:Arial;text-align:center;padding:20px;}";
  html += "button{padding:15px 30px;margin:10px;font-size:18px;cursor:pointer;}</style>";
  html += "</head><body>";
  html += "<h1>ESP32-S3 Phone Controller</h1>";
  html += "<h3>Control your phone via USB!</h3>";
  html += "<button onclick=\"fetch('/type?text=Hello')\">Type 'Hello'</button><br>";
  html += "<button onclick=\"fetch('/home')\">Home Button</button><br>";
  html += "<button onclick=\"fetch('/back')\">Back Button</button><br>";
  html += "<button onclick=\"fetch('/menu')\">Menu</button><br>";
  html += "<button onclick=\"fetch('/volume_up')\">Volume Up</button><br>";
  html += "<button onclick=\"fetch('/volume_down')\">Volume Down</button><br>";
  html += "<form onsubmit=\"event.preventDefault();fetch('/type?text='+document.getElementById('txt').value);\">";
  html += "<input id='txt' placeholder='Type anything...' style='padding:10px;width:200px;'>";
  html += "<button type='submit'>Send</button></form>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleType() {
  if (server.hasArg("text")) {
    String text = server.arg("text");
    Keyboard.print(text);
    server.send(200, "text/plain", "Typed: " + text);
    Serial.println("Typed: " + text);
  } else {
    server.send(400, "text/plain", "Missing text parameter");
  }
}

void handleHome() {
  Keyboard.press(KEY_ESC);
  delay(100);
  Keyboard.releaseAll();
  server.send(200, "text/plain", "Home pressed");
}

void handleBack() {
  Keyboard.press(KEY_BACKSPACE);
  delay(100);
  Keyboard.releaseAll();
  server.send(200, "text/plain", "Back pressed");
}

void handleMenu() {
  Keyboard.press(KEY_TAB);
  delay(100);
  Keyboard.releaseAll();
  server.send(200, "text/plain", "Menu pressed");
}

void handleVolumeUp() {
  Keyboard.press(KEY_UP_ARROW);
  delay(100);
  Keyboard.releaseAll();
  server.send(200, "text/plain", "Volume Up");
}

void handleVolumeDown() {
  Keyboard.press(KEY_DOWN_ARROW);
  delay(100);
  Keyboard.releaseAll();
  server.send(200, "text/plain", "Volume Down");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  
  Serial.println("\nESP32-S3 Phone Controller Starting...");
  
  // Initialize USB Keyboard
  Keyboard.begin();
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
    
    // Blink LED to show success
    for(int i=0; i<3; i++) {
      digitalWrite(LED_PIN, LOW);
      delay(200);
      digitalWrite(LED_PIN, HIGH);
      delay(200);
    }
  } else {
    Serial.println("\nWiFi Failed!");
  }
  
  // Setup web server
  server.on("/", handleRoot);
  server.on("/type", handleType);
  server.on("/home", handleHome);
  server.on("/back", handleBack);
  server.on("/menu", handleMenu);
  server.on("/volume_up", handleVolumeUp);
  server.on("/volume_down", handleVolumeDown);
  
  server.begin();
  Serial.println("Web server started!");
  Serial.println("\nReady! Connect phone via USB and control from web interface.");
}

void loop() {
  server.handleClient();
}
