#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// Configuração dos pinos
#define PINO_DS18B20 4
#define pinoPeltier 5


// Configurações WiFi
const char* ssid = "ESP32";
const char* password = "12345678";

// Configurações de temperatura
float TEMP_ALVO = 4.0;
const float TEMP_TOLERANCE = 0.2; 
const int MAX_SAIDA_PELTIER = 255;
const int MIN_SAIDA_PELTIER = 0;

// Controle PID
float kp = 100.0;
float ki = 0.5;
float kd = 50.0;

float erro_anterior = 0;
float integral = 0;
unsigned long tempo_anterior = 0;

// Estados do sistema
enum EstadoSistema {
  RESFRIAMENTO_INICIAL,
  CONTROLE_PID,
  ESTABILIZADO
};

EstadoSistema estado_atual = RESFRIAMENTO_INICIAL;
bool sistema_ligado = true;
float temperatura_atual = 0;
int saida_pwm_atual = 0;

OneWire oneWire(PINO_DS18B20);
DallasTemperature sensores(&oneWire);

AsyncWebServer server(80);

const char* HTML_PAGE = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Controle Peltier 4°C</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
            color: #333;
        }
        
        .container {
            max-width: 1200px;
            margin: 0 auto;
            background: rgba(255, 255, 255, 0.95);
            border-radius: 20px;
            padding: 30px;
            box-shadow: 0 20px 40px rgba(0,0,0,0.1);
            backdrop-filter: blur(10px);
        }
        
        .header {
            text-align: center;
            margin-bottom: 30px;
        }
        
        .header h1 {
            color: #2c3e50;
            font-size: 2.5em;
            margin-bottom: 10px;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.1);
        }
        
        .header p {
            color: #7f8c8d;
            font-size: 1.1em;
        }
        
        .dashboard {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 20px;
            margin-bottom: 30px;
        }
        
        .card {
            background: #fff;
            border-radius: 15px;
            padding: 25px;
            box-shadow: 0 10px 30px rgba(0,0,0,0.1);
            transition: transform 0.3s ease, box-shadow 0.3s ease;
        }
        
        .card:hover {
            transform: translateY(-5px);
            box-shadow: 0 15px 40px rgba(0,0,0,0.15);
        }
        
        .card h3 {
            color: #2c3e50;
            margin-bottom: 15px;
            font-size: 1.4em;
            border-bottom: 2px solid #3498db;
            padding-bottom: 10px;
        }
        
        .temp-display {
            font-size: 3em;
            font-weight: bold;
            text-align: center;
            margin: 20px 0;
            color: #2980b9;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.1);
        }
        
        .status-indicator {
            display: inline-block;
            width: 12px;
            height: 12px;
            border-radius: 50%;
            margin-right: 8px;
            animation: pulse 2s infinite;
        }
        
        .status-on { background-color: #27ae60; }
        .status-cooling { background-color: #3498db; }
        .status-stable { background-color: #f39c12; }
        .status-off { background-color: #e74c3c; }
        
        @keyframes pulse {
            0% { opacity: 1; }
            50% { opacity: 0.5; }
            100% { opacity: 1; }
        }
        
        .control-group {
            margin: 20px 0;
        }
        
        .control-group label {
            display: block;
            margin-bottom: 8px;
            font-weight: 600;
            color: #2c3e50;
        }
        
        .control-group input {
            width: 100%;
            padding: 12px;
            border: 2px solid #bdc3c7;
            border-radius: 8px;
            font-size: 1em;
            transition: border-color 0.3s ease;
        }
        
        .control-group input:focus {
            outline: none;
            border-color: #3498db;
        }
        
        .btn {
            background: linear-gradient(45deg, #3498db, #2980b9);
            color: white;
            border: none;
            padding: 12px 25px;
            border-radius: 8px;
            font-size: 1em;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s ease;
            margin: 5px;
            box-shadow: 0 4px 15px rgba(52, 152, 219, 0.3);
        }
        
        .btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 6px 20px rgba(52, 152, 219, 0.4);
        }
        
        .btn.danger {
            background: linear-gradient(45deg, #e74c3c, #c0392b);
            box-shadow: 0 4px 15px rgba(231, 76, 60, 0.3);
        }
        
        .btn.danger:hover {
            box-shadow: 0 6px 20px rgba(231, 76, 60, 0.4);
        }
        
        .progress-bar {
            width: 100%;
            height: 20px;
            background: #ecf0f1;
            border-radius: 10px;
            overflow: hidden;
            margin: 10px 0;
        }
        
        .progress-fill {
            height: 100%;
            background: linear-gradient(45deg, #3498db, #2980b9);
            border-radius: 10px;
            transition: width 0.3s ease;
        }
        
        .info-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 15px;
            margin-top: 20px;
        }
        
        .info-item {
            text-align: center;
            padding: 15px;
            background: #f8f9fa;
            border-radius: 10px;
        }
        
        .info-item .value {
            font-size: 1.5em;
            font-weight: bold;
            color: #2c3e50;
        }
        
        .info-item .label {
            font-size: 0.9em;
            color: #7f8c8d;
            margin-top: 5px;
        }
        
        @media (max-width: 768px) {
            .dashboard {
                grid-template-columns: 1fr;
            }
            
            .header h1 {
                font-size: 2em;
            }
            
            .temp-display {
                font-size: 2.5em;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🧊 Controle Peltier</h1>
            <p>Sistema de Controle de Temperatura 4°C</p>
        </div>
        
        <div class="dashboard">
            <div class="card">
                <h3>🌡️ Temperatura Atual</h3>
                <div class="temp-display" id="tempAtual">--°C</div>
                <div class="info-grid">
                    <div class="info-item">
                        <div class="value" id="tempAlvo">4.0°C</div>
                        <div class="label">Alvo</div>
                    </div>
                    <div class="info-item">
                        <div class="value" id="erro">--°C</div>
                        <div class="label">Erro</div>
                    </div>
                </div>
            </div>
            
            <div class="card">
                <h3>⚡ Status do Sistema</h3>
                <div style="margin: 20px 0;">
                    <span class="status-indicator status-on" id="statusIndicator"></span>
                    <span id="statusText">Carregando...</span>
                </div>
                <div class="progress-bar">
                    <div class="progress-fill" id="pwmProgress" style="width: 0%"></div>
                </div>
                <div class="info-grid">
                    <div class="info-item">
                        <div class="value" id="pwmValue">0</div>
                        <div class="label">PWM</div>
                    </div>
                    <div class="info-item">
                        <div class="value" id="pwmPercent">0%</div>
                        <div class="label">Potência</div>
                    </div>
                </div>
            </div>
            
            <div class="card">
                <h3>🎛️ Controles</h3>
                <div class="control-group">
                    <label for="tempTarget">Temperatura Alvo:</label>
                    <input type="number" id="tempTarget" value="4.0" step="0.1" min="-10" max="50">
                </div>
                <div class="control-group">
                    <button class="btn" onclick="setTarget()">Definir Alvo</button>
                    <button class="btn danger" onclick="toggleSystem()">Ligar/Desligar</button>
                </div>
            </div>
            
            <div class="card">
                <h3>🔧 Parâmetros PID</h3>
                <div class="control-group">
                    <label for="kpValue">Kp (Proporcional):</label>
                    <input type="number" id="kpValue" value="100.0" step="0.1">
                </div>
                <div class="control-group">
                    <label for="kiValue">Ki (Integral):</label>
                    <input type="number" id="kiValue" value="0.5" step="0.1">
                </div>
                <div class="control-group">
                    <label for="kdValue">Kd (Derivativo):</label>
                    <input type="number" id="kdValue" value="50.0" step="0.1">
                </div>
                <button class="btn" onclick="setPID()">Atualizar PID</button>
            </div>
        </div>
    </div>

    <script>
        function updateData() {
            fetch('/data')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('tempAtual').textContent = data.temperatura.toFixed(1) + '°C';
                    document.getElementById('tempAlvo').textContent = data.alvo.toFixed(1) + '°C';
                    document.getElementById('erro').textContent = Math.abs(data.erro).toFixed(1) + '°C';
                    document.getElementById('pwmValue').textContent = data.pwm;
                    document.getElementById('pwmPercent').textContent = Math.round((data.pwm / 255) * 100) + '%';
                    document.getElementById('pwmProgress').style.width = Math.round((data.pwm / 255) * 100) + '%';
                    document.getElementById('statusText').textContent = data.status;
                    
                    // Atualizar indicador de status
                    const indicator = document.getElementById('statusIndicator');
                    indicator.className = 'status-indicator ';
                    if (!data.ligado) {
                        indicator.className += 'status-off';
                    } else if (data.status.includes('INICIAL')) {
                        indicator.className += 'status-cooling';
                    } else if (data.status.includes('ESTABILIZADO')) {
                        indicator.className += 'status-stable';
                    } else {
                        indicator.className += 'status-on';
                    }
                })
                .catch(error => console.error('Erro:', error));
        }
        
        function setTarget() {
            const target = document.getElementById('tempTarget').value;
            fetch('/setTarget', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({target: parseFloat(target)})
            });
        }
        
        function toggleSystem() {
            fetch('/toggle', {method: 'POST'});
        }
        
        function setPID() {
            const kp = document.getElementById('kpValue').value;
            const ki = document.getElementById('kiValue').value;
            const kd = document.getElementById('kdValue').value;
            
            fetch('/setPID', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({
                    kp: parseFloat(kp),
                    ki: parseFloat(ki),
                    kd: parseFloat(kd)
                })
            });
        }
        
        // Atualizar dados a cada 2 segundos
        setInterval(updateData, 2000);
        updateData(); // Primeira chamada
    </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  sensores.begin();
  
  pinMode(pinoPeltier, OUTPUT);
  ledcAttachPin(pinoPeltier, 0);
  ledcSetup(0, 1000, 8);
  
  tempo_anterior = millis();
  
  // Conectar WiFi
  WiFi.softAP(ssid,password);
  Serial.print("Conectado! IP: ");
  Serial.println(WiFi.softAPIP());
  
  // Configurar rotas do servidor
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", HTML_PAGE);
  });
  
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
    StaticJsonDocument<200> doc;
    doc["temperatura"] = temperatura_atual;
    doc["alvo"] = TEMP_ALVO;
    doc["erro"] = TEMP_ALVO - temperatura_atual;
    doc["pwm"] = saida_pwm_atual;
    doc["ligado"] = sistema_ligado;
    
    String status = "";
    switch (estado_atual) {
      case RESFRIAMENTO_INICIAL: status = "RESFR. INICIAL"; break;
      case CONTROLE_PID: status = "CONTROLE PID"; break;
      case ESTABILIZADO: status = "ESTABILIZADO"; break;
    }
    if (!sistema_ligado) status = "DESLIGADO";
    doc["status"] = status;
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  server.on("/setTarget", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      StaticJsonDocument<100> doc;
      deserializeJson(doc, (const char*)data);
      TEMP_ALVO = doc["target"];
      estado_atual = RESFRIAMENTO_INICIAL; // Reiniciar controle
      integral = 0; // Reset integral
      Serial.println("Nova temperatura alvo: " + String(TEMP_ALVO));
      request->send(200, "text/plain", "OK");
    });
  
  server.on("/toggle", HTTP_POST, [](AsyncWebServerRequest *request){
    sistema_ligado = !sistema_ligado;
    if (!sistema_ligado) {
      ledcWrite(0, 0);
      saida_pwm_atual = 0;
    }
    Serial.println("Sistema " + String(sistema_ligado ? "LIGADO" : "DESLIGADO"));
    request->send(200, "text/plain", "OK");
  });
  
  server.on("/setPID", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      StaticJsonDocument<100> doc;
      deserializeJson(doc, (const char*)data);
      kp = doc["kp"];
      ki = doc["ki"];
      kd = doc["kd"];
      integral = 0; // Reset integral
      Serial.println("Novos parâmetros PID - Kp: " + String(kp) + " Ki: " + String(ki) + " Kd: " + String(kd));
      request->send(200, "text/plain", "OK");
    });
  
  server.begin();
  Serial.println("=== Servidor Web Iniciado ===");
  Serial.println("Acesse: http://" + WiFi.localIP().toString());
}

float calcularPID(float temperatura) {
  unsigned long tempo_atual = millis();
  float dt = (tempo_atual - tempo_anterior) / 1000.0;
  
  if (dt <= 0) dt = 0.001;
  
  float erro = TEMP_ALVO - temperatura;
  
  float proporcional = kp * erro;
  
  integral += erro * dt;
  if (integral > 100) integral = 100;
  if (integral < -100) integral = -100;
  float integral_termo = ki * integral;
  
  float derivativo = kd * (erro - erro_anterior) / dt;
  
  float saida = proporcional + integral_termo + derivativo;
  
  if (saida > MAX_SAIDA_PELTIER) saida = MAX_SAIDA_PELTIER;
  if (saida < MIN_SAIDA_PELTIER) saida = MIN_SAIDA_PELTIER;
  
  erro_anterior = erro;
  tempo_anterior = tempo_atual;
  
  return saida;
}

void loop() {
  sensores.requestTemperatures();
  temperatura_atual = sensores.getTempCByIndex(0);
  
  // Verificar sensor
  if (temperatura_atual == DEVICE_DISCONNECTED_C || temperatura_atual < -50) {
    Serial.println("ERRO: Sensor desconectado!");
    ledcWrite(0, 0);
    saida_pwm_atual = 0;
    return;
  }
  
  int saidaPeltier = 0;
  String status = "";
  
  if (sistema_ligado) {
    // Máquina de estados
    switch (estado_atual) {
      case RESFRIAMENTO_INICIAL:
        saidaPeltier = MAX_SAIDA_PELTIER;
        status = "RESFR. INICIAL";
        
        if (temperatura_atual <= TEMP_ALVO + 1.0) {
          estado_atual = CONTROLE_PID;
          integral = 0;
          Serial.println("Mudando para controle PID...");
        }
        break;
        
      case CONTROLE_PID:
        saidaPeltier = (int)calcularPID(temperatura_atual);
        status = "CONTROLE PID";
        
        if (abs(temperatura_atual - TEMP_ALVO) <= TEMP_TOLERANCE) {
          estado_atual = ESTABILIZADO;
          Serial.println("Temperatura estabilizada!");
        }
        break;
        
      case ESTABILIZADO:
        saidaPeltier = (int)calcularPID(temperatura_atual);
        status = "ESTABILIZADO";
        
        if (abs(temperatura_atual - TEMP_ALVO) > TEMP_TOLERANCE * 2) {
          estado_atual = CONTROLE_PID;
          Serial.println("Saiu da tolerância, ajustando...");
        }
        break;
    }
  } else {
    status = "DESLIGADO";
    saidaPeltier = 0;
  }
  
  saida_pwm_atual = saidaPeltier;
  ledcWrite(0, saidaPeltier);
  
  // Log no Serial Monitor
  Serial.print("Temp: ");
  Serial.print(temperatura_atual, 2);
  Serial.print("°C | Alvo: ");
  Serial.print(TEMP_ALVO);
  Serial.print("°C | PWM: ");
  Serial.print(saidaPeltier);
  Serial.print(" | Status: ");
  Serial.println(status);
  
  delay(500);
}