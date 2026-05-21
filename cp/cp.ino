#include <WiFi.h>
#include <time.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

// ================= LCD I2C =================
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Endereço I2C padrão

// ================= SERVO =================
// NÃO TEM SERVO NO DIAGRAMA - SIMULADO COM LED
// Servo servo;

// ================= WIFI =================
const char* ssid     = "";
const char* password = ";

// ================= NTP =================
const char* ntpServer = "a.st1.ntp.br";
const long  gmtOffset_sec = -3 * 3600;
const int   daylightOffset_sec = 0;

// ================= PINOS DO DIAGRAMA =================
const int botaoAbrir  = 33;  // BTN1 (azul)
const int botaoFechar = 25;  // BTN2 (amarelo)
const int buzzer      = 32;  // Buzzer
// Servo simulado com LED26 (azul)
const int ledPortaAberta  = 26;  
const int ledPortaFechada = 27; 
const int ledStatus       = 26;  // Reaproveitado

// ================= DHT22 =================
#include "DHT.h"
#define DHTPIN 18
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// ================= SERVO SIMULADO =================
const int PORTA_ABERTA  = 90;
const int PORTA_FECHADA = 0;

// ================= HORÁRIOS =================
const int HORA_REFEICAO[3][2] = {{7,0}, {12,0}, {18,0}};

// ================= TEMPOS =================
const unsigned long DEBOUNCE_MS   = 300;
const unsigned long AVISO_DURACAO = 5000;
const unsigned long PORTA_TEMPO   = 10000;  // 10s para simulação

// ================= VARIÁVEIS =================
unsigned long ultimoPressAbrir  = 0;
unsigned long ultimoPressFechar = 0;
unsigned long tempoAvisoInicio  = 0;
unsigned long tempoPortaAbriu   = 0;
unsigned long ultimaAtualizacao = 0;

bool portaAberta    = false;
bool mostrandoAviso = false;
bool refeicaoDisp[3] = {true, true, true};

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(115200);
  
  // Inicializar hardware do diagrama
  pinMode(botaoAbrir, INPUT_PULLUP);
  pinMode(botaoFechar, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);
  pinMode(ledPortaAberta, OUTPUT);
  pinMode(ledPortaFechada, OUTPUT);
  
  // LEDs iniciais apagados
  digitalWrite(ledPortaAberta, LOW);
  digitalWrite(ledPortaFechada, LOW);
  
  // Inicializar LCD I2C
  Wire.begin(21, 22);  // SDA=21, SCL=22 (do diagrama)
  lcd.init();
  lcd.backlight();
  
  // Inicializar DHT
  dht.begin();
  
  // Tela inicial
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Pet Feeder v2.0");
  lcd.setCursor(0,1);
  lcd.print("Conectando...");
  
  conectarWiFi();
  configurarNTP();
  
  delay(2000);
  lcd.clear();
  
  Serial.println("Sistema iniciado!");
}

// =====================================================
// LOOP
// =====================================================
void loop() {
  struct tm timeinfo;
  bool tempoValido = getLocalTime(&timeinfo);
  unsigned long ms = millis();
  
  // Controle automático porta
  if (portaAberta && (ms - tempoPortaAbriu >= PORTA_TEMPO)) {
    fecharPorta(timeinfo);
  }
  
  // Horários automáticos
  if (tempoValido) {
    verificarHorarios(timeinfo);
  }
  
  // Botões
  verificarBotoes(ms);
  
  // Display
  if (ms - ultimaAtualizacao > 1000) {  // Atualiza a cada 1s
    atualizarDisplay(tempoValido ? &timeinfo : nullptr);
    ultimaAtualizacao = ms;
  }
  
  delay(100);
}

// =====================================================
// WIFI OBRIGATÓRIO
// =====================================================
void conectarWiFi() {
  WiFi.begin(ssid, password);
  
  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 30) {
    delay(500);
    Serial.print(".");
    lcd.setCursor(12,1);
    lcd.print(".");
    tentativas++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi OK!");
    Serial.print("IP: "); Serial.println(WiFi.localIP());
  } else {
    erroCritico("WiFi Falhou!");
  }
}

// =====================================================
// NTP OBRIGATÓRIO
// =====================================================
void configurarNTP() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  struct tm timeinfo;
  if(getLocalTime(&timeinfo)) {
    Serial.printf("NTP OK: %02d:%02d:%02d\n", 
                  timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  } else {
    erroCritico("NTP Falhou!");
  }
}

// =====================================================
// BOTÕES
// =====================================================
void verificarBotoes(unsigned long ms) {
  // Botão Abrir (BTN1 pin 33)
  if (digitalRead(botaoAbrir) == LOW && !portaAberta && 
      (ms - ultimoPressAbrir > DEBOUNCE_MS)) {
    ultimoPressAbrir = ms;
    abrirPorta(ms);
  }
  
  // Botão Fechar (BTN2 pin 25)
  if (digitalRead(botaoFechar) == LOW && portaAberta && 
      (ms - ultimoPressFechar > DEBOUNCE_MS)) {
    ultimoPressFechar = ms;
    fecharPorta(getLocalTimeStruct());
  }
}

// =====================================================
// HORÁRIOS AUTOMÁTICOS
// =====================================================
void verificarHorarios(struct tm timeinfo) {
  for (int i = 0; i < 3; i++) {
    if (timeinfo.tm_hour == HORA_REFEICAO[i][0] &&
        timeinfo.tm_min == HORA_REFEICAO[i][1] &&
        timeinfo.tm_sec <= 5 &&
        refeicaoDisp[i] &&
        !portaAberta) {
      
      Serial.printf("Refeição #%d: %02d:%02d\n", i+1, 
                    HORA_REFEICAO[i][0], HORA_REFEICAO[i][1]);
      refeicaoDisp[i] = false;
      abrirPorta(millis());
      break;
    }
    
    if (timeinfo.tm_min != HORA_REFEICAO[i][1]) {
      refeicaoDisp[i] = true;
    }
  }
}

// =====================================================
// ABRIR PORTA (LED 26 liga)
// =====================================================
void abrirPorta(unsigned long ms) {
  portaAberta = true;
  tempoPortaAbriu = ms;
  
  // Simula servo com LED
  digitalWrite(ledPortaAberta, HIGH);
  digitalWrite(ledPortaFechada, LOW);
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("PORTA ABERTA!");
  lcd.setCursor(0,1);
  lcd.print("Racao liberada");
  
  // Beeps (buzzer pin 32)
  for(int i = 0; i < 3; i++) {
    tone(buzzer, 1200 + i*200, 200);
    delay(250);
  }
  
  Serial.println("*** PORTA ABERTA ***");
}

// =====================================================
// FECHAR PORTA (LED 27 liga)
// =====================================================
void fecharPorta(struct tm timeinfo) {
  portaAberta = false;
  
  // Simula servo fechando
  digitalWrite(ledPortaAberta, LOW);
  digitalWrite(ledPortaFechada, HIGH);
  
  mostrandoAviso = true;
  tempoAvisoInicio = millis();
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("* COMIDA CAIU *");
  
  char hora[17];
  sprintf(hora, "%02d:%02d:%02d", 
          timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  lcd.setCursor(0,1);
  lcd.print(hora);
  
  // Alerta sonoro
  tone(buzzer, 800, 200);
  delay(250);
  tone(buzzer, 1200, 200);
  delay(250);
  tone(buzzer, 1600, 400);
  
  Serial.println("*** PORTA FECHADA ***");
}

// =====================================================
// DISPLAY
// =====================================================
void atualizarDisplay(struct tm* timeinfo) {
  if (mostrandoAviso && (millis() - tempoAvisoInicio < AVISO_DURACAO)) {
    return;
  }
  mostrandoAviso = false;
  
  lcd.clear();
  
  if (timeinfo) {
    // Linha 0: Hora + Temperatura + Status
    float temp = dht.readTemperature();
    char linha0[17];
    sprintf(linha0, "H:%02d:%02d T:%.0fC", 
            timeinfo->tm_hour, timeinfo->tm_min, temp);
    lcd.setCursor(0,0);
    lcd.print(linha0);
    
    // Linha 1: Próxima + Status porta
    char linha1[17];
    int proxH = 99;
    
    for(int i = 0; i < 3; i++) {
      if (HORA_REFEICAO[i][0] > timeinfo->tm_hour ||
          (HORA_REFEICAO[i][0] == timeinfo->tm_hour && 
           HORA_REFEICAO[i][1] > timeinfo->tm_min)) {
        proxH = HORA_REFEICAO[i][0];
        break;
      }
    }
    
    if (proxH < 99) {
      sprintf(linha1, "Prox:%02d:00 P:%s", proxH, 
              portaAberta ? "AB" : "FE");
    } else {
      sprintf(linha1, "Prox:amanha P:%s", portaAberta ? "AB" : "FE");
    }
    
    lcd.setCursor(0,1);
    lcd.print(linha1);
  }
}

// =====================================================
// AUXILIARES
// =====================================================
struct tm getLocalTimeStruct() {
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  return timeinfo;
}

void erroCritico(const char* msg) {
  Serial.println("ERRO CRITICO: " + String(msg));
  lcd.clear();
  lcd.print("ERRO CRITICO!");
  lcd.setCursor(0,1);
  lcd.print(msg);
  
  // Beep erro (buzzer pin 32)
  for(int i = 0; i < 10; i++) {
    tone(buzzer, 300, 200);
    delay(250);
  }
  ESP.restart();
}