<<<<<<< HEAD
# Alimentador Automático para Pets - IoT

## Descrição do Projeto

Este projeto foi desenvolvido para resolver um problema comum para tutores de animais: controlar a alimentação do pet mesmo quando o dono não está por perto.

O sistema utiliza um ESP32 para liberar ração automaticamente em um horário programado. Quando chega o horário definido, o servo motor abre a porta do reservatório, a comida cai no pote, um alarme sonoro avisa que a porta será fechada e depois o sistema fecha automaticamente.

## Problema Resolvido

Muitos tutores esquecem horários de alimentação ou ficam fora de casa. O alimentador automático ajuda a manter a rotina alimentar do animal com mais controle e segurança.

## Componentes Utilizados

- ESP32
- Servo motor
- Display LCD I2C
- Buzzer
- LEDs de status
- Botão físico
- Dashboard web via Wi-Fi
- Wokwi para simulação

## Funcionamento

1. O ESP32 conecta ao Wi-Fi do Wokwi.
2. O sistema sincroniza o horário pela internet.
3. O dashboard web mostra status, horário atual e horário programado.
4. No horário configurado, a porta abre automaticamente.
5. A ração cai durante alguns segundos.
6. O buzzer toca avisando que a porta vai fechar.
7. A porta fecha automaticamente.
8. O usuário também pode liberar comida manualmente pelo botão físico ou pelo site.

## Front-End

O Front-End é o dashboard web aberto pelo IP do ESP32. Ele mostra:

- Status da porta
- Horário atual
- Horário programado
- Tempo de liberação da ração
- Último evento
- Botão para liberar comida manualmente
- Botão para fechar a porta

## Back-End

O Back-End é o código do ESP32, responsável por:

- Controlar o servo motor
- Ler o botão
- Controlar buzzer e LEDs
- Atualizar o LCD
- Criar o servidor web
- Verificar o horário programado
- Executar a automação da alimentação

## Horário Programado

No código, o horário está configurado aqui:

```cpp
int horaAlimentacao = 12;
int minutoAlimentacao = 0;
```

Para mudar o horário, basta alterar esses valores.

## Quantidade de Ração

A quantidade é simulada pelo tempo que a porta fica aberta:

```cpp
int tempoLiberacaoMs = 5000;
```

Quanto maior o tempo, mais ração seria liberada.

## Como Executar

1. Abrir o projeto no VS Code.
2. Instalar a extensão PlatformIO IDE.
3. Clicar em Build.
4. Rodar a simulação no Wokwi.
5. Abrir o IP mostrado no monitor serial ou no LCD.

## Objetivo

Criar uma solução IoT funcional, com automação, dashboard e atuadores, aplicada ao cuidado de pets.
=======
# 🐾 Alimentador Automático Inteligente para Pets com ESP32

## 📌 Descrição do Projeto

Este projeto consiste em um alimentador automático inteligente para pets desenvolvido utilizando o microcontrolador ESP32 e conceitos de Internet das Coisas (IoT).

O sistema foi criado com o objetivo de automatizar a alimentação de animais domésticos, permitindo que a ração seja liberada automaticamente em intervalos programados ou manualmente por botões físicos e dashboard web.

Além da automação da alimentação, o projeto conta com monitoramento visual e sonoro, proporcionando maior controle, segurança e praticidade para o usuário.

---

# 💡 Objetivo do Projeto

O principal objetivo do projeto é auxiliar donos de pets que passam longos períodos fora de casa, garantindo que o animal receba alimentação de forma automática, organizada e segura.

O sistema foi desenvolvido visando:

- automação residencial
- praticidade
- controle alimentar
- integração IoT
- monitoramento em tempo real

---

# ⚙️ Funcionalidades Implementadas

✅ Liberação automática de ração a cada 20 segundos  
✅ Controle manual por botão físico  
✅ Controle manual pelo dashboard web  
✅ Servo motor para abertura e fechamento do compartimento  
✅ Alarme sonoro utilizando buzzer  
✅ LEDs indicadores de status  
✅ Display LCD exibindo mensagens do sistema  
✅ Dashboard web acessível pelo navegador  
✅ Sistema totalmente sem `delay()`  
✅ Controle utilizando `millis()`  
✅ Estrutura modular e organizada  

---

# 🧩 Componentes Utilizados

## 🔹 ESP32

Microcontrolador principal responsável pelo processamento do sistema, conexão Wi-Fi e controle dos componentes.

---

## 🔹 Servo Motor SG90

Responsável pela abertura e fechamento do compartimento de ração.

### Função:
- abrir a tampa do alimentador
- fechar automaticamente após o tempo programado

---

## 🔹 Display LCD 16x2 com módulo I2C

Exibe informações em tempo real sobre o funcionamento do sistema.

### Mensagens exibidas:
- "Aguardando"
- "Auto 20 seg"
- "Liberando racao"
- "Atencao"
- "Vai fechar"
- "Porta fechada"

---

## 🔹 Buzzer

Responsável pelos alertas sonoros do sistema.

### Função:
- aviso de liberação de ração
- alerta de fechamento da porta

---

## 🔹 LEDs

### LED Verde
Indica que a porta está aberta e a ração está sendo liberada.

### LED Azul
Indica que o sistema está em espera ou porta fechada.

---

## 🔹 Botões Push Button

### Botão Amarelo
Liberação manual da ração.

### Botão Azul
Fechamento manual da porta.

---

## 🔹 Sensor DHT22

Sensor de temperatura e umidade utilizado para expansão do projeto IoT.

### Possíveis usos:
- monitoramento do ambiente
- controle climático
- monitoramento da conservação da ração

---

# 🌐 Dashboard Web

O sistema cria uma página web hospedada diretamente no ESP32.

Através do navegador é possível:

✅ Ver status da porta  
✅ Visualizar eventos do sistema  
✅ Liberar ração remotamente  
✅ Fechar a porta manualmente  
✅ Monitorar o funcionamento do alimentador  

---

# 🔊 Funcionamento do Sistema

## 🔄 Funcionamento Automático

1. O ESP32 inicia o sistema.
2. O LCD exibe a mensagem de inicialização.
3. O sistema conecta ao Wi-Fi.
4. O dashboard web é iniciado.
5. O sistema entra em modo de espera.
6. O LCD inicia a contagem regressiva:
   - Auto 20 seg
   - Auto 19 seg
   - Auto 18 seg
   - ...
7. Ao chegar em 0 segundos:
   - o servo abre a porta
   - a ração é liberada
   - o LED verde acende
   - o buzzer toca
8. Após 5 segundos:
   - o sistema inicia o alerta de fechamento
   - o buzzer emite avisos sonoros
9. O servo fecha automaticamente a porta.
10. O sistema retorna ao modo de espera.

---

# 🖲️ Funcionamento Manual

O sistema também permite controle manual:

## Pelo botão físico:
- abrir a porta
- liberar ração
- fechar a porta

## Pelo dashboard web:
- liberar comida remotamente
- fechar a porta remotamente

---

# 📟 Mensagens no Display LCD

O display LCD informa o estado atual do sistema em tempo real.

### Exemplos:

```text
Aguardando
Auto 20 seg
```

```text
Liberando
racao
```

```text
Atencao
Vai fechar
```

```text
Porta fechada
Racao OK
```

---

# 🧠 Estrutura do Software

O sistema foi desenvolvido utilizando:

- programação orientada a eventos
- controle não bloqueante
- `millis()` no lugar de `delay()`
- lógica modular
- arquitetura IoT

---

# 🔧 Tecnologias Utilizadas

- C++
- ESP32
- Arduino Framework
- PlatformIO
- Wokwi Simulator
- Wi-Fi
- HTML básico
- IoT

---

# 🚀 Possíveis Melhorias Futuras

- Aplicativo mobile
- Integração com MQTT
- Controle via celular
- Sensor de nível de ração
- Câmera para monitoramento do pet
- Integração com Alexa ou Google Assistant
- Impressão 3D da estrutura
- Histórico de alimentação
- Múltiplos horários automáticos

---

# 🐶 Aplicações do Projeto

Ideal para:

- automação residencial
- donos que trabalham fora
- controle alimentar de pets
- projetos acadêmicos de IoT
- aprendizado de ESP32

---

# 📁 Estrutura do Projeto

```text
src/
 └── main.ino

diagram.json
platformio.ini
wokwi.toml
README.md
```

---

# ▶️ Simulação no Wokwi

O projeto foi desenvolvido e testado no simulador Wokwi utilizando ESP32.

A simulação inclui:

- LCD
- buzzer
- servo motor
- LEDs
- botões
- dashboard web
- lógica automática

---

# 👨‍💻 Integrantes

RM565139 — João Victor Luiz  
RM566551 — Pedro Henrique Vaz Ferreira
>>>>>>> 09406ab48dc02ca546018d31bfed440440a34dfe
