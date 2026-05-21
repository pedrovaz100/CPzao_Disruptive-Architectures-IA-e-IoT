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
