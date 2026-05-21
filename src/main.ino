#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <time.h>

// ======================================================
// PROJETO: ALIMENTADOR AUTOMATICO PARA PETS
// FIAP - Checkpoint IoT
// Descricao:
// Em horarios programados, o ESP32 abre a porta do reservatorio,
// libera a racao, toca um alarme antes de fechar e mostra tudo
// no LCD e no dashboard web.
// ======================================================

// Wi-Fi do Wokwi
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// Horario programado para liberar comida
// Altere aqui o horario desejado:
int horaAlimentacao = 12;
int minutoAlimentacao = 0;

// Quantidade simulada de racao
// Quanto maior o tempo, mais racao cairia no pote
int tempoLiberacaoMs = 5000;

// Pinos do circuito
#define SERVO_PIN 18
#define BUZZER_PIN 19
#define LED_ABERTO 26
#define LED_FECHADO 27
#define BOTAO_LIBERAR 25

// Angulos do servo
#define PORTA_FECHADA 0
#define PORTA_ABERTA 90

WebServer server(80);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo servoPorta;

bool portaAberta = false;
bool jaAlimentouHoje = false;
int ultimoDia = -1;
String ultimoEvento = "Sistema iniciado";

void atualizarLeds() {
  digitalWrite(LED_ABERTO, portaAberta ? HIGH : LOW);
  digitalWrite(LED_FECHADO, portaAberta ? LOW : HIGH);
}

void escreverLCD(String linha1, String linha2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(linha1.substring(0, 16));
  lcd.setCursor(0, 1);
  lcd.print(linha2.substring(0, 16));
}

void alarmeFechamento() {
  ultimoEvento = "Alarme: porta vai fechar";
  escreverLCD("ATENCAO", "Porta fechando");

  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, 1200);
    delay(300);
    noTone(BUZZER_PIN);
    delay(300);
  }
}

void abrirPorta() {
  servoPorta.write(PORTA_ABERTA);
  portaAberta = true;
  atualizarLeds();

  ultimoEvento = "Porta aberta: racao caindo";
  escreverLCD("Liberando", "racao do pet");

  tone(BUZZER_PIN, 900);
  delay(400);
  noTone(BUZZER_PIN);
}

void fecharPorta() {
  alarmeFechamento();

  servoPorta.write(PORTA_FECHADA);
  portaAberta = false;
  atualizarLeds();

  ultimoEvento = "Porta fechada";
  escreverLCD("Porta fechada", "Processo OK");
}

void liberarComida() {
  abrirPorta();
  delay(tempoLiberacaoMs);
  fecharPorta();
  ultimoEvento = "Alimentacao concluida";
  escreverLCD("Alimentacao", "concluida");
}

String horaAtualTexto() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "Sem horario";
  }

  char buffer[9];
  sprintf(buffer, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  return String(buffer);
}

void verificarHorarioAlimentacao() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return;
  }

  if (timeinfo.tm_mday != ultimoDia) {
    ultimoDia = timeinfo.tm_mday;
    jaAlimentouHoje = false;
  }

  if (timeinfo.tm_hour == horaAlimentacao &&
      timeinfo.tm_min == minutoAlimentacao &&
      !jaAlimentouHoje) {
    jaAlimentouHoje = true;
    ultimoEvento = "Horario programado ativado";
    liberarComida();
  }
}

String paginaWeb() {
  String estado = portaAberta ? "ABERTA" : "FECHADA";
  String situacao = portaAberta ? "Liberando comida" : "Aguardando horario programado";

  String html = "";
  html += "<!DOCTYPE html><html lang='pt-BR'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<meta http-equiv='refresh' content='5'>";
  html += "<title>Alimentador Automatico Pet</title>";
  html += "<style>";
  html += "body{font-family:Arial;background:#eef2f5;margin:0;padding:25px;text-align:center;}";
  html += ".card{max-width:560px;margin:auto;background:white;padding:25px;border-radius:18px;box-shadow:0 6px 20px #0002;}";
  html += "h1{color:#263238;margin-bottom:8px;}.sub{color:#607d8b;margin-bottom:20px;}";
  html += ".status{font-size:25px;font-weight:bold;margin:15px;color:#1b5e20;}";
  html += ".info{background:#f4f7f9;padding:12px;border-radius:12px;margin:10px 0;text-align:left;}";
  html += "button{border:0;border-radius:12px;padding:14px 22px;margin:8px;font-size:16px;cursor:pointer;font-weight:bold;}";
  html += ".abrir{background:#4CAF50;color:white;}.fechar{background:#F44336;color:white;}";
  html += ".rodape{font-size:13px;color:#78909c;margin-top:18px;}";
  html += "</style></head><body>";
  html += "<div class='card'>";
  html += "<h1>Alimentador Automatico para Pets</h1>";
  html += "<div class='sub'>Controle de racao com ESP32, servo, LCD e alarme</div>";
  html += "<div class='status'>Porta: " + estado + "</div>";
  html += "<div class='info'><b>Situacao:</b> " + situacao + "</div>";
  html += "<div class='info'><b>Horario atual:</b> " + horaAtualTexto() + "</div>";
  html += "<div class='info'><b>Horario programado:</b> ";
  if (horaAlimentacao < 10) html += "0";
  html += String(horaAlimentacao) + ":";
  if (minutoAlimentacao < 10) html += "0";
  html += String(minutoAlimentacao) + "</div>";
  html += "<div class='info'><b>Tempo de liberacao:</b> " + String(tempoLiberacaoMs / 1000) + " segundos</div>";
  html += "<div class='info'><b>Ultimo evento:</b> " + ultimoEvento + "</div>";
  html += "<a href='/liberar'><button class='abrir'>Liberar comida agora</button></a>";
  html += "<a href='/fechar'><button class='fechar'>Fechar porta</button></a>";
  html += "<div class='rodape'>Dashboard FE + automacao BE do ESP32</div>";
  html += "</div></body></html>";
  return html;
}

void rotaPrincipal() {
  server.send(200, "text/html", paginaWeb());
}

void rotaLiberar() {
  liberarComida();
  server.sendHeader("Location", "/");
  server.send(303);
}

void rotaFechar() {
  fecharPorta();
  server.sendHeader("Location", "/");
  server.send(303);
}

void conectarWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Conectando no Wi-Fi");

  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 30) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("Conectado! IP: ");
    Serial.println(WiFi.localIP());
    escreverLCD("IP do site:", WiFi.localIP().toString());
  } else {
    Serial.println();
    Serial.println("Wi-Fi nao conectado");
    escreverLCD("WiFi falhou", "Modo local");
  }
}

void configurarHorario() {
  // Horario de Brasilia: UTC-3
  configTime(-3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 8000)) {
    Serial.println("Horario NTP configurado");
  } else {
    Serial.println("Nao foi possivel configurar NTP");
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_ABERTO, OUTPUT);
  pinMode(LED_FECHADO, OUTPUT);
  pinMode(BOTAO_LIBERAR, INPUT_PULLUP);

  Wire.begin();
  lcd.init();
  lcd.backlight();

  servoPorta.attach(SERVO_PIN);
  servoPorta.write(PORTA_FECHADA);
  portaAberta = false;
  atualizarLeds();

  escreverLCD("Alimentador", "iniciando...");
  delay(1000);

  conectarWiFi();
  configurarHorario();

  server.on("/", rotaPrincipal);
  server.on("/liberar", rotaLiberar);
  server.on("/fechar", rotaFechar);
  server.begin();

  ultimoEvento = "Sistema pronto";
  escreverLCD("Sistema pronto", "Aguardando...");
}

void loop() {
  server.handleClient();
  verificarHorarioAlimentacao();

  // Botao fisico para liberar comida manualmente
  if (digitalRead(BOTAO_LIBERAR) == LOW) {
    delay(80);
    if (digitalRead(BOTAO_LIBERAR) == LOW) {
      liberarComida();
      while (digitalRead(BOTAO_LIBERAR) == LOW) {
        delay(10);
      }
    }
  }

  delay(200);
}
