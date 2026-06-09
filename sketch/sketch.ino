#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ========== CONFIGURAÇÕES WI-FI ==========
const char* ssid = "Wokwi-GUEST";      // Rede do Wokwi
const char* password = "";             // Sem senha para Wokwi-GUEST

// ========== Pinos e sensores ==========
// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// DHT22
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Sensores analógicos
#define SOIL_PIN 34   // Potenciômetro (umidade do solo)
#define LDR_PIN  35   // Fotoresistor (luminosidade)

// Atuadores
#define LED_GREEN 2
#define LED_RED   15
#define RELAY_PIN 5

// Web server na porta 80
WebServer server(80);

// Variáveis globais para estado dos atuadores (persistente)
bool ledGreenState = LOW;
bool ledRedState   = LOW;
bool relayState    = LOW;

// ========== PROTÓTIPOS ==========
void handleSensors();
void handleActuators();
void handleControl();
void handleDashboard();
void updateActuators();
void readAndUpdateDisplay(); // Corrigido: Protótipo adicionado aqui

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  delay(1000);

  // Inicializa sensores e atuadores
  dht.begin();
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(LED_GREEN, ledGreenState);
  digitalWrite(LED_RED, ledRedState);
  digitalWrite(RELAY_PIN, relayState);

  // Inicializa OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("ERRO OLED");
    while (true);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Conectando WiFi...");
  display.display();

  // Conecta ao Wi-Fi
  WiFi.begin(ssid, password);
  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("WiFi OK");
    display.print("IP: ");
    display.println(WiFi.localIP());
    display.display();
    delay(2000);
  } else {
    Serial.println("Falha no WiFi. Continuando sem rede.");
    display.clearDisplay();
    display.println("WiFi falhou");
    display.display();
  }

  // Configura rotas do servidor
  server.on("/", handleDashboard);                // Página principal (dashboard)
  server.on("/api/sensors", handleSensors);       // Endpoint JSON: leitura dos sensores
  server.on("/api/actuators", handleActuators);   // Endpoint JSON: estado atual dos atuadores
  server.on("/api/control", HTTP_POST, handleControl); // Endpoint POST: controla atuadores
  server.begin();
  Serial.println("Servidor HTTP iniciado");
}

// ========== LOOP PRINCIPAL ==========
void loop() {
  server.handleClient();  // Processa requisições web

  // Leitura dos sensores a cada 2 segundos
  static unsigned long lastRead = 0;
  if (millis() - lastRead > 2000) {
    lastRead = millis();
    readAndUpdateDisplay();
  }
}

// ========== FUNÇÃO QUE LÊ SENSORES E ATUALIZA DISPLAY LOCAL ==========
void readAndUpdateDisplay() {
  float temperatura = dht.readTemperature();
  float umidadeAr = dht.readHumidity();
  int solo = analogRead(SOIL_PIN);
  int luz = analogRead(LDR_PIN);

  // Exibe no Serial
  Serial.println("========== GEO SAT ==========");
  Serial.print("Temperatura: "); Serial.print(temperatura); Serial.println(" °C");
  Serial.print("Umidade Ar: "); Serial.print(umidadeAr); Serial.println(" %");
  Serial.print("Solo (analógico): "); Serial.println(solo);
  Serial.print("Luminosidade: "); Serial.println(luz);
  Serial.print("LED Verde: "); Serial.println(ledGreenState ? "ON" : "OFF");
  Serial.print("LED Vermelho: "); Serial.println(ledRedState ? "ON" : "OFF");
  Serial.print("Relé: "); Serial.println(relayState ? "ATIVADO" : "DESLIGADO");
  Serial.println("==============================");

  // Atualiza display OLED
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("GeoSat");
  display.print("Temp: "); display.print(temperatura); display.println(" C");
  display.print("Ar: "); display.print(umidadeAr); display.println("%");
  display.print("Solo: "); display.println(solo);
  display.print("Luz: "); display.println(luz);
  display.print("LED V:"); display.print(ledGreenState ? "ON " : "OFF");
  display.print(" R:"); display.println(ledRedState ? "ON" : "OFF");
  display.print("Rele:"); display.println(relayState ? "LIG" : "DES");
  display.display();
}

// ========== ENDPOINTS JSON ==========
void handleSensors() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  int solo = analogRead(SOIL_PIN);
  int luz = analogRead(LDR_PIN);

  // Evita serializar NaN (vira null no JSON e quebra o dashboard)
  if (isnan(t)) t = 0;
  if (isnan(h)) h = 0;

  StaticJsonDocument<256> doc;
  doc["temperature"] = t;
  doc["humidity"] = h;
  doc["soil_raw"] = solo;
  doc["light_raw"] = luz;
  doc["soil_percent"] = map(solo, 0, 4095, 0, 100);
  doc["light_percent"] = map(luz, 0, 4095, 0, 100);

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleActuators() {
  StaticJsonDocument<128> doc;
  doc["led_green"] = ledGreenState;
  doc["led_red"] = ledRedState;
  doc["relay"] = relayState;
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleControl() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"Body missing\"}");
    return;
  }
  String body = server.arg("plain");
  StaticJsonDocument<128> doc;
  DeserializationError error = deserializeJson(doc, body);
  if (error) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  // Atualiza variáveis se os campos existirem
  if (doc.containsKey("led_green")) ledGreenState = doc["led_green"];
  if (doc.containsKey("led_red")) ledRedState = doc["led_red"];
  if (doc.containsKey("relay")) relayState = doc["relay"];

  // Aplica aos pinos físicos
  updateActuators();

  // Resposta com novo estado
  StaticJsonDocument<128> resDoc;
  resDoc["led_green"] = ledGreenState;
  resDoc["led_red"] = ledRedState;
  resDoc["relay"] = relayState;
  String response;
  serializeJson(resDoc, response);
  server.send(200, "application/json", response);
}

void updateActuators() {
  digitalWrite(LED_GREEN, ledGreenState);
  digitalWrite(LED_RED, ledRedState);
  digitalWrite(RELAY_PIN, relayState);
}

// ========== DASHBOARD HTML (página principal) ==========
void handleDashboard() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <title>GeoSat Dashboard</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; margin: 20px; background: #f0f0f0; }
    h1 { color: #2c3e50; }
    .card { background: white; border-radius: 10px; padding: 15px; margin-bottom: 20px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }
    .sensor { font-size: 1.2em; margin: 5px 0; }
    .actuator { display: inline-block; margin: 10px; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; font-size: 1em; }
    .btn-on { background-color: #2ecc71; color: white; }
    .btn-off { background-color: #e74c3c; color: white; }
    .status { font-weight: bold; }
    table { width: 100%; border-collapse: collapse; }
    td { padding: 8px; border-bottom: 1px solid #ddd; }
    .label { font-weight: bold; width: 40%; }
  </style>
</head>
<body>
  <h1>🌱 GeoSat - Estação de Monitoramento</h1>
  <div class="card">
    <h2>📡 Sensores</h2>
    <table id="sensorTable">
      <tr><td class="label">Temperatura:</td><td id="temp">--</td><td>°C</td></tr>
      <tr><td class="label">Umidade do Ar:</td><td id="hum">--</td><td>%</td></tr>
      <tr><td class="label">Umidade do Solo (cru):</td><td id="soil">--</td><td></td></tr>
      <tr><td class="label">Luminosidade (cru):</td><td id="light">--</td><td></td></tr>
      <tr><td class="label">Solo (% estimado):</td><td id="soilPerc">--</td><td>%</td></tr>
      <tr><td class="label">Luz (% estimado):</td><td id="lightPerc">--</td><td>%</td></tr>
    </table>
  </div>
  <div class="card">
    <h2>⚙️ Atuadores</h2>
    <div>
      <button id="btnGreenOn" class="actuator btn-on">LED Verde ON</button>
      <button id="btnGreenOff" class="actuator btn-off">LED Verde OFF</button>
      <button id="btnRedOn" class="actuator btn-on">LED Vermelho ON</button>
      <button id="btnRedOff" class="actuator btn-off">LED Vermelho OFF</button>
      <button id="btnRelayOn" class="actuator btn-on">Relé ON</button>
      <button id="btnRelayOff" class="actuator btn-off">Relé OFF</button>
    </div>
    <h3>Estado atual</h3>
    <table>
      <tr><td class="label">LED Verde:</td><td id="ledGreenStatus" class="status">--</td></tr>
      <tr><td class="label">LED Vermelho:</td><td id="ledRedStatus" class="status">--</td></tr>
      <tr><td class="label">Relé:</td><td id="relayStatus" class="status">--</td></tr>
    </table>
  </div>
  <script>
    function fetchSensors() {
      fetch('/api/sensors')
        .then(res => res.json())
        .then(data => {
          document.getElementById('temp').innerText = data.temperature.toFixed(1);
          document.getElementById('hum').innerText = data.humidity.toFixed(1);
          document.getElementById('soil').innerText = data.soil_raw;
          document.getElementById('light').innerText = data.light_raw;
          document.getElementById('soilPerc').innerText = data.soil_percent;
          document.getElementById('lightPerc').innerText = data.light_percent;
        })
        .catch(err => console.error('Erro sensores:', err));
    }

    function fetchActuators() {
      fetch('/api/actuators')
        .then(res => res.json())
        .then(data => {
          document.getElementById('ledGreenStatus').innerText = data.led_green ? 'LIGADO' : 'DESLIGADO';
          document.getElementById('ledRedStatus').innerText = data.led_red ? 'LIGADO' : 'DESLIGADO';
          document.getElementById('relayStatus').innerText = data.relay ? 'ATIVADO' : 'DESLIGADO';
        });
    }

    function sendControl(led_green, led_red, relay) {
      const body = { led_green, led_red, relay };
      fetch('/api/control', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(body)
      })
        .then(res => res.json())
        .then(data => {
          fetchActuators();
        })
        .catch(err => console.error('Erro controle:', err));
    }

    // Corrigido: Botões agora preservam o estado atual no envio do JSON em vez de mandar null
    document.getElementById('btnGreenOn').onclick = () => sendControl(true, document.getElementById('ledRedStatus').innerText === 'LIGADO', document.getElementById('relayStatus').innerText === 'ATIVADO');
    document.getElementById('btnGreenOff').onclick = () => sendControl(false, document.getElementById('ledRedStatus').innerText === 'LIGADO', document.getElementById('relayStatus').innerText === 'ATIVADO');
    document.getElementById('btnRedOn').onclick = () => sendControl(document.getElementById('ledGreenStatus').innerText === 'LIGADO', true, document.getElementById('relayStatus').innerText === 'ATIVADO');
    document.getElementById('btnRedOff').onclick = () => sendControl(document.getElementById('ledGreenStatus').innerText === 'LIGADO', false, document.getElementById('relayStatus').innerText === 'ATIVADO');
    document.getElementById('btnRelayOn').onclick = () => sendControl(document.getElementById('ledGreenStatus').innerText === 'LIGADO', document.getElementById('ledRedStatus').innerText === 'LIGADO', true);
    document.getElementById('btnRelayOff').onclick = () => sendControl(document.getElementById('ledGreenStatus').innerText === 'LIGADO', document.getElementById('ledRedStatus').innerText === 'LIGADO', false);

    setInterval(() => {
      fetchSensors();
      fetchActuators();
    }, 2000);
    fetchSensors();
    fetchActuators();
  </script>
</body>
</html>
  )rawliteral";
  server.send(200, "text/html; charset=utf-8", html);
}