#include <WiFi.h>
#include <WebServer.h>
#include <time.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include "DHT.h"

// ================= LCD I2C =================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================= WIFI WOKWI =================
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// ================= SITE / SERVIDOR =================
WebServer server(80);

// ================= NTP =================
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -3 * 3600;
const int daylightOffset_sec = 0;

// ================= PINOS =================
const int botaoAbrir      = 33;
const int botaoFechar     = 25;
const int buzzer          = 32;
const int ledPortaAberta  = 26;
const int ledPortaFechada = 27;
const int servoPorta      = 19;

// ================= SERVO =================
Servo porta;
const int ANGULO_FECHADO = 0;
const int ANGULO_ABERTO  = 90;

// ================= DHT22 =================
#define DHTPIN 18
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// ================= HORÁRIOS AUTOMÁTICOS =================
const int HORA_REFEICAO[3][2] = {
  {7, 0},
  {12, 0},
  {18, 0}
};

// ================= TEMPOS =================
const unsigned long DEBOUNCE_MS   = 300;
const unsigned long AVISO_DURACAO = 5000;
const unsigned long PORTA_TEMPO   = 10000;

// ================= VARIÁVEIS =================
unsigned long ultimoPressAbrir  = 0;
unsigned long ultimoPressFechar = 0;
unsigned long tempoAvisoInicio  = 0;
unsigned long tempoPortaAbriu   = 0;
unsigned long ultimaAtualizacao = 0;

bool portaAberta = false;
bool mostrandoAviso = false;
bool refeicaoDisp[3] = {true, true, true};
String ultimoEvento = "Sistema iniciado";

// ================= PROTÓTIPOS =================
void conectarWiFi();
void configurarNTP();
void configurarSite();
void verificarBotoes(unsigned long ms, bool tempoValido, struct tm timeinfo);
void verificarHorarios(struct tm timeinfo);
void abrirPorta(unsigned long ms, String origem = "Manual");
void fecharPorta(struct tm timeinfo, bool horaValida, String origem = "Manual");
void atualizarDisplay(struct tm* timeinfo);
String horaAtualTexto();
String temperaturaTexto();
String paginaHTML();
void tocarAbertura();
void tocarFechamento();

void setup() {
  Serial.begin(115200);

  pinMode(botaoAbrir, INPUT_PULLUP);
  pinMode(botaoFechar, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);
  pinMode(ledPortaAberta, OUTPUT);
  pinMode(ledPortaFechada, OUTPUT);

  porta.setPeriodHertz(50);
  porta.attach(servoPorta, 500, 2400);
  porta.write(ANGULO_FECHADO);

  digitalWrite(ledPortaAberta, LOW);
  digitalWrite(ledPortaFechada, HIGH);

  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  dht.begin();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pet Feeder");
  lcd.setCursor(0, 1);
  lcd.print("Iniciando...");

  conectarWiFi();
  configurarNTP();
  configurarSite();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Site pronto:");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());

  Serial.println("Sistema iniciado!");
}

void loop() {
  server.handleClient();

  struct tm timeinfo;
  bool tempoValido = getLocalTime(&timeinfo, 10);
  unsigned long ms = millis();

  if (portaAberta && (ms - tempoPortaAbriu >= PORTA_TEMPO)) {
    fecharPorta(timeinfo, tempoValido, "Automatico");
  }

  if (tempoValido) {
    verificarHorarios(timeinfo);
  }

  verificarBotoes(ms, tempoValido, timeinfo);

  if (ms - ultimaAtualizacao >= 1000) {
    atualizarDisplay(tempoValido ? &timeinfo : nullptr);
    ultimaAtualizacao = ms;
  }
}

void conectarWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Conectando WiFi");

  Serial.print("Conectando ao WiFi");
  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 40) {
    delay(250);
    Serial.print(".");
    lcd.setCursor(tentativas % 16, 1);
    lcd.print(".");
    tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi conectado!");
    Serial.print("Abra o site no navegador: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi falhou. Os botoes fisicos ainda funcionam.");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi falhou");
    lcd.setCursor(0, 1);
    lcd.print("Botoes OK");
    delay(1500);
  }
}

void configurarNTP() {
  if (WiFi.status() != WL_CONNECTED) return;

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sincronizando");
  lcd.setCursor(0, 1);
  lcd.print("horario...");

  struct tm timeinfo;
  int tentativas = 0;
  while (!getLocalTime(&timeinfo, 200) && tentativas < 20) {
    Serial.print("#");
    tentativas++;
  }

  if (getLocalTime(&timeinfo, 200)) {
    Serial.printf("\nNTP OK: %02d:%02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  } else {
    Serial.println("\nNTP falhou, mas o projeto continua funcionando.");
  }
}

void configurarSite() {
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", paginaHTML());
  });

  server.on("/abrir", HTTP_GET, []() {
    if (!portaAberta) abrirPorta(millis(), "Site");
    server.sendHeader("Location", "/");
    server.send(303, "text/plain", "");
  });

  server.on("/fechar", HTTP_GET, []() {
    struct tm timeinfo;
    bool tempoValido = getLocalTime(&timeinfo, 10);
    if (portaAberta) fecharPorta(timeinfo, tempoValido, "Site");
    server.sendHeader("Location", "/");
    server.send(303, "text/plain", "");
  });

  server.on("/status", HTTP_GET, []() {
    String json = "{";
    json += "\"porta\":\"" + String(portaAberta ? "ABERTA" : "FECHADA") + "\",";
    json += "\"hora\":\"" + horaAtualTexto() + "\",";
    json += "\"temperatura\":\"" + temperaturaTexto() + "\",";
    json += "\"evento\":\"" + ultimoEvento + "\"";
    json += "}";
    server.send(200, "application/json", json);
  });

  server.begin();
  Serial.println("Servidor web iniciado.");
}

void verificarBotoes(unsigned long ms, bool tempoValido, struct tm timeinfo) {
  if (digitalRead(botaoAbrir) == LOW && !portaAberta && (ms - ultimoPressAbrir > DEBOUNCE_MS)) {
    ultimoPressAbrir = ms;
    abrirPorta(ms, "Botao");
  }

  if (digitalRead(botaoFechar) == LOW && portaAberta && (ms - ultimoPressFechar > DEBOUNCE_MS)) {
    ultimoPressFechar = ms;
    fecharPorta(timeinfo, tempoValido, "Botao");
  }
}

void verificarHorarios(struct tm timeinfo) {
  for (int i = 0; i < 3; i++) {
    bool foraDoHorario = timeinfo.tm_hour != HORA_REFEICAO[i][0] || timeinfo.tm_min != HORA_REFEICAO[i][1];
    if (foraDoHorario) refeicaoDisp[i] = true;
  }

  for (int i = 0; i < 3; i++) {
    bool horarioCerto = timeinfo.tm_hour == HORA_REFEICAO[i][0] &&
                        timeinfo.tm_min == HORA_REFEICAO[i][1] &&
                        timeinfo.tm_sec <= 5;

    if (horarioCerto && refeicaoDisp[i] && !portaAberta) {
      refeicaoDisp[i] = false;
      abrirPorta(millis(), "Horario");
      break;
    }
  }
}

void abrirPorta(unsigned long ms, String origem) {
  portaAberta = true;
  tempoPortaAbriu = ms;
  ultimoEvento = "Porta aberta por " + origem;

  porta.write(ANGULO_ABERTO);
  digitalWrite(ledPortaAberta, HIGH);
  digitalWrite(ledPortaFechada, LOW);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("PORTA ABERTA!");
  lcd.setCursor(0, 1);
  lcd.print("Racao liberada");

  tocarAbertura();
  Serial.println(ultimoEvento);
}

void fecharPorta(struct tm timeinfo, bool horaValida, String origem) {
  portaAberta = false;
  ultimoEvento = "Porta fechada por " + origem;

  porta.write(ANGULO_FECHADO);
  digitalWrite(ledPortaAberta, LOW);
  digitalWrite(ledPortaFechada, HIGH);

  mostrandoAviso = true;
  tempoAvisoInicio = millis();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("* COMIDA CAIU *");
  lcd.setCursor(0, 1);

  if (horaValida) {
    char hora[17];
    sprintf(hora, "Hora %02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    lcd.print(hora);
  } else {
    lcd.print("Porta fechada");
  }

  tocarFechamento();
  Serial.println(ultimoEvento);
}

void atualizarDisplay(struct tm* timeinfo) {
  if (mostrandoAviso && (millis() - tempoAvisoInicio < AVISO_DURACAO)) return;
  mostrandoAviso = false;

  lcd.clear();

  String temp = temperaturaTexto();

  if (!timeinfo) {
    lcd.setCursor(0, 0);
    lcd.print("Sem horario NTP");
    lcd.setCursor(0, 1);
    lcd.print(portaAberta ? "Porta: ABERTA" : "Porta: FECHADA");
    return;
  }

  char linha0[17];
  snprintf(linha0, sizeof(linha0), "%02d:%02d T:%s", timeinfo->tm_hour, timeinfo->tm_min, temp.c_str());
  lcd.setCursor(0, 0);
  lcd.print(linha0);

  int proxH = HORA_REFEICAO[0][0];
  int proxM = HORA_REFEICAO[0][1];
  for (int i = 0; i < 3; i++) {
    int h = HORA_REFEICAO[i][0];
    int m = HORA_REFEICAO[i][1];
    if (h > timeinfo->tm_hour || (h == timeinfo->tm_hour && m > timeinfo->tm_min)) {
      proxH = h;
      proxM = m;
      break;
    }
  }

  char linha1[17];
  snprintf(linha1, sizeof(linha1), "P:%s Prox:%02d:%02d", portaAberta ? "AB" : "FE", proxH, proxM);
  lcd.setCursor(0, 1);
  lcd.print(linha1);
}

String horaAtualTexto() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 10)) return "Sem NTP";

  char hora[20];
  sprintf(hora, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  return String(hora);
}

String temperaturaTexto() {
  float temp = dht.readTemperature();
  if (isnan(temp)) return "--C";

  char texto[8];
  snprintf(texto, sizeof(texto), "%.0fC", temp);
  return String(texto);
}

String paginaHTML() {
  String estado = portaAberta ? "ABERTA" : "FECHADA";
  String cor = portaAberta ? "#16a34a" : "#dc2626";

  String html = "<!DOCTYPE html><html lang='pt-BR'><head>";
  html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Pet Feeder IoT</title>";
  html += "<style>body{font-family:Arial;background:#f3f4f6;margin:0;padding:20px;text-align:center;}";
  html += ".card{max-width:430px;margin:auto;background:white;padding:25px;border-radius:18px;box-shadow:0 4px 18px #0002;}";
  html += "h1{margin-top:0}.status{font-size:28px;font-weight:bold;color:";
  html += cor;
  html += ";}";
  html += "a{display:block;text-decoration:none;color:white;padding:15px;margin:12px;border-radius:12px;font-size:20px;font-weight:bold;}";
  html += ".abrir{background:#16a34a}.fechar{background:#dc2626}.info{background:#e5e7eb;padding:12px;border-radius:12px;margin-top:15px;}";
  html += "</style></head><body><div class='card'>";
  html += "<h1>🐾 Alimentador Pet</h1>";
  html += "<p>Porta agora:</p><p class='status'>" + estado + "</p>";
  html += "<a class='abrir' href='/abrir'>ABRIR PORTA</a>";
  html += "<a class='fechar' href='/fechar'>FECHAR PORTA</a>";
  html += "<div class='info'><p><b>Hora:</b> " + horaAtualTexto() + "</p>";
  html += "<p><b>Temperatura:</b> " + temperaturaTexto() + "</p>";
  html += "<p><b>Último evento:</b> " + ultimoEvento + "</p>";
  html += "<p>Atualiza sozinho a cada 3 segundos.</p></div>";
  html += "</div><script>setTimeout(()=>location.reload(),3000)</script></body></html>";
  return html;
}

void tocarAbertura() {
  tone(buzzer, 1200, 150);
  delay(180);
  tone(buzzer, 1500, 150);
  delay(180);
  noTone(buzzer);
}

void tocarFechamento() {
  tone(buzzer, 800, 150);
  delay(180);
  tone(buzzer, 1000, 150);
  delay(180);
  noTone(buzzer);
}
