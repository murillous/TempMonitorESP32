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
  <html lang="pt-BR">
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Monitor de Temperatura</title>
    <style>
      body {
        font-family: Arial, sans-serif;
        background-color: #ecfeff;
        color: #164e63;
        padding: 20px;
        text-align: center;
        max-width: 800px;
        margin: 0 auto;
      }
      
      h2 {
        color: #0891b2;
        font-size: 2rem;
        margin-bottom: 1.5rem;
        position: relative;
        padding-bottom: 10px;
      }
      
      .sensors-container {
        display: flex;
        flex-wrap: wrap;
        justify-content: center;
        gap: 20px;
      }
      
      .sensor {
        flex: 1;
        min-width: 250px;
        padding: 20px;
        border-radius: 15px;
        background: white;
        box-shadow: 0 5px 15px rgba(6, 182, 212, 0.2);
        margin-bottom: 20px;
        position: relative;
      }
      
      h3 {
        color: #0891b2;
        font-size: 1.3rem;
        margin-bottom: 15px;
      }
      
      .temperature {
        display: flex;
        justify-content: space-between;
        margin: 10px 0;
        padding: 10px;
        border-radius: 8px;
        background-color: #f0fdff;
      }
      
      .temperature-value {
        font-size: 1.5rem;
        font-weight: bold;
        color: #0891b2;
      }
      
      .temperature-unit {
        font-size: 0.9rem;
        color: #164e63;
      }
      
      .wave {
        position: absolute;
        bottom: 0;
        left: 0;
        width: 100%;
        height: 8px;
        background: linear-gradient(to right, #0891b2, #22d3ee, #0891b2);
        border-radius: 0 0 15px 15px;
      }
      
      @media (max-width: 600px) {
        .sensors-container {
          flex-direction: column;
        }
        
        .sensor {
          width: 100%;
        }
      }
    </style>
  </head>
  <body>
    <h2>Monitor de Temperatura</h2>
    
    <div class="sensors-container">
      <div class="sensor">
        <h3>Sensor 1</h3>
        <div class="temperature">
          <span>Celsius</span>
          <div>
            <span class="temperature-value" id="temp1c">%TEMP1C%</span>
            <span class="temperature-unit">°C</span>
          </div>
        </div>
        <div class="temperature">
          <span>Fahrenheit</span>
          <div>
            <span class="temperature-value" id="temp1f">%TEMP1F%</span>
            <span class="temperature-unit">°F</span>
          </div>
        </div>
        <div class="wave"></div>
      </div>
  
      <div class="sensor">
        <h3>Sensor 2</h3>
        <div class="temperature">
          <span>Celsius</span>
          <div>
            <span class="temperature-value" id="temp2c">%TEMP2C%</span>
            <span class="temperature-unit">°C</span>
          </div>
        </div>
        <div class="temperature">
          <span>Fahrenheit</span>
          <div>
            <span class="temperature-value" id="temp2f">%TEMP2F%</span>
            <span class="temperature-unit">°F</span>
          </div>
        </div>
        <div class="wave"></div>
      </div>
    </div>
  
    <script>
      function updateData() {
        fetch('/data')
          .then(function(response) {
            return response.json();
          })
          .then(function(data) {
            document.getElementById("temp1c").textContent = data.t1c;
            document.getElementById("temp1f").textContent = data.t1f;
            document.getElementById("temp2c").textContent = data.t2c;
            document.getElementById("temp2f").textContent = data.t2f;
          })
          .catch(function(error) {
            console.log('Erro ao atualizar dados:', error);
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