# Phycocarbon IoT — Biofotorreator Inteligente com ESP32

## Visão Geral

O projeto **Phycocarbon IoT** simula um biofotorreator automatizado para cultivo de microalgas, como **Spirulina** e **Chlorella**, utilizando um microcontrolador **ESP32** no ambiente **Wokwi**.

A solução monitora variáveis críticas do tanque, interpreta o estado operacional, exibe informações no LCD, aciona LEDs/servo motor e envia dados em tempo real via **MQTT** para integração com a API **.NET**, responsável por consumir a telemetria e persistir os dados no banco **Oracle**.

Este módulo representa a camada de borda do ecossistema Phycocarbon, conectando sensores simulados, atuadores e comunicação assíncrona com o restante da arquitetura.

---

## Objetivo da Solução IoT

O objetivo do dispositivo IoT é monitorar as condições de um tanque de microalgas e auxiliar na automação da produção de biomassa.

O ESP32 realiza:

- leitura simulada de pH da água;
- leitura de luminosidade local;
- leitura de temperatura da água;
- leitura simulada de turbidez;
- exibição das leituras em display LCD;
- acionamento de LED verde para operação normal;
- acionamento de LED vermelho para alerta;
- acionamento de servo motor para simular válvula/bomba de colheita;
- publicação de telemetria em JSON via MQTT;
- publicação de alertas críticos via MQTT;
- recebimento de comandos remotos via MQTT.

---

## Componentes Utilizados

| Componente | Função |
| --- | --- |
| ESP32 DevKit | Microcontrolador principal |
| Potenciômetro 1 | Simula sensor de pH |
| Potenciômetro 2 | Simula sensor de turbidez |
| LDR | Simula luminosidade local |
| DS18B20 | Simula temperatura da água |
| LCD 16x2 I2C | Exibe leituras e status do tanque |
| LED Verde | Indica operação normal |
| LED Vermelho | Indica alerta operacional |
| Servo Motor | Simula válvula/bomba de colheita |

---

## Mapeamento de Pinos

| Componente | Pino ESP32 |
| --- | --- |
| LED Vermelho | GPIO 25 |
| LED Verde | GPIO 26 |
| Potenciômetro pH | GPIO 34 |
| LDR | GPIO 35 |
| Sensor DS18B20 | GPIO 4 |
| Potenciômetro Turbidez | GPIO 32 |
| Servo Motor | GPIO 27 |
| LCD SDA | GPIO 21 |
| LCD SCL | GPIO 22 |

---

## Identificação do Dispositivo

Este protótipo representa um dispositivo IoT vinculado a uma fazenda e a um tanque cadastrados no banco Oracle.

| Campo | Valor |
| --- | --- |
| ID Fazenda | 5 |
| ID Tanque | 10 |
| ID Dispositivo | 10 |

Esses IDs são usados para permitir que a API .NET relacione a origem da telemetria com o tanque correto no banco de dados.

---

## Faixas Operacionais

As faixas utilizadas no código foram alinhadas ao comportamento esperado para um tanque de cultivo de microalgas.

| Variável | Faixa / Regra |
| --- | --- |
| pH ideal | 6.5 até 8.5 |
| Temperatura ideal | 18°C até 28°C |
| Luminosidade ideal | 3000 até 15000 |
| Turbidez baixa | menor que 100 |
| Pronto para colheita | turbidez maior ou igual a 650 |

A luminosidade é simulada em uma escala proporcional de **0 a 15000**, aproximando o comportamento esperado para uma leitura em lux.

---

## Lógica de Funcionamento

O ESP32 lê os sensores periodicamente e classifica o estado do tanque.

Estados possíveis:

| Status | Significado |
| --- | --- |
| NORMAL | Todas as variáveis estão dentro da faixa ideal |
| PH_CRITICO | pH extremamente fora da faixa segura |
| PH_BAIXO | pH abaixo da faixa ideal |
| PH_ALTO | pH acima da faixa ideal |
| TEMPERATURA_ALTA | Temperatura acima da faixa ideal |
| TEMPERATURA_BAIXA | Temperatura abaixo da faixa ideal |
| LUMINOSIDADE_BAIXA | Luminosidade abaixo do mínimo esperado |
| TURBIDEZ_FORA_PADRAO | Turbidez abaixo do mínimo esperado |
| PRONTO_COLHEITA | Biomassa densa o suficiente para colheita |

Quando o status é **PRONTO_COLHEITA**, o servo motor pode ser acionado automaticamente, simulando a abertura da válvula ou bomba de colheita.

---

## Convenção do Servo Motor

Nesta simulação Wokwi:

| Ângulo | Significado |
| --- | --- |
| 0° | Servo aberto / Colheita ON |
| 90° | Servo fechado / Colheita OFF |

---

## Comunicação MQTT

O dispositivo conecta-se ao broker público HiveMQ.

| Configuração | Valor |
| --- | --- |
| Broker | `broker.hivemq.com` |
| Porta | `1883` |
| Client ID base | `phycocarbon-esp32-tanque01` |

---

## Tópicos MQTT

O projeto utiliza três tópicos MQTT principais:

| Tópico | Tipo | Finalidade |
| --- | --- | --- |
| `phycocarbon/fiap/tanque01/telemetria` | Publicação | Envio das métricas do tanque |
| `phycocarbon/fiap/tanque01/comandos` | Assinatura | Recebimento de comandos remotos |
| `phycocarbon/fiap/tanque01/alertas` | Publicação | Envio de alertas críticos |

> Importante: os testes MQTT devem usar os tópicos acima. Eles são os mesmos tópicos definidos no arquivo `Dispositivo-IoT.ino`.

---

## Payload de Telemetria

O ESP32 publica telemetria no tópico:

```txt
phycocarbon/fiap/tanque01/telemetria
```

Exemplo de payload em estado normal:

```json
{
  "dispositivo_id": 10,
  "pH": 7.4,
  "temp": 24.5,
  "turbidez": 533,
  "luminosidade": 12000,
  "status": "NORMAL",
  "pronto_colheita": false,
  "servo_aberto": false
}
```

Exemplo de payload quando o tanque está pronto para colheita:

```json
{
  "dispositivo_id": 10,
  "pH": 7.4,
  "temp": 26.5,
  "turbidez": 680,
  "luminosidade": 12500,
  "status": "PRONTO_COLHEITA",
  "pronto_colheita": true,
  "servo_aberto": true
}
```

---

## Comandos MQTT

O ESP32 recebe comandos remotos pelo tópico:

```txt
phycocarbon/fiap/tanque01/comandos
```

O dispositivo aceita comandos em texto puro ou JSON.

Exemplo em texto puro:

```txt
ABRIR_SERVO
```

Exemplo em JSON:

```json
{
  "comando": "ABRIR_SERVO"
}
```

### Lista de Comandos Aceitos

| Comando | Ação |
| --- | --- |
| `COLHER` | Abre e fecha o servo temporariamente |
| `LIGAR_BOMBA` | Abre e fecha o servo temporariamente |
| `ACIONAR_COLHEITA` | Abre e fecha o servo temporariamente |
| `ABRIR_SERVO` | Mantém o servo aberto |
| `SERVO_ABRIR` | Mantém o servo aberto |
| `ABRIR` | Mantém o servo aberto |
| `FECHAR_SERVO` | Fecha o servo |
| `SERVO_FECHAR` | Fecha o servo |
| `FECHAR` | Fecha o servo |

---

## Payload de Alerta

O ESP32 publica alertas críticos no tópico:

```txt
phycocarbon/fiap/tanque01/alertas
```

Esse tópico é usado quando alguma variável do tanque sai da faixa ideal, como pH crítico, temperatura inadequada, baixa luminosidade ou turbidez fora do padrão.

Exemplo de payload de alerta:

```json
{
  "id_dispositivo": 10,
  "id_tanque": 10,
  "id_fazenda": 5,
  "tipo_alerta": "PH_BAIXO",
  "severidade": "ATENCAO",
  "status_alerta": "ABERTO",
  "mensagem": "pH abaixo da faixa ideal.",
  "pH": 5.8,
  "temp": 24.5,
  "turbidez": 420,
  "luminosidade": 11000,
  "pronto_colheita": false,
  "servo_aberto": false
}
```

---

## Tipos de Alerta

| Tipo de Alerta | Descrição |
| --- | --- |
| `PH_CRITICO` | pH em faixa crítica |
| `PH_BAIXO` | pH abaixo da faixa ideal |
| `PH_ALTO` | pH acima da faixa ideal |
| `TEMPERATURA_ALTA` | Temperatura acima da faixa ideal |
| `TEMPERATURA_BAIXA` | Temperatura abaixo da faixa ideal |
| `LUMINOSIDADE_BAIXA` | Luminosidade abaixo do mínimo esperado |
| `TURBIDEZ_FORA_PADRAO` | Turbidez abaixo do padrão esperado |

---

## Bibliotecas Utilizadas

O projeto utiliza as seguintes bibliotecas no Arduino/Wokwi:

```cpp
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP32Servo.h>
#include <math.h>
```

---

## Como Testar no Wokwi

1. Abra o projeto no Wokwi.
2. Inicie a simulação.
3. Verifique no LCD se o Wi-Fi conectou.
4. Verifique no Serial Monitor se o MQTT conectou.
5. Acesse um cliente MQTT, como o HiveMQ WebSocket Client.
6. Conecte no broker `broker.hivemq.com`.
7. Assine o tópico de telemetria:

```txt
phycocarbon/fiap/tanque01/telemetria
```

8. Confirme o recebimento do JSON de telemetria.
9. Assine o tópico de alertas:

```txt
phycocarbon/fiap/tanque01/alertas
```

10. Altere os sensores simulados para forçar um alerta.
11. Publique comandos no tópico:

```txt
phycocarbon/fiap/tanque01/comandos
```

12. Teste os comandos:

```txt
COLHER
ABRIR_SERVO
FECHAR_SERVO
ACIONAR_COLHEITA
```

Também é possível testar comandos em JSON:

```json
{
  "comando": "ABRIR_SERVO"
}
```

---

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
API .NET
      ↓
Banco Oracle
      ↓
API Java / Mobile React Native / Dashboard
```

---

## Integração com o Ecossistema Phycocarbon

A camada IoT é responsável pela coleta física dos dados do biofotorreator.

Os dados gerados pelo ESP32 podem ser consumidos pela API .NET, persistidos no banco Oracle e exibidos no aplicativo mobile React Native.

Essa arquitetura permite que o sistema:

- monitore o cultivo de microalgas em tempo real;
- detecte condições críticas de operação;
- registre histórico de métricas no banco;
- acione alertas preventivos;
- automatize a colheita quando a biomassa atingir o ponto ideal;
- forneça dados para dashboards e futuras análises preditivas.

---

## Arquivos do Projeto

| Arquivo | Descrição |
| --- | --- |
| `Dispositivo-IoT.ino` | Código principal do ESP32 |
| `diagram.json` | Diagrama do circuito no Wokwi |
| `wokwi.toml` | Configuração do projeto Wokwi |
| `README.md` | Documentação do projeto |

---

## Integrantes da Equipe

| Nome | RM | Turma | GitHub | LinkedIn |
| --- | --- | --- | --- | --- |
| Alexander Dennis Isidro Mamani | 565554 | 2TDSPG | [alex-isidro](https://github.com/alex-isidro) | [LinkedIn](https://www.linkedin.com/in/alexander-dennis-a3b48824b/) |
| Arthur Brito da Silva | 562085 | 2TDSPG | [thubrito](https://github.com/thubrito) | [LinkedIn](https://www.linkedin.com/in/arthurbritodasilva/) |
| Kelson Zhang | 563748 | 2TDSPG | [KelsonZh0](https://github.com/KelsonZh0) | [LinkedIn](https://www.linkedin.com/in/kelson-zhang-211456323/) |
| Luiz Felipe Flosi dos Santos | 563197 | 2TDSPG | [felipeflosii](https://github.com/felipeflosii) | [LinkedIn](https://www.linkedin.com/in/felipeflosii/) |
| Pedro Henrique Brum Lopes | 561780 | 2TDSPG | [PedroBrum-DEV](https://github.com/PedroBrum-DEV) | [LinkedIn](https://www.linkedin.com/in/pedro-brum-66a31b326/) |

---

## Licença

Projeto desenvolvido exclusivamente para fins acadêmicos na Global Solution FIAP 2026.
