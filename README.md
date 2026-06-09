# GeoSat — Estação IoT de Monitoramento Agrícola (ESP32)

Entrega da disciplina **Disruptive Architectures: IoT, IOB & Generative IA** — Global Solution 2026/1 (FIAP).

## Descrição da solução

O GeoSat conecta a economia espacial a problemas reais na Terra: dados de observação por satélite (clima e vegetação) orientam o manejo agrícola, enquanto esta **estação IoT de campo com ESP32** coleta as condições locais do talhão (temperatura, umidade do ar, umidade do solo e luminosidade) e permite o acionamento remoto da irrigação. A estação expõe os dados via **Wi-Fi**, por **WebServer com API REST (JSON)** e um **dashboard web**, além de exibir as leituras em um **display OLED** no próprio campo.

Alinhado aos ODS da ONU: 2 (Fome Zero e Agricultura Sustentável), 9 (Indústria, Inovação e Infraestrutura) e 13 (Ação Climática).

## Conformidade com os requisitos da disciplina

| Requisito | Como é atendido |
| :--- | :--- |
| Protótipo funcional com ESP32 | Simulação completa no Wokwi (`sketch/sketch.ino` + `diagram.json`) |
| 2 entradas (sensores, botões ou equivalentes) | DHT22 (temperatura/umidade do ar), potenciômetro (umidade do solo) e LDR (luminosidade) |
| 2 saídas (LEDs, atuadores ou equivalentes) | LED verde, LED vermelho e módulo relé (irrigação) |
| 1 interface local (LCD, OLED ou similar) | Display OLED SSD1306 128x64 (I2C) |
| Comunicação via Wi-Fi | Conexão à rede `Wokwi-GUEST` |
| WebServer, API REST e/ou MQTT | WebServer HTTP na porta 80 com API REST |
| Mínimo 3 endpoints JSON documentados | `GET /api/sensors`, `GET /api/actuators`, `POST /api/control` (documentados abaixo) |
| Dashboard/interface para visualização dos dados | Dashboard web servido pelo próprio ESP32 em `GET /` |

## Componentes de hardware

| Componente | Pino ESP32 | Função |
| :--- | :--- | :--- |
| ESP32 DevKit C v4 | — | Microcontrolador principal |
| DHT22 | GPIO 4 | Entrada: temperatura e umidade do ar |
| Potenciômetro | GPIO 34 (ADC) | Entrada: simula sensor de umidade do solo |
| LDR (fotoresistor) | GPIO 35 (ADC) | Entrada: luminosidade |
| LED Verde | GPIO 2 | Saída: solo adequado (umidade ≥ 30%), automático |
| LED Vermelho | GPIO 15 | Saída: alerta de solo seco (umidade < 30%), automático |
| Módulo Relé | GPIO 5 | Saída: acionamento da irrigação |
| OLED SSD1306 | GPIO 21 (SDA) / GPIO 22 (SCL) | Interface local (endereço I2C 0x3C) |

## API REST — Endpoints documentados

Base: `http://<IP-do-ESP32>` (o IP aparece no Serial Monitor e no OLED ao conectar).

### 1. `GET /api/sensors` — leitura dos sensores

Resposta `200 application/json`:

```json
{
  "temperature": 24.0,
  "humidity": 40.0,
  "soil_raw": 2048,
  "light_raw": 1500,
  "soil_percent": 50,
  "light_percent": 36
}
```

### 2. `GET /api/actuators` — estado atual dos atuadores

Resposta `200 application/json`:

```json
{
  "led_green": true,
  "led_red": false,
  "relay": false
}
```

### 3. `POST /api/control` — controla a irrigação (relé)

Os LEDs são automáticos (alerta de umidade do solo); o relé é o atuador de controle remoto.

Corpo da requisição (`Content-Type: application/json`):

```json
{
  "relay": true
}
```

Resposta `200 application/json` (novo estado):

```json
{
  "relay": true
}
```

Erros: `400 {"error":"Body missing"}` (sem corpo) ou `400 {"error":"Invalid JSON"}` (JSON inválido).

### Exemplos com curl

```bash
curl http://<IP>/api/sensors
curl http://<IP>/api/actuators
curl -X POST http://<IP>/api/control -H "Content-Type: application/json" -d "{\"relay\": true}"
```

## Dashboard

O endpoint `GET /` serve o dashboard web diretamente do ESP32, com:

- Leituras dos sensores atualizadas a cada 2 segundos (temperatura, umidade do ar, solo e luminosidade);
- Botões para ligar/desligar a irrigação (relé);
- Estado dos LEDs de alerta (automáticos, conforme a umidade do solo) e do relé em tempo real.

## Como executar

### Opção 1 — Site wokwi.com (sem instalar nada)

1. Acesse [wokwi.com](https://wokwi.com) e crie um novo projeto **ESP32 (Arduino)**.
2. Substitua o conteúdo do `sketch.ino` pelo arquivo `sketch/sketch.ino` deste repositório.
3. Substitua o conteúdo do `diagram.json` pelo arquivo `diagram.json` deste repositório.
4. No arquivo `libraries.txt` do projeto Wokwi, adicione as bibliotecas listadas no `libraries.txt` deste repositório.
5. Clique em **Play** para iniciar a simulação.
6. No Serial Monitor, aguarde `WiFi conectado!` e anote o IP exibido.
7. Gire o potenciômetro (umidade do solo), ajuste o LDR e o DHT22 para variar as leituras e observe o OLED e o Serial Monitor refletindo os valores.

### Opção 2 — VS Code / Cursor com a extensão Wokwi (build local)

Pré-requisitos: [arduino-cli](https://arduino.github.io/arduino-cli/) e a extensão **Wokwi Simulator**.

```bash
# Configurar o arduino-cli (uma única vez)
arduino-cli config init
arduino-cli config set board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32
arduino-cli lib install "DHT sensor library" "Adafruit GFX Library" "Adafruit SSD1306" "ArduinoJson"

# Compilar (na raiz do repositório)
arduino-cli compile --fqbn esp32:esp32:esp32 sketch --output-dir build
```

Depois, inicie a simulação pela extensão (o `wokwi.toml` já aponta para `build/sketch.ino.bin`). O `wokwi.toml` também já encaminha a porta 80 do ESP32 para a sua máquina:

- Dashboard: `http://localhost:9080/`
- API: `http://localhost:9080/api/sensors`, `/api/actuators`, `/api/control`

Observação: mantenha a aba do simulador visível — se ficar oculta, a simulação pausa.

## Estrutura do repositório

```
.
├── sketch/
│   └── sketch.ino   # Firmware do ESP32 (Wi-Fi, WebServer, API REST, OLED, dashboard)
├── diagram.json     # Circuito da simulação no Wokwi (inclui serial monitor)
├── libraries.txt    # Bibliotecas necessárias na simulação
├── wokwi.toml       # Configuração do Wokwi (firmware compilado + port forwarding)
├── integrantes.txt  # RM, nome e turma dos integrantes
├── .gitignore       # Ignora build/ (artefatos de compilação)
└── README.md        # Esta documentação
```

## Vídeo de apresentação

Vídeo (até 3 minutos) apresentando a proposta de solução e o funcionamento do sistema:

**Link:** [ADICIONAR LINK DO VÍDEO AQUI]

## Integrantes

Equipe GeoSat — 2TDSR

| RM | Nome | Turma |
| :--- | :--- | :--- |
| 562906 | Altamir Lima | 2TDSR |
| 562248 | Felipe Conte | 2TDSR |
| 564495 | Luiz Goncalves | 2TDSR |
| 563558 | Olavo Neves | 2TDSR |
| 561940 | Pedro Franca | 2TDSR |
