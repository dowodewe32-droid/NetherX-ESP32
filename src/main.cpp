#include <WiFi.h>
#include <WebServer.h>

WebServer server(80);

void setup() {
  Serial.begin(115200);
  delay(500);
  
  Serial.println("");
  Serial.println("===== NETHER-X =====");
  
  WiFi.disconnect(true);
  delay(100);
  
  WiFi.mode(WIFI_AP);
  delay(100);
  
  boolean APstarted = WiFi.softAP("GMpro", "Sangkur87");
  
  delay(500);
  
  Serial.println("AP Status: " + String(APstarted));
  Serial.println("SSID: GMpro");
  Serial.println("PASS: Sangkur87");
  Serial.println("IP: " + WiFi.softAPIP().toString());
  Serial.println("MAC: " + WiFi.softAPmacAddress());
  Serial.println("====================");
  
  delay(500);
  
  server.on("/", []() {
    String html = "<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>NETHER-X</title></head>";
    html += "<body style='background:#000;color:#0f0;font-family:monospace;padding:20px'>";
    html += "<h1>NETHER-X v1.0</h1>";
    html += "<p>Status: ONLINE</p>";
    html += "<p>SSID: GMpro | Pass: Sangkur87</p>";
    html += "<p>IP: 192.168.4.1</p>";
    html += "<p>MAC: " + WiFi.softAPmacAddress() + "</p>";
    html += "</body></html>";
    server.send(200, "text/html", html);
  });
  
  server.begin();
  Serial.println("Web Server Started");
}

void loop() {
  server.handleClient();
  delay(1);
}