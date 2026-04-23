#include <WiFi.h>
#include <WebServer.h>

WebServer server(80);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("Starting AP...");
  
  WiFi.mode(WIFI_AP);
  bool success = WiFi.softAP("GMpro", "Sangkur87");
  
  Serial.println("AP Result: " + String(success ? "OK" : "FAILED"));
  Serial.println("IP: " + WiFi.softAPIP().toString());
  
  server.on("/", []() {
    server.send(200, "text/html", "<html><body><h1>NETHER-X OK!</h1><p>AP: GMpro | Pass: Sangkur87</p></body></html>");
  });
  server.begin();
  
  Serial.println("Server started");
}

void loop() {
  server.handleClient();
}