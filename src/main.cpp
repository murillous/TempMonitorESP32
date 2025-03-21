#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Configuração dos sensores
#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Quantidade de sensores
const int numSensors = 2;
DeviceAddress sensorAddress[2];  // Armazenar endereços dos sensores

// Variáveis de temperatura
char tempC1[8] = "--";
char tempC2[8] = "--";
char tempF1[8] = "--";
char tempF2[8] = "--";

// Configuração temporal
unsigned long lastTime = 0;
const unsigned long timerDelay = 10000;

// Wi-Fi
const char* ssid = "ESP32_EXPOLEA";
const char* password = "12345678";
AsyncWebServer server(80);

void printAddress(DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++) {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

void setupSensors() {
  sensors.begin();
  Serial.println();
  Serial.println("Localizando sensores...");
  
  if (sensors.getDeviceCount() < numSensors) {
    Serial.println("Sensores insuficientes detectados!");
    while (1);
  }

  // Armazena endereços dos sensores
  sensors.getAddress(sensorAddress[0], 0);
  sensors.getAddress(sensorAddress[1], 1);

  Serial.println("Sensores encontrados:");
  Serial.print("Sensor 1: ");
  printAddress(sensorAddress[0]);
  Serial.println();
  Serial.print("Sensor 2: ");
  printAddress(sensorAddress[1]);
  Serial.println();
}

void updateTemperatures() {
  sensors.requestTemperatures();
  
  float t1 = sensors.getTempC(sensorAddress[0]);
  float t2 = sensors.getTempC(sensorAddress[1]);

  // Verifica se os sensores retornam valores válidos
  if (t1 == DEVICE_DISCONNECTED_C) {
    strcpy(tempC1, "N/A");
    strcpy(tempF1, "N/A");
  } else {
    dtostrf(t1, 6, 2, tempC1);
    dtostrf(sensors.toFahrenheit(t1), 6, 2, tempF1);
  }
  
  if (t2 == DEVICE_DISCONNECTED_C) {
    strcpy(tempC2, "N/A");
    strcpy(tempF2, "N/A");
  } else {
    dtostrf(t2, 6, 2, tempC2);
    dtostrf(sensors.toFahrenheit(t2), 6, 2, tempF2);
  }

  Serial.printf("Sensor 1: %s°C | %s°F\n", tempC1, tempF1);
  Serial.printf("Sensor 2: %s°C | %s°F\n", tempC2, tempF2);
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Monitor de Temperatura</title>
  <style>
    /* Estilos simplificados sem fontes externas */
    body {
      margin: 20px;
      font-family: Arial, sans-serif;
      text-align: center;
    }
    h2 {
      color: #059e8a;
    }
    .sensor {
      display: inline-block;
      margin: 20px;
      padding: 20px;
      border: 2px solid #059e8a;
      border-radius: 10px;
    }
  </style>
</head>
<body>
  <h2>Monitor de Temperatura</h2>
  
  <div class="sensor">
    <h3>Sensor 1</h3>
    <p>🌡️ <span id="temp1c">%TEMP1C%</span>°C</p>
    <p>🌡️ <span id="temp1f">%TEMP1F%</span>°F</p>
  </div>

  <div class="sensor">
    <h3>Sensor 2</h3>
    <p>🌡️ <span id="temp2c">%TEMP2C%</span>°C</p>
    <p>🌡️ <span id="temp2f">%TEMP2F%</span>°F</p>
  </div>

  <script>
    // Script de atualização mantido
    function updateData() {
      fetch('/data')
        .then(r => r.json())
        .then(data => {
          document.getElementById("temp1c").textContent = data.t1c;
          document.getElementById("temp1f").textContent = data.t1f;
          document.getElementById("temp2c").textContent = data.t2c;
          document.getElementById("temp2f").textContent = data.t2f;
        });
    }
    setInterval(updateData, 10000);
    updateData();
  </script>
</body>
</html> 
)rawliteral";  

// Função para processar as tags no HTML
String processor(const String& var) {
  if (var == "TEMP1C") return tempC1;
  if (var == "TEMP1F") return tempF1;
  if (var == "TEMP2C") return tempC2;
  if (var == "TEMP2F") return tempF2;
  return String();
}

void setup() {
  Serial.begin(115200);
  setupSensors();

  WiFi.softAP(ssid, password);
  
  Serial.println();
  Serial.print("IP do AP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = String("{") +
      "\"t1c\":\"" + tempC1 + "\"," +
      "\"t1f\":\"" + tempF1 + "\"," +
      "\"t2c\":\"" + tempC2 + "\"," +
      "\"t2f\":\"" + tempF2 + "\"" +
      "}";
    request->send(200, "application/json", json);
  });

  server.begin();
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    updateTemperatures();
    lastTime = millis();
  }
}