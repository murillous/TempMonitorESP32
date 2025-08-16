#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// Configuração dos pinos
#define PINO_DS18B20 4
#define pinoPeltier 5

// Configurações WiFi
const char *ssid = "ESP32";
const char *password = "12345678";

// Configurações de temperatura e controle
float TEMP_ALVO = 4.0;
const float DEADBAND = 0.2;              // Banda morta para evitar oscilação
const float INIT_THRESHOLD = 2.0;        // Threshold para sair do resfriamento inicial
const float SAMPLE_TIME_SEC = 0.5;       // Tempo de amostragem fixo
const int PWM_MAX = 255;
const int PWM_MIN = 0;

// Parâmetros PID
float kp = 30.0;
float ki = 0.1;   
float kd = 10.0;  
const float I_MAX = 100.0;               // Limite do integrador
const unsigned long LIMIT_TIMEOUT_MS = 30000; // 30s para detectar setpoint inatingível

// Variáveis do controlador PID
float integral = 0;
float lastError = 0;
float lastTemp = 0;
unsigned long lastTime = 0;
unsigned long timeAtMax = 0;
bool coolingInit = false;
bool systemLimitReached = false;

// Variáveis do sistema
bool sistema_ligado = false;
float temperatura_atual = 0;
int saida_pwm_atual = 0;
String status_atual = "DESLIGADO";

// Objetos
OneWire oneWire(PINO_DS18B20);
DallasTemperature sensores(&oneWire);
AsyncWebServer server(80);

// HTML permanece o mesmo
const char *HTML_PAGE = R"rawliteral(
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
        .status-limit { background-color: #e74c3c; }
        .status-off { background-color: #95a5a6; }
        
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
        
        .alert {
            background: #e74c3c;
            color: white;
            padding: 15px;
            border-radius: 10px;
            margin: 10px 0;
            text-align: center;
            font-weight: bold;
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
            <p>Sistema de Controle de Temperatura - Versão Corrigida</p>
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
                <div id="limitAlert" class="alert" style="display: none;">
                    ⚠️ LIMITE DO SISTEMA ATINGIDO - Setpoint pode ser inatingível!
                </div>
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
                    <button class="btn" onclick="resetLimit()">Reset Limite</button>
                </div>
            </div>
            
            <div class="card">
                <h3>🔧 Parâmetros PID</h3>
                <div class="control-group">
                    <label for="kpValue">Kp (Proporcional):</label>
                    <input type="number" id="kpValue" value="30.0" step="0.1">
                </div>
                <div class="control-group">
                    <label for="kiValue">Ki (Integral):</label>
                    <input type="number" id="kiValue" value="0.1" step="0.1">
                </div>
                <div class="control-group">
                    <label for="kdValue">Kd (Derivativo):</label>
                    <input type="number" id="kdValue" value="10.0" step="0.1">
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
                    document.getElementById('erro').textContent = Math.abs(data.erro).toFixed(2) + '°C';
                    document.getElementById('pwmValue').textContent = data.pwm;
                    document.getElementById('pwmPercent').textContent = Math.round((data.pwm / 255) * 100) + '%';
                    document.getElementById('pwmProgress').style.width = Math.round((data.pwm / 255) * 100) + '%';
                    document.getElementById('statusText').textContent = data.status;
                    
                    // Mostrar alerta de limite
                    const limitAlert = document.getElementById('limitAlert');
                    limitAlert.style.display = data.limitReached ? 'block' : 'none';
                    
                    // Atualizar indicador de status
                    const indicator = document.getElementById('statusIndicator');
                    indicator.className = 'status-indicator ';
                    if (!data.ligado) {
                        indicator.className += 'status-off';
                    } else if (data.limitReached) {
                        indicator.className += 'status-limit';
                    } else if (data.status.includes('INICIAL')) {
                        indicator.className += 'status-cooling';
                    } else if (data.status.includes('ESTÁVEL')) {
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
        
        function resetLimit() {
            fetch('/resetLimit', {method: 'POST'});
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
        
        setInterval(updateData, 2000);
        updateData();
    </script>
</body>
</html>
)rawliteral";

void resetController() {
    integral = 0;
    lastError = 0;
    lastTime = millis();
    timeAtMax = 0;
    systemLimitReached = false;
    coolingInit = false;
    Serial.println("Controlador resetado");
}

void initCooling() {
    coolingInit = true;
    integral = 0;
    lastError = 0;
    Serial.println("Iniciando resfriamento inicial...");
}

int calculatePID(float currentTemp) {
    unsigned long now = millis();
    float dt = (now - lastTime) / 1000.0;
    
    // Garante sample time mínimo
    if (dt < SAMPLE_TIME_SEC) {
        return saida_pwm_atual; // Mantém saída anterior
    }
    
    lastTime = now;
    
    // CORREÇÃO CRÍTICA: erro = temperatura_atual - setpoint
    // Positivo = precisa resfriar, Negativo = não precisa resfriar
    float error = currentTemp - TEMP_ALVO;
    
    // Se não precisa resfriar (temperatura abaixo do setpoint)
    if (error <= 0) {
        integral = 0; // Evita windup quando não há necessidade de resfriamento
        coolingInit = false;
        systemLimitReached = false;
        timeAtMax = 0;
        status_atual = "SEM RESFRIAMENTO";
        Serial.println("Setpoint atingido - PWM = 0");
        return 0;
    }
    
    // Resfriamento inicial
    if (coolingInit) {
        status_atual = "RESFR. INICIAL";
        if (error <= INIT_THRESHOLD) {
            // Transição suave para PID
            coolingInit = false;
            integral = 0; // Bumpless transfer
            Serial.println("Transição para controle PID");
        }
        return PWM_MAX;
    }
    
    // Controle PID
    float derivative = 0;
    if (lastTime > 0) {
        derivative = (error - lastError) / dt;
    }
    
    float proportional = kp * error;
    float integralTerm = ki * integral;
    float derivativeTerm = kd * derivative;
    
    float output = proportional + integralTerm + derivativeTerm;
    
    // Anti-windup inteligente
    bool saturatingHigh = (output >= PWM_MAX && error > 0);
    bool saturatingLow = (output <= 0 && error < 0);
    
    if (!saturatingHigh && !saturatingLow) {
        integral += error * dt;
        integral = constrain(integral, -I_MAX, I_MAX);
    }
    
    // Recalcular com integral atualizada
    output = proportional + (ki * integral) + derivativeTerm;
    output = constrain(output, 0, PWM_MAX);
    
    // Deadband para estabilidade
    if (abs(error) <= DEADBAND) {
        status_atual = "ESTÁVEL (DEADBAND)";
        // Mantém a saída calculada pelo PID, não força zero!
    } else {
        status_atual = "CONTROLE PID";
    }
    
    // Detectar saturação prolongada
    if (output >= PWM_MAX - 1) {
        if (timeAtMax == 0) {
            timeAtMax = now;
        } else if (now - timeAtMax > LIMIT_TIMEOUT_MS) {
            systemLimitReached = true;
            status_atual = "LIMITE ATINGIDO";
            Serial.println("⚠️ LIMITE DO SISTEMA - Setpoint pode ser inatingível!");
        }
    } else {
        timeAtMax = 0;
        if (systemLimitReached && error > INIT_THRESHOLD) {
            systemLimitReached = false; // Reset se saiu da saturação
        }
    }
    
    lastError = error;
    
    // Debug PID detalhado
    if (millis() % 5000 < 500) {
        Serial.println("PID: P=" + String(proportional, 1) + 
                      " I=" + String(integralTerm, 1) + 
                      " D=" + String(derivativeTerm, 1) + 
                      " Err=" + String(error, 2) + 
                      " Out=" + String(output, 1));
    }
    
    return (int)output;
}

void setup() {
    Serial.begin(115200);
    sensores.begin();
    
    pinMode(pinoPeltier, OUTPUT);
    ledcSetup(0, 1000, 8);
    ledcAttachPin(pinoPeltier, 0);
    
    resetController();
    
    // WiFi
    WiFi.softAP(ssid, password);
    
    // Rotas do servidor
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", HTML_PAGE);
    });

    server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
        StaticJsonDocument<300> doc;
        doc["temperatura"] = temperatura_atual;
        doc["alvo"] = TEMP_ALVO;
        doc["erro"] = temperatura_atual - TEMP_ALVO;
        doc["pwm"] = saida_pwm_atual;
        doc["ligado"] = sistema_ligado;
        doc["status"] = status_atual;
        doc["limitReached"] = systemLimitReached;
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    server.on("/setTarget", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
              [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        StaticJsonDocument<100> doc;
        deserializeJson(doc, (const char*)data);
        
        float newTarget = doc["target"];
        
        // Bumpless transfer ao mudar setpoint
        if (sistema_ligado) {
            float currentError = temperatura_atual - TEMP_ALVO;
            float newError = temperatura_atual - newTarget;
            
            // Se mudou de precisar resfriar para não precisar, ou vice-versa
            if ((currentError > 0) != (newError > 0)) {
                resetController();
            }
            
            // Se o novo setpoint requer resfriamento inicial
            if (newError > INIT_THRESHOLD) {
                initCooling();
            }
        }
        
        TEMP_ALVO = newTarget;
        
        Serial.println("Nova temperatura alvo: " + String(TEMP_ALVO) + "°C");
        request->send(200, "text/plain", "OK");
    });

    server.on("/toggle", HTTP_POST, [](AsyncWebServerRequest *request) {
        sistema_ligado = !sistema_ligado;
        if (!sistema_ligado) {
            ledcWrite(0, 0);
            saida_pwm_atual = 0;
            status_atual = "DESLIGADO";
        } else {
            resetController();
            float error = temperatura_atual - TEMP_ALVO;
            if (error > INIT_THRESHOLD) {
                initCooling();
            }
        }
        Serial.println("Sistema " + String(sistema_ligado ? "LIGADO" : "DESLIGADO"));
        request->send(200, "text/plain", "OK");
    });

    server.on("/resetLimit", HTTP_POST, [](AsyncWebServerRequest *request) {
        systemLimitReached = false;
        timeAtMax = 0;
        Serial.println("Limite resetado pelo usuário");
        request->send(200, "text/plain", "OK");
    });

    server.on("/setPID", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
              [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        StaticJsonDocument<100> doc;
        deserializeJson(doc, (const char*)data);
        kp = doc["kp"];
        ki = doc["ki"];
        kd = doc["kd"];
        integral = 0; // Reset integral quando muda parâmetros
        Serial.println("PID atualizado - Kp:" + String(kp) + " Ki:" + String(ki) + " Kd:" + String(kd));
        request->send(200, "text/plain", "OK");
    });

    server.begin();
    Serial.println("=== Sistema de Controle Peltier Iniciado ===");
    Serial.println("Versão com correções técnicas implementadas");
    Serial.println("Acesse: http://" + WiFi.softAPIP().toString());
}

void loop() {
    unsigned long now = millis();
    static unsigned long lastSample = 0;
    
    // Sample time rigoroso
    if (now - lastSample < (SAMPLE_TIME_SEC * 1000)) {
        return;
    }
    lastSample = now;
    
    // Ler temperatura
    sensores.requestTemperatures();
    temperatura_atual = sensores.getTempCByIndex(0);
    
    // Verificar sensor
    if (temperatura_atual == DEVICE_DISCONNECTED_C || temperatura_atual < -50) {
        Serial.println("ERRO: Sensor desconectado!");
        ledcWrite(0, 0);
        saida_pwm_atual = 0;
        status_atual = "ERRO SENSOR";
        return;
    }
    
    // Filtro simples de temperatura (média móvel)
    static float tempBuffer[3] = {0};
    tempBuffer[0] = tempBuffer[1];
    tempBuffer[1] = tempBuffer[2];
    tempBuffer[2] = temperatura_atual;
    temperatura_atual = (tempBuffer[0] + tempBuffer[1] + tempBuffer[2]) / 3.0;
    
    int pwmOutput = 0;
    
    if (sistema_ligado) {
        pwmOutput = calculatePID(temperatura_atual);
    } else {
        status_atual = "DESLIGADO";
    }
    
    saida_pwm_atual = pwmOutput;
    ledcWrite(0, pwmOutput);
    
    // Log detalhado
    Serial.print("Temp: ");
    Serial.print(temperatura_atual, 2);
    Serial.print("°C | Alvo: ");
    Serial.print(TEMP_ALVO, 1);
    Serial.print("°C | Erro: ");
    Serial.print(temperatura_atual - TEMP_ALVO, 2);
    Serial.print("°C | PWM: ");
    Serial.print(pwmOutput);
    Serial.print(" | Status: ");
    Serial.println(status_atual);
}