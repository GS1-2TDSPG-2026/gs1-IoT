# Phycocarbon IoT — Biofotorreator Inteligente com ESP32

## Visão Geral

O projeto **Phycocarbon IoT** simula um biofotorreator automatizado para cultivo de microalgas, como **Spirulina** e **Chlorella**, utilizando um microcontrolador **ESP32** no ambiente **Wokwi**.

O sistema monitora variáveis críticas do tanque, interpreta o estado operacional e envia os dados em tempo real via **MQTT** para integração com uma API externa em **.NET Core**, responsável por consumir a telemetria e persistir os dados no banco **Oracle**.

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
* publicação de alertas críticos via MQTT;
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

## Identificação do Dispositivo

Este protótipo representa o dispositivo IoT vinculado a uma fazenda e a um tanque cadastrados no banco Oracle.

| Campo          | Valor |
| -------------- | ----- |
| ID Fazenda     | 5     |
| ID Tanque      | 10    |
| ID Dispositivo | 10    |

Esses IDs são enviados no payload MQTT para permitir que a API .NET identifique corretamente a origem da telemetria.

## Faixas Operacionais

As faixas utilizadas no código foram alinhadas ao tanque de microalgas cadastrado no banco.

| Variável             | Faixa / Regra                 |
| -------------------- | ----------------------------- |
| pH ideal             | 6.5 até 8.5                   |
| Temperatura ideal    | 18°C até 28°C                 |
| Luminosidade ideal   | 3000 até 15000                |
| Turbidez baixa       | menor que 100                 |
| Pronto para colheita | turbidez maior ou igual a 650 |

A luminosidade é simulada em uma escala proporcional de **0 a 15000**, aproximando o comportamento esperado para uma leitura em lux no banco de dados.

## Lógica de Funcionamento

O ESP32 lê os sensores periodicamente e classifica o estado do tanque.

Estados possíveis:

| Status               | Significado                                    |
| -------------------- | ---------------------------------------------- |
| NORMAL               | Todas as variáveis estão dentro da faixa ideal |
| PH_CRITICO           | pH extremamente fora da faixa segura           |
| PH_BAIXO             | pH abaixo da faixa ideal                       |
| PH_ALTO              | pH acima da faixa ideal                        |
| TEMPERATURA_ALTA     | Temperatura acima da faixa ideal               |
| TEMPERATURA_BAIXA    | Temperatura abaixo da faixa ideal              |
| LUMINOSIDADE_BAIXA   | Luminosidade abaixo do mínimo esperado         |
| TURBIDEZ_FORA_PADRAO | Turbidez abaixo do mínimo esperado             |
| PRONTO_COLHEITA      | Biomassa densa o suficiente para colheita      |

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

## Tópicos MQTT

O projeto utiliza três tópicos MQTT principais:

| Tipo       | Tópico                                   | Função                                    |
| ---------- | ---------------------------------------- | ----------------------------------------- |
| Telemetria | `algaspace/fazenda/5/tanque/10/metricas` | Envio periódico das leituras dos sensores |
| Comandos   | `algaspace/fazenda/5/tanque/10/comandos` | Recebimento de comandos remotos           |
| Alertas    | `algaspace/fazenda/5/tanque/10/alertas`  | Publicação de eventos críticos detectados |

O tópico de telemetria está alinhado com o campo `topico_mqtt` da tabela `TB_DISPOSITIVO_IOT`.

## Tópico de Telemetria

O ESP32 publica os dados no tópico:

```txt
algaspace/fazenda/5/tanque/10/metricas
```

## Exemplo de Payload de Telemetria

```json
{
  "idDispositivo": 10,
  "idTanque": 10,
  "idFazenda": 5,
  "ph": 7.4,
  "temperatura": 24.5,
  "turbidez": 533,
  "luminosidade": 12000,
  "status": "NORMAL",
  "prontoColheita": false,
  "servoAberto": false
}
```

Quando o tanque está pronto para colheita, o payload pode aparecer assim:

```json
{
  "idDispositivo": 10,
  "idTanque": 10,
  "idFazenda": 5,
  "ph": 7.4,
  "temperatura": 26.5,
  "turbidez": 680,
  "luminosidade": 12500,
  "status": "PRONTO_COLHEITA",
  "prontoColheita": true,
  "servoAberto": true
}
```

## Tópico de Comandos

O ESP32 recebe comandos remotos pelo tópico:

```txt
algaspace/fazenda/5/tanque/10/comandos
```

## Comandos MQTT Aceitos

O dispositivo aceita comandos em texto puro ou em JSON.

### Exemplo em texto puro

```txt
COLHER
```

### Exemplo em JSON

```json
{
  "comando": "COLHER"
}
```

## Lista de Comandos

| Comando      | Ação                                 |
| ------------ | ------------------------------------ |
| COLHER       | Abre e fecha o servo temporariamente |
| LIGAR_BOMBA  | Abre e fecha o servo temporariamente |
| SERVO_ABRIR  | Mantém o servo aberto                |
| ABRIR        | Mantém o servo aberto                |
| SERVO_FECHAR | Fecha o servo                        |
| FECHAR       | Fecha o servo                        |

## Tópico de Alertas

O ESP32 publica alertas críticos no tópico:

```txt
algaspace/fazenda/5/tanque/10/alertas
```

Esse tópico é usado quando alguma variável do tanque sai da faixa ideal, como pH crítico, temperatura inadequada, baixa luminosidade ou turbidez fora do padrão.

## Exemplo de Payload de Alerta

```json
{
  "idDispositivo": 10,
  "idTanque": 10,
  "idFazenda": 5,
  "tipoAlerta": "PH_BAIXO",
  "severidade": "ALTA",
  "statusAlerta": "ABERTO",
  "mensagem": "pH abaixo da faixa ideal do tanque",
  "ph": 5.8,
  "temperatura": 24.5,
  "turbidez": 420,
  "luminosidade": 11000,
  "prontoColheita": false,
  "servoAberto": false
}
```

## Tipos de Alerta

Os tipos de alerta seguem a mesma nomenclatura prevista no banco de dados.

| Tipo de Alerta       | Descrição                              |
| -------------------- | -------------------------------------- |
| PH_CRITICO           | pH em faixa crítica                    |
| PH_BAIXO             | pH abaixo da faixa ideal               |
| PH_ALTO              | pH acima da faixa ideal                |
| TEMPERATURA_ALTA     | Temperatura acima da faixa ideal       |
| TEMPERATURA_BAIXA    | Temperatura abaixo da faixa ideal      |
| LUMINOSIDADE_BAIXA   | Luminosidade abaixo do mínimo esperado |
| TURBIDEZ_FORA_PADRAO | Turbidez abaixo do padrão esperado     |

## Bibliotecas Utilizadas

O projeto utiliza as seguintes bibliotecas:

```cpp
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP32Servo.h>
```

## Como Testar

1. Abrir o projeto no Wokwi.
2. Iniciar a simulação.
3. Verificar no LCD se o Wi-Fi conectou.
4. Verificar se o MQTT conectou.
5. Acessar um cliente MQTT, como o HiveMQ WebSocket Client.
6. Conectar no broker `broker.hivemq.com`.
7. Assinar o tópico de telemetria:

```txt
algaspace/fazenda/5/tanque/10/metricas
```

8. Confirmar o recebimento do JSON.
9. Assinar o tópico de alertas:

```txt
algaspace/fazenda/5/tanque/10/alertas
```

10. Alterar os sensores simulados para forçar um alerta.
11. Publicar comandos no tópico:

```txt
algaspace/fazenda/5/tanque/10/comandos
```

12. Testar os comandos:

```txt
COLHER
SERVO_ABRIR
SERVO_FECHAR
```

Também é possível testar comandos em JSON:

```json
{
  "comando": "SERVO_ABRIR"
}
```

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
API .NET Core
      ↓
Banco Oracle
      ↓
API Java / Mobile React Native / Dashboard
```

## Integração com o Ecossistema Phycocarbon

A camada IoT é responsável pela coleta física dos dados do biofotorreator.

Os dados gerados pelo ESP32 podem ser consumidos pela API .NET Core, persistidos no banco Oracle e exibidos no aplicativo mobile React Native.

Essa arquitetura permite que o sistema:

* monitore o cultivo de microalgas em tempo real;
* detecte condições críticas de operação;
* registre histórico de métricas no banco;
* acione alertas preventivos;
* automatize a colheita quando a biomassa atingir o ponto ideal;
* forneça dados para dashboards e futuras análises preditivas.


## 👥 Integrantes da Equipe

| Nome                           | RM     | Turma  | GitHub                                            | LinkedIn                                                            |
| ------------------------------ | ------ | ------ | ------------------------------------------------- | ------------------------------------------------------------------- |
| Alexander Dennis Isidro Mamani | 565554 | 2TDSPG | [alex-isidro](https://github.com/alex-isidro)     | [LinkedIn](https://www.linkedin.com/in/alexander-dennis-a3b48824b/) |
| Arthur Brito da Silva          | 562085 | 2TDSPG | [thubrito](https://github.com/thubrito)           | [LinkedIn](https://www.linkedin.com/in/arthurbritodasilva/)         |
| Kelson Zhang                   | 563748 | 2TDSPG | [KelsonZh0](https://github.com/KelsonZh0)         | [LinkedIn](https://www.linkedin.com/in/kelson-zhang-211456323/)     |
| Luiz Felipe Flosi dos Santos   | 563197 | 2TDSPG | [felipeflosii](https://github.com/felipeflosii)   | [LinkedIn](https://www.linkedin.com/in/felipeflosii/)               |
| Pedro Henrique Brum Lopes      | 561780 | 2TDSPG | [PedroBrum-DEV](https://github.com/PedroBrum-DEV) | [LinkedIn](https://www.linkedin.com/in/pedro-brum-66a31b326/)       |

---

