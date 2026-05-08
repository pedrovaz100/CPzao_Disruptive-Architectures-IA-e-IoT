// =========================
// INCLUDES DE BIBLIOTECAS
// =========================
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "DHTesp.h"
#include <LiquidCrystal_I2C.h>
#include <uri/UriBraces.h>

// =========================
// CONFIGURACAO DO WIFI
// =========================
#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define WIFI_CHANNEL 6


// =========================
// PINOS
// =========================
const int PIN_LED1 = 26;
const int PIN_LED2 = 27;
const int PIN_DHT   = 18;
const int PIN_BTN1  = 33;
const int PIN_BTN2  = 25;

// =========================
// OBJETOS
// =========================
WebServer server(80);
DHTesp dht;
LiquidCrystal_I2C lcd(0x27, 16, 2);

// =========================
// VARIAVEIS
// =========================
bool led1State = false;
bool led2State = false;
int  lcdMode   = 0;   // 0=IP/Rede  1=DHT22  2=LEDs  3=Open-Meteo

float dhtTemp  = 0.0;
float dhtHumid = 0.0;
bool  dhtOk    = false;


float weatherTemp  = 0.0;
float weatherHumid = 0.0;
bool  weatherOk    = false;

unsigned long lastDhtRead = 0;
unsigned long lastBtn1    = 0;
unsigned long lastBtn2    = 0;
#define DHT_INTERVAL_MS  2000
#define DEBOUNCE_MS       300

// =========================
// URL DA API
// =========================
// Temperatura atual de Sao Paulo
 const char* WEATHER_URL = "https://api.open-meteo.com/v1/forecast?latitude=-23.55&longitude=-46.63&current=temperature_2m,relative_humidity_2m";

// =========================
// FUNCOES AUXILIARES
// =========================

// Leitura do sensor DHT22
void readDHT() {
  TempAndHumidity data = dht.getTempAndHumidity();
  if (dht.getStatus() == 0) {
    dhtTemp  = data.temperature;
    dhtHumid = data.humidity;
    dhtOk    = true;
  }
}

//configura pinos
void configPins() {
  // Pinos de saida
  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);

  // Pinos de entrada com pullup interno
  pinMode(PIN_BTN1, INPUT_PULLUP);
  pinMode(PIN_BTN2, INPUT_PULLUP);
}

// init lcd
void initLCD() {
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Inicializando...");
}

// conect wifi
void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);

  Serial.print("Conectando ao WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;

  http.begin(WEATHER_URL);
  int code = http.GET();

  if (code == HTTP_CODE_OK) {
    String payload = http.getString();
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (!err) {
      weatherTemp  = doc["current"]["temperature_2m"].as<float>();
      weatherHumid = doc["current"]["relative_humidity_2m"].as<float>();
      weatherOk    = true;
    }
  }
  http.end();
}

void updateLCD() {
  lcd.clear();
  switch (lcdMode) {
    case 0:
      lcd.setCursor(0, 0);
      lcd.print("IP:");
      lcd.setCursor(0, 1);
      lcd.print(WiFi.localIP().toString());
      break;

    case 1:
      lcd.setCursor(0, 0);
      if (dhtOk) {
        lcd.print("T: " + String(dhtTemp, 1) + " C");
        lcd.setCursor(0, 1);
        lcd.print("H: " + String(dhtHumid, 1) + " %");
      } else {
        lcd.print("DHT22: erro");
      }
      break;

    case 2:
      lcd.setCursor(0, 0);
      lcd.print("LED1: " + String(led1State ? "ON" : "OFF"));
      lcd.setCursor(0, 1);
      lcd.print("LED2: " + String(led2State ? "ON" : "OFF"));
      break;

    case 3:
      lcd.setCursor(0, 0);
      if (weatherOk) {
        lcd.print("T.ext:" + String(weatherTemp, 1));
        lcd.setCursor(0, 1);
        lcd.print("H.ext:" + String(weatherHumid, 1));
      } else {
        lcd.print("Clima: N/A");
        lcd.setCursor(0, 1);
        lcd.print("Pressione BTN1");
      }
      break;
  }
}

String buildErrorJson(const String& msg) {
  JsonDocument doc;
  doc["status"]  = "error";
  doc["message"] = msg;
  String out;
  serializeJson(doc, out);
  return out;
}

// =========================
// HANDLERS — API REST
// =========================

void handleSensorTempHum() {
  readDHT();
  JsonDocument doc;
  doc["source"] = "DHT22";
  if (dhtOk) {
    doc["temperature"] = dhtTemp;
    doc["humidity"]    = dhtHumid;
    doc["status"]      = "ok";
    String out; serializeJson(doc, out);
    server.send(200, "application/json", out);
  } else {
    doc["temperature"] = nullptr;
    doc["humidity"]    = nullptr;
    doc["status"]      = "error";
    doc["message"]     = "Falha na leitura do DHT22";
    String out; serializeJson(doc, out);
    server.send(503, "application/json", out);
  }
}

void handleLedGet() {
  String led = server.pathArg(0);
  int ledNum = led.toInt();

  if (ledNum != 1 && ledNum != 2) {
    server.send(404, "application/json", buildErrorJson("LED invalido"));
    return;
  }

  bool state;

  if (ledNum == 1) {
    state = led1State;
  } else {
    state = led2State;
  }

  JsonDocument doc;
  doc["led"]    = ledNum;
  doc["state"]  = state ? "on" : "off";
  doc["status"] = "ok";

  String out;
  serializeJson(doc, out);

  server.send(200, "application/json", out);
}

void handleLedPost() {
  String led = server.pathArg(0);
  int ledNum = led.toInt();

  if (ledNum != 1 && ledNum != 2) {
    server.send(404, "application/json", buildErrorJson("LED invalido"));
    return;
  }

  if (!server.hasArg("plain")) {
    server.send(400, "application/json", buildErrorJson("Body ausente"));
    return;
  }

  JsonDocument req;
  DeserializationError err = deserializeJson(req, server.arg("plain"));

  if (err || !req["state"].is<const char*>()) {
    server.send(400, "application/json", buildErrorJson("JSON invalido ou campo 'state' ausente"));
    return;
  }

  String newState = req["state"].as<String>();

  if (newState != "on" && newState != "off") {
    server.send(400, "application/json", buildErrorJson("Valor invalido. Use 'on' ou 'off'"));
    return;
  }

  bool state = (newState == "on");

  if (ledNum == 1) {
    led1State = state;
    digitalWrite(PIN_LED1, state ? HIGH : LOW);
  } else {
    led2State = state;
    digitalWrite(PIN_LED2, state ? HIGH : LOW);
  }

  updateLCD();

  JsonDocument res;
  res["led"]    = ledNum;
  res["state"]  = newState;
  res["status"] = "ok";

  String out;
  serializeJson(res, out);

  server.send(200, "application/json", out);
}

void handleWeather() {
  fetchWeather();
  JsonDocument doc;
  doc["source"] = "Open-Meteo";
  if (weatherOk) {
    doc["temperature"] = weatherTemp;
    doc["humidity"]    = weatherHumid;
    doc["status"]      = "ok";
    String out; serializeJson(doc, out);
    server.send(200, "application/json", out);
  } else {
    doc["temperature"] = nullptr;
    doc["humidity"]    = nullptr;
    doc["status"]      = "error";
    doc["message"]     = "Falha ao consultar Open-Meteo";
    String out; serializeJson(doc, out);
    server.send(503, "application/json", out);
  }
  updateLCD();
}

void handleStatus() {
  readDHT();
  JsonDocument doc;
  doc["ip"]   = WiFi.localIP().toString();
  doc["ssid"] = String(WIFI_SSID);

  doc["leds"]["led1"]["state"]  = led1State ? "on" : "off";
  doc["leds"]["led1"]["status"] = "ok";
  doc["leds"]["led2"]["state"]  = led2State ? "on" : "off";
  doc["leds"]["led2"]["status"] = "ok";

  doc["sensor"]["source"]      = "DHT22";
  doc["sensor"]["temperature"] = dhtOk ? dhtTemp : 0;
  doc["sensor"]["humidity"]    = dhtOk ? dhtHumid : 0;
  doc["sensor"]["status"]      = dhtOk ? "ok" : "error";

  doc["weather"]["source"]      = "Open-Meteo";
  doc["weather"]["temperature"] = weatherOk ? weatherTemp : 0;
  doc["weather"]["humidity"]    = weatherOk ? weatherHumid : 0;
  doc["weather"]["status"]      = weatherOk ? "ok" : "unavailable";

  doc["status"] = "ok";
  String out; serializeJson(doc, out);
  server.send(200, "application/json", out);
}

// =========================
// HANDLER — PAGINA WEB
// =========================

void sendHtml() {
  readDHT();

  String html = "<!DOCTYPE html><html lang='pt-br'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>Smart Home - CP3</title>";
  html += "<style>";
  html += "body{font-family:Arial,sans-serif;background:#1a1a2e;color:#eee;margin:0;padding:20px;}";
  html += "h1{color:#e94560;text-align:center;}";
  html += ".card{background:#16213e;border-radius:10px;padding:16px;margin:12px auto;max-width:500px;}";
  html += ".card h2{color:#aaa;font-size:.85rem;text-transform:uppercase;margin:0 0 10px;}";
  html += ".row{display:flex;justify-content:space-between;margin:6px 0;}";
  html += ".label{color:#aaa;} .val{font-weight:bold;}";
  html += ".on{color:#4ecca3;} .off{color:#e94560;}";
  html += "button{padding:8px 14px;border:none;border-radius:6px;cursor:pointer;margin:4px;font-size:.85rem;}";
  html += ".btn-on{background:#4ecca3;color:#000;} .btn-off{background:#e94560;color:#fff;}";
  html += ".btn-api{background:#0f3460;color:#fff;width:100%;}";
  html += ".footer{text-align:center;margin-top:20px;color:#555;font-size:.75rem;}";
  html += "</style></head><body>";

  html += "<h1>&#127968; Smart Home &mdash; CP3</h1>";

  // Card sistema
  html += "<div class='card'><h2>Sistema</h2>";
  html += "<div class='row'><span class='label'>IP</span><span class='val'>" + WiFi.localIP().toString() + "</span></div>";
  html += "<div class='row'><span class='label'>Rede</span><span class='val'>" + String(WIFI_SSID) + "</span></div>";
  html += "</div>";

  // Card sensor interno
  html += "<div class='card'><h2>Sensor Interno (DHT22)</h2>";
  html += "<div class='row'><span class='label'>Temperatura</span><span class='val'>";
  html += dhtOk ? String(dhtTemp, 1) + " &deg;C" : "<span class='off'>indisponivel</span>";
  html += "</span></div>";
  html += "<div class='row'><span class='label'>Umidade</span><span class='val'>";
  html += dhtOk ? String(dhtHumid, 1) + " %" : "<span class='off'>indisponivel</span>";
  html += "</span></div></div>";

  // Card iluminacao
  String s1 = led1State ? "on" : "off";
  String s2 = led2State ? "on" : "off";
  String s1Upper = s1;
  String s2Upper = s2;

  s1Upper.toUpperCase();
  s2Upper.toUpperCase();
  html += "<div class='card'><h2>Iluminacao</h2>";
  html += "<div class='row'><span class='label'>LED 1</span><span class='val " + s1 + "'>" + s1Upper + "</span></div>";
  html += "<div class='row'><span class='label'>LED 2</span><span class='val " + s2 + "'>" + s2Upper + "</span></div>";
  html += "<div style='text-align:center;margin-top:10px'>";
  html += "<button class='btn-on'  onclick=\"ctrl('/api/led/1','on')\">LED1 ON</button>";
  html += "<button class='btn-off' onclick=\"ctrl('/api/led/1','off')\">LED1 OFF</button>";
  html += "<button class='btn-on'  onclick=\"ctrl('/api/led/2','on')\">LED2 ON</button>";
  html += "<button class='btn-off' onclick=\"ctrl('/api/led/2','off')\">LED2 OFF</button>";
  html += "</div></div>";

  // Card clima externo
  html += "<div class='card'><h2>Clima Externo (Open-Meteo &mdash; S&atilde;o Paulo)</h2>";
  html += "<div class='row'><span class='label'>Temperatura</span><span class='val'>";
  html += weatherOk ? String(weatherTemp, 1) + " &deg;C" : "<span class='off'>aguardando consulta</span>";
  html += "</span></div>";
  html += "<div class='row'><span class='label'>Umidade</span><span class='val'>";
  html += weatherOk ? String(weatherHumid, 1) + " %" : "<span class='off'>aguardando consulta</span>";
  html += "</span></div>";
  html += "<div style='margin-top:10px'>";
  html += "<button class='btn-api' onclick=\"fetch('/api/weather').then(()=>location.reload())\">Atualizar Clima</button>";
  html += "</div></div>";

  html += "<div class='footer'>Disruptive Architecture IoT &mdash; CP3</div>";

  html += "<script>";
  html += "function ctrl(url,state){";
  html += "fetch(url,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({state:state})})";
  html += ".then(()=>location.reload());}";
  html += "</script>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

void handleNotFound() {
  server.send(404, "application/json", buildErrorJson("Rota nao encontrada"));
}

// =========================
// SETUP
// =========================
void setup() {
  Serial.begin(115200);

  // Configura pinos
  configPins();

  // DHT22
  dht.setup(PIN_DHT, DHTesp::DHT22);

  // LCD
  initLCD();

  // WiFi
  connectWiFi();

 // Rotas da API

  server.on("/", sendHtml);                   
  
  server.on("/api/weather", HTTP_GET, []() {
    handleWeather();
  });

  server.on("/api/status", HTTP_GET, []() {
    handleStatus();
  });
  
  server.on("/api/sensor/temphum", HTTP_GET, []() {
    handleSensorTempHum();
  });

  server.on(UriBraces("/api/led/{}"), HTTP_GET, []() {
    handleLedGet();
  });

  server.on(UriBraces("/api/led/{}"), HTTP_POST, []() {
    handleLedPost();
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("WebServer iniciado na porta 80.");
}

// =========================
// LOOP
// =========================
void loop() {
  server.handleClient();

  // Leitura periodica do DHT22 (a cada 2 segundos)
  unsigned long now = millis();
  if (now - lastDhtRead > DHT_INTERVAL_MS) {
    lastDhtRead = now;
    readDHT();
  }

  // BTN1 — consulta Open-Meteo e muda LCD para modo clima
  if (digitalRead(PIN_BTN1) == LOW) {
    if (now - lastBtn1 > DEBOUNCE_MS) {
      lastBtn1 = now;
      Serial.println("BTN1: consultando Open-Meteo...");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Consultando...");
      fetchWeather();
      lcdMode = 3;
      updateLCD();
    }
  }

  // BTN2 — alterna tela do LCD
  if (digitalRead(PIN_BTN2) == LOW) {
    if (now - lastBtn2 > DEBOUNCE_MS) {
      lastBtn2 = now;
      lcdMode = (lcdMode + 1) % 4;
      updateLCD();
      Serial.println("BTN2: LCD modo " + String(lcdMode));
    }
  }
}