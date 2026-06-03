# Phycocarbon IoT — Biofotorreator Inteligente com ESP32

## Visão Geral

O projeto **Phycocarbon IoT** simula um biofotorreator automatizado para cultivo de microalgas, como **Spirulina** e **Chlorella**, utilizando um microcontrolador **ESP32** no ambiente Wokwi.

O sistema monitora variáveis críticas do tanque, interpreta o estado operacional e envia os dados em tempo real via **MQTT** para integração futura com uma API externa em **.NET Core**.

## Objetivo da Solução IoT

O objetivo do dispositivo IoT é monitorar as condições do tanque de microalgas e auxiliar na automação da produção de biomassa.

O ESP32 realiza:

* leitura de pH da água;
* leitura de luminosidade local;
* leitura de temperatura da água;
* leitura simulada de turbidez;
* exibição dos dados em display LCD;
* acionamento de LEDs de status;
* acionamento de servo motor para simular válvula/bomba de colheita;
* publicação de telemetria em JSON via MQTT;
* recebimento de comandos remotos via MQTT.

## Componentes Utilizados

| Componente      | Função                           |
| --------------- | -------------------------------- |
| ESP32 DevKit    | Microcontrolador principal       |
| Potenciômetro 1 | Simula sensor de pH              |
| Potenciômetro 2 | Simula sensor de turbidez        |
| LDR             | Simula luminosidade local        |
| DS18B20         | Simula temperatura da água       |
| LCD 16x2 I2C    | Exibe leituras e status          |
| LED Verde       | Indica operação normal           |
| LED Vermelho    | Indica alerta                    |
| Servo Motor     | Simula válvula/bomba de colheita |

## Mapeamento de Pinos

| Componente             | Pino ESP32 |
| ---------------------- | ---------- |
| LED Vermelho           | GPIO 25    |
| LED Verde              | GPIO 26    |
| Potenciômetro pH       | GPIO 34    |
| LDR                    | GPIO 35    |
| Sensor DS18B20         | GPIO 4     |
| Potenciômetro Turbidez | GPIO 32    |
| Servo Motor            | GPIO 27    |
| LCD SDA                | GPIO 21    |
| LCD SCL                | GPIO 22    |

## Faixas Operacionais

| Variável             | Faixa / Regra                 |
| -------------------- | ----------------------------- |
| pH ideal             | 6.5 até 8.5                   |
| Temperatura ideal    | 20°C até 30°C                 |
| Luminosidade ideal   | 250 até 900                   |
| Turbidez baixa       | menor que 100                 |
| Pronto para colheita | turbidez maior ou igual a 650 |

## Lógica de Funcionamento

O ESP32 lê os sensores periodicamente e classifica o estado do tanque.

Estados possíveis:

| Status          | Significado                                       |
| --------------- | ------------------------------------------------- |
| NORMAL          | Todas as variáveis estão dentro da faixa adequada |
| ALERTA_PH       | pH fora da faixa ideal                            |
| ALERTA_TEMP     | Temperatura fora da faixa ideal                   |
| ALERTA_LUZ      | Luminosidade fora da faixa ideal                  |
| TURBIDEZ_BAIXA  | Turbidez abaixo do mínimo esperado                |
| PRONTO_COLHEITA | Biomassa densa o suficiente para colheita         |

Quando o status é **PRONTO_COLHEITA**, o servo motor é acionado automaticamente, simulando a abertura da válvula ou bomba de colheita.

## Convenção do Servo Motor

Nesta simulação Wokwi:

| Ângulo | Significado                  |
| ------ | ---------------------------- |
| 0°     | Servo aberto / Colheita ON   |
| 90°    | Servo fechado / Colheita OFF |

## Comunicação MQTT

O dispositivo conecta-se ao broker público HiveMQ.

Broker utilizado:

```txt
broker.hivemq.com
```

Porta MQTT:

```txt
1883
```

## Tópico de Telemetria

O ESP32 publica os dados no tópico:

```txt
phycocarbon/fiap/tanque01/telemetria
```

## Exemplo de Payload JSON

```json
{
  "dispositivo_id": 10,
  "pH": 7.8,
  "temp": 22,
  "turbidez": 533,
  "luminosidade": 799,
  "status": "NORMAL",
  "pronto_colheita": false,
  "servo_aberto": false
}
```

Quando o tanque está pronto para colheita, o payload pode aparecer assim:

```json
{
  "dispositivo_id": 10,
  "pH": 7.4,
  "temp": 26.5,
  "turbidez": 680,
  "luminosidade": 799,
  "status": "PRONTO_COLHEITA",
  "pronto_colheita": true,
  "servo_aberto": true
}
```

## Tópico de Comandos

O ESP32 também recebe comandos remotos pelo tópico:

```txt
phycocarbon/fiap/tanque01/comandos
```

## Comandos MQTT Aceitos

| Comando      | Ação                                 |
| ------------ | ------------------------------------ |
| COLHER       | Abre e fecha o servo temporariamente |
| SERVO_ABRIR  | Mantém o servo aberto                |
| SERVO_FECHAR | Fecha o servo                        |

## Como Testar

1. Abrir o projeto no Wokwi.
2. Iniciar a simulação.
3. Verificar no LCD se o Wi-Fi conectou.
4. Verificar se o MQTT conectou.
5. Acessar um cliente MQTT, como o HiveMQ WebSocket Client.
6. Conectar no broker `broker.hivemq.com`.
7. Assinar o tópico `phycocarbon/fiap/tanque01/telemetria`.
8. Confirmar o recebimento do JSON.
9. Publicar comandos no tópico `phycocarbon/fiap/tanque01/comandos`.
10. Testar os comandos `COLHER`, `SERVO_ABRIR` e `SERVO_FECHAR`.

## Fluxo da Arquitetura

```txt
Sensores simulados
      ↓
ESP32
      ↓
Processamento local
      ↓
Display LCD / LEDs / Servo
      ↓
MQTT
      ↓
Broker HiveMQ
      ↓
API .NET Core / Dashboard / Mobile
```

## Integração com o Ecossistema Phycocarbon

A camada IoT representa o ponto de coleta física da solução Phycocarbon. Os dados gerados pelo ESP32 podem ser consumidos futuramente pela API .NET Core, persistidos em banco Oracle e exibidos no aplicativo mobile React Native.

Essa arquitetura permite que o sistema monitore o cultivo de microalgas em tempo real, detecte condições críticas e automatize a colheita quando a biomassa atingir o ponto ideal.
