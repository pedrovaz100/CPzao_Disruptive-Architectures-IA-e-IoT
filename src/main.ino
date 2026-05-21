#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
WebServer server(80);
Servo porta;

const char* ssid = "Wokwi-GUEST";
const char* password = "";

const int botaoAbrir = 33;
const int botaoFechar = 25;
const int buzzer = 32;
const int ledPortaAberta = 26;
const int ledPortaFechada = 27;
const int servoPorta = 19;

const int ANGULO_FECHADO = 0;
const int ANGULO_ABERTO = 90;

const unsigned long INTERVALO_AUTO = 20000;
const unsigned long TEMPO_ABERTA = 5000;
const unsigned long TEMPO_ALARME = 2000;
const unsigned long TEMPO_BIP = 300;
const unsigned long DEBOUNCE = 250;

bool portaAberta = false;
bool liberando = false;
bool alarmando = false;
bool bipLigado = false;

unsigned long ultimoAuto = 0;
unsigned long inicioLiberacao = 0;
unsigned long inicioAlarme = 0;
unsigned long ultimoBip = 0;
unsigned long ultimoBotaoAbrir = 0;
unsigned long ultimoBotaoFechar = 0;
unsigned long ultimaTela = 0;
unsigned long ultimoWifi = 0;

String ultimoEvento = "Sistema iniciado";
String linha1Atual = "";
String linha2Atual = "";

void tela(String linha1, String linha2) {

  linha1 = linha1.substring(0, 16);
  linha2 = linha2.substring(0, 16);

  if (linha1 == linha1Atual &&
      linha2 == linha2Atual) {
    return;
  }

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print(linha1);

  lcd.setCursor(0, 1);
  lcd.print(linha2);

  linha1Atual = linha1;
  linha2Atual = linha2;
}

void atualizarLeds() {

  digitalWrite(
    ledPortaAberta,
    portaAberta ? HIGH : LOW
  );

  digitalWrite(
    ledPortaFechada,
    portaAberta ? LOW : HIGH
  );
}

void abrirPorta(String origem) {

  if (liberando || alarmando) {
    return;
  }

  porta.write(ANGULO_ABERTO);

  portaAberta = true;
  liberando = true;
  alarmando = false;

  inicioLiberacao = millis();

  ultimoEvento = "Aberta por " + origem;

  atualizarLeds();

  tela(
    "Liberando",
    "racao"
  );

  tone(buzzer, 1000);

  bipLigado = true;

  ultimoBip = millis();
}

void iniciarAlarme() {

  liberando = false;
  alarmando = true;

  inicioAlarme = millis();

  ultimoBip = millis();

  ultimoEvento = "Aviso fechamento";

  tela(
    "Atencao",
    "Vai fechar"
  );
}

void fecharPorta(String origem) {

  noTone(buzzer);

  porta.write(ANGULO_FECHADO);

  portaAberta = false;
  liberando = false;
  alarmando = false;
  bipLigado = false;

  ultimoEvento = "Fechada por " + origem;

  atualizarLeds();

  tela(
    "Porta fechada",
    "Racao OK"
  );
}

void controleAutomatico() {

  unsigned long agora = millis();

  if (!liberando &&
      !alarmando &&
      agora - ultimoAuto >= INTERVALO_AUTO) {

    ultimoAuto = agora;

    abrirPorta("Auto");
  }
}

void controleLiberacao() {

  if (liberando &&
      millis() - inicioLiberacao >= TEMPO_ABERTA) {

    iniciarAlarme();
  }
}

void controleAlarme() {

  if (!alarmando) {

    if (bipLigado &&
        millis() - ultimoBip >= 400) {

      noTone(buzzer);

      bipLigado = false;
    }

    return;
  }

  unsigned long agora = millis();

  if (agora - ultimoBip >= TEMPO_BIP) {

    ultimoBip = agora;

    if (bipLigado) {

      noTone(buzzer);

      bipLigado = false;

    } else {

      tone(buzzer, 1200);

      bipLigado = true;
    }
  }

  if (agora - inicioAlarme >= TEMPO_ALARME) {

    fecharPorta("Auto");
  }
}

void verificarBotoes() {

  unsigned long agora = millis();

  if (digitalRead(botaoAbrir) == LOW &&
      agora - ultimoBotaoAbrir >= DEBOUNCE) {

    ultimoBotaoAbrir = agora;

    abrirPorta("Botao");
  }

  if (digitalRead(botaoFechar) == LOW &&
      agora - ultimoBotaoFechar >= DEBOUNCE) {

    ultimoBotaoFechar = agora;

    fecharPorta("Botao");
  }
}

String paginaHTML() {

  String estado =
    portaAberta ? "ABERTA" : "FECHADA";

  String html = "";

  html += "<!DOCTYPE html>";
  html += "<html lang='pt-BR'>";

  html += "<head>";

  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<meta http-equiv='refresh' content='3'>";

  html += "<title>Alimentador Pet</title>";

  html += "<style>";

  html += "body{font-family:Arial;background:#f3f4f6;text-align:center;padding:20px;}";

  html += ".card{max-width:500px;margin:auto;background:white;padding:25px;border-radius:18px;box-shadow:0 4px 18px #0002;}";

  html += ".info{background:#e5e7eb;padding:12px;border-radius:12px;margin:10px;}";

  html += "a{display:block;text-decoration:none;color:white;padding:14px;margin:12px;border-radius:12px;font-size:18px;font-weight:bold;}";

  html += ".abrir{background:#16a34a}";
  html += ".fechar{background:#dc2626}";

  html += "</style>";

  html += "</head>";

  html += "<body>";

  html += "<div class='card'>";

  html += "<h1>Alimentador Pet</h1>";

  html += "<div class='info'><b>Porta:</b> ";
  html += estado;
  html += "</div>";

  html += "<div class='info'><b>Contagem automatica:</b><br>";
  html += "20 segundos - Abrir porta<br>";
  html += "15 segundos - Liberando racao<br>";
  html += "10 segundos - Porta aberta<br>";
  html += "5 segundos - Aviso sonoro<br>";
  html += "0 segundos - Porta fechada";
  html += "</div>";

  html += "<div class='info'><b>Ultimo evento:</b> ";
  html += ultimoEvento;
  html += "</div>";

  html += "<a class='abrir' href='/abrir'>";
  html += "Liberar comida";
  html += "</a>";

  html += "<a class='fechar' href='/fechar'>";
  html += "Fechar porta";
  html += "</a>";

  html += "</div>";

  html += "</body>";
  html += "</html>";

  return html;
}

void configurarSite() {

  server.on("/", HTTP_GET, []() {

    server.send(
      200,
      "text/html",
      paginaHTML()
    );
  });

  server.on("/abrir", HTTP_GET, []() {

    abrirPorta("Site");

    server.sendHeader("Location", "/");

    server.send(303);
  });

  server.on("/fechar", HTTP_GET, []() {

    fecharPorta("Site");

    server.sendHeader("Location", "/");

    server.send(303);
  });

  server.begin();
}

void conectarWiFi() {

  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);

  Serial.print("Conectando WiFi");

  tela(
    "Conectando",
    "WiFi"
  );
}

void atualizarWiFi() {

  static bool conectado = false;

  if (conectado) {
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {

    conectado = true;

    Serial.println();

    Serial.print("IP: ");

    Serial.println(
      WiFi.localIP()
    );

    tela(
      "IP do site:",
      WiFi.localIP().toString()
    );

    return;
  }

  if (millis() - ultimoWifi >= 500) {

    ultimoWifi = millis();

    Serial.print(".");
  }
}

void atualizarDisplay() {

  if (millis() - ultimaTela < 1000) {
    return;
  }

  ultimaTela = millis();

  if (!liberando && !alarmando) {

    unsigned long restante =
      (INTERVALO_AUTO - (millis() - ultimoAuto)) / 1000;

    if (restante > 20) {
      restante = 20;
    }

    tela(
      "Aguardando",
      "Auto " + String(restante) + " seg"
    );
  }
}

void setup() {

  Serial.begin(115200);

  pinMode(botaoAbrir, INPUT_PULLUP);
  pinMode(botaoFechar, INPUT_PULLUP);

  pinMode(buzzer, OUTPUT);

  pinMode(ledPortaAberta, OUTPUT);
  pinMode(ledPortaFechada, OUTPUT);

  Wire.begin(21, 22);

  lcd.init();
  lcd.backlight();

  porta.setPeriodHertz(50);

  porta.attach(
    servoPorta,
    500,
    2400
  );

  porta.write(ANGULO_FECHADO);

  portaAberta = false;

  atualizarLeds();

  tela(
    "Pet Feeder",
    "Iniciando"
  );

  conectarWiFi();

  configurarSite();
}

void loop() {

  server.handleClient();

  atualizarWiFi();

  controleAutomatico();

  controleLiberacao();

  controleAlarme();

  verificarBotoes();

  atualizarDisplay();
}