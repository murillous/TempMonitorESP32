#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// Configuração dos pinos
#define PINO_DS18B20 4
#define pino_peltier 5

// Configurações WiFi
const char *nome_rede = "ESP32";
const char *senha_rede = "12345678";

// Configurações de temperatura e controle
float TEMPERATURA_ALVO = 4.0;
const float BANDA_MORTA = 0.2;              // Banda morta para evitar oscilação
const float LIMITE_INICIAL = 2.0;           // Threshold para sair do resfriamento inicial
const float TEMPO_AMOSTRA_SEG = 0.5;        // Tempo de amostragem fixo
const int PWM_MAXIMO = 255;
const int PWM_MINIMO = 0;

// Parâmetros PID
float kp = 30.0;
float ki = 0.1;   
float kd = 10.0;  
const float INTEGRAL_MAXIMO = 100.0;        // Limite do integrador
const unsigned long TIMEOUT_LIMITE_MS = 30000; // 30s para detectar setpoint inatingível

// Variáveis do controlador PID
float integral = 0;
float ultimo_erro = 0;
float ultima_temperatura = 0;
unsigned long ultimo_tempo = 0;
unsigned long tempo_no_maximo = 0;
bool resfriamento_inicial = false;
bool limite_sistema_atingido = false;

// Variáveis do sistema
bool sistema_ligado = false;
float temperatura_atual = 0;
int saida_pwm_atual = 0;
String status_atual = "DESLIGADO";

// Objetos
OneWire unWire(PINO_DS18B20);
DallasTemperature sensores(&unWire);
AsyncWebServer servidor(80);

// HTML permanece o mesmo
const char *PAGINA_HTML = R"rawliteral(
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
            <p>Sistema de Controle de Temperatura</p>
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
                    <button class="btn" onclick="definirAlvo()">Definir Alvo</button>
                    <button class="btn danger" onclick="alternarSistema()">Ligar/Desligar</button>
                    <button class="btn" onclick="resetarLimite()">Reset Limite</button>
                </div>
            </div>
            
            <div class="card">
                <h3>🔧 Parâmetros PID</h3>
                <div class="control-group">
                    <label for="valorKp">Kp (Proporcional):</label>
                    <input type="number" id="valorKp" value="30.0" step="0.1">
                </div>
                <div class="control-group">
                    <label for="valorKi">Ki (Integral):</label>
                    <input type="number" id="valorKi" value="0.1" step="0.1">
                </div>
                <div class="control-group">
                    <label for="valorKd">Kd (Derivativo):</label>
                    <input type="number" id="valorKd" value="10.0" step="0.1">
                </div>
                <button class="btn" onclick="definirPID()">Atualizar PID</button>
            </div>
        </div>
    </div>

    <script>
        function atualizarDados() {
            fetch('/dados')
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
        
        function definirAlvo() {
            const alvo = document.getElementById('tempTarget').value;
            fetch('/definirAlvo', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({alvo: parseFloat(alvo)})
            });
        }
        
        function alternarSistema() {
            fetch('/alternar', {method: 'POST'});
        }
        
        function resetarLimite() {
            fetch('/resetarLimite', {method: 'POST'});
        }
        
        function definirPID() {
            const kp = document.getElementById('valorKp').value;
            const ki = document.getElementById('valorKi').value;
            const kd = document.getElementById('valorKd').value;
            
            fetch('/definirPID', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({
                    kp: parseFloat(kp),
                    ki: parseFloat(ki),
                    kd: parseFloat(kd)
                })
            });
        }
        
        setInterval(atualizarDados, 2000);
        atualizarDados();
    </script>
</body>
</html>
)rawliteral";

void resetar_controlador() {
    integral = 0;
    ultimo_erro = 0;
    ultimo_tempo = millis();
    tempo_no_maximo = 0;
    limite_sistema_atingido = false;
    resfriamento_inicial = false;
    Serial.println("Controlador resetado");
}

void iniciar_resfriamento() {
    resfriamento_inicial = true;
    integral = 0;
    ultimo_erro = 0;
    Serial.println("Iniciando resfriamento inicial...");
}

int calcular_pid(float temperatura_atual) {
    unsigned long agora = millis();
    float dt = (agora - ultimo_tempo) / 1000.0;
    
    // Garante sample time mínimo
    if (dt < TEMPO_AMOSTRA_SEG) {
        return saida_pwm_atual; // Mantém saída anterior
    }
    
    ultimo_tempo = agora;
    
    // CORREÇÃO CRÍTICA: erro = temperatura_atual - setpoint
    // Positivo = precisa resfriar, Negativo = não precisa resfriar
    float erro = temperatura_atual - TEMPERATURA_ALVO;
    
    // Se não precisa resfriar (temperatura abaixo do setpoint)
    if (erro <= 0) {
        integral = 0; // Evita windup quando não há necessidade de resfriamento
        resfriamento_inicial = false;
        limite_sistema_atingido = false;
        tempo_no_maximo = 0;
        status_atual = "SEM RESFRIAMENTO";
        Serial.println("Setpoint atingido - PWM = 0");
        return 0;
    }
    
    // Resfriamento inicial
    if (resfriamento_inicial) {
        status_atual = "RESFR. INICIAL";
        if (erro <= LIMITE_INICIAL) {
            // Transição suave para PID
            resfriamento_inicial = false;
            integral = 0; // Bumpless transfer
            Serial.println("Transição para controle PID");
        }
        return PWM_MAXIMO;
    }
    
    // Controle PID
    float derivada = 0;
    if (ultimo_tempo > 0) {
        derivada = (erro - ultimo_erro) / dt;
    }
    
    float proporcional = kp * erro;
    float termo_integral = ki * integral;
    float termo_derivativo = kd * derivada;
    
    float saida = proporcional + termo_integral + termo_derivativo;
    
    // Anti-windup inteligente
    bool saturando_alto = (saida >= PWM_MAXIMO && erro > 0);
    bool saturando_baixo = (saida <= 0 && erro < 0);
    
    if (!saturando_alto && !saturando_baixo) {
        integral += erro * dt;
        integral = constrain(integral, -INTEGRAL_MAXIMO, INTEGRAL_MAXIMO);
    }
    
    // Recalcular com integral atualizada
    saida = proporcional + (ki * integral) + termo_derivativo;
    saida = constrain(saida, 0, PWM_MAXIMO);
    
    // Deadband para estabilidade
    if (abs(erro) <= BANDA_MORTA) {
        status_atual = "ESTÁVEL (DEADBAND)";
        // Mantém a saída calculada pelo PID, não força zero!
    } else {
        status_atual = "CONTROLE PID";
    }
    
    // Detectar saturação prolongada
    if (saida >= PWM_MAXIMO - 1) {
        if (tempo_no_maximo == 0) {
            tempo_no_maximo = agora;
        } else if (agora - tempo_no_maximo > TIMEOUT_LIMITE_MS) {
            limite_sistema_atingido = true;
            status_atual = "LIMITE ATINGIDO";
            Serial.println("⚠️ LIMITE DO SISTEMA - Setpoint pode ser inatingível!");
        }
    } else {
        tempo_no_maximo = 0;
        if (limite_sistema_atingido && erro > LIMITE_INICIAL) {
            limite_sistema_atingido = false; // Reset se saiu da saturação
        }
    }
    
    ultimo_erro = erro;
    
    // Debug PID detalhado
    if (millis() % 5000 < 500) {
        Serial.println("PID: P=" + String(proporcional, 1) + 
                      " I=" + String(termo_integral, 1) + 
                      " D=" + String(termo_derivativo, 1) + 
                      " Err=" + String(erro, 2) + 
                      " Out=" + String(saida, 1));
    }
    
    return (int)saida;
}

void setup() {
    Serial.begin(115200);
    sensores.begin();
    
    pinMode(pino_peltier, OUTPUT);
    ledcSetup(0, 1000, 8);
    ledcAttachPin(pino_peltier, 0);
    
    resetar_controlador();
    
    // WiFi
    WiFi.softAP(nome_rede, senha_rede);
    
    // Rotas do servidor
    servidor.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", PAGINA_HTML);
    });

    servidor.on("/dados", HTTP_GET, [](AsyncWebServerRequest *request) {
        StaticJsonDocument<300> doc;
        doc["temperatura"] = temperatura_atual;
        doc["alvo"] = TEMPERATURA_ALVO;
        doc["erro"] = temperatura_atual - TEMPERATURA_ALVO;
        doc["pwm"] = saida_pwm_atual;
        doc["ligado"] = sistema_ligado;
        doc["status"] = status_atual;
        doc["limitReached"] = limite_sistema_atingido;
        
        String resposta;
        serializeJson(doc, resposta);
        request->send(200, "application/json", resposta);
    });

    servidor.on("/definirAlvo", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
              [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        StaticJsonDocument<100> doc;
        deserializeJson(doc, (const char*)data);
        
        float novo_alvo = doc["alvo"];
        
        // Bumpless transfer ao mudar setpoint
        if (sistema_ligado) {
            float erro_atual = temperatura_atual - TEMPERATURA_ALVO;
            float novo_erro = temperatura_atual - novo_alvo;
            
            // Se mudou de precisar resfriar para não precisar, ou vice-versa
            if ((erro_atual > 0) != (novo_erro > 0)) {
                resetar_controlador();
            }
            
            // Se o novo setpoint requer resfriamento inicial
            if (novo_erro > LIMITE_INICIAL) {
                iniciar_resfriamento();
            }
        }
        
        TEMPERATURA_ALVO = novo_alvo;
        
        Serial.println("Nova temperatura alvo: " + String(TEMPERATURA_ALVO) + "°C");
        request->send(200, "text/plain", "OK");
    });

    servidor.on("/alternar", HTTP_POST, [](AsyncWebServerRequest *request) {
        sistema_ligado = !sistema_ligado;
        if (!sistema_ligado) {
            ledcWrite(0, 0);
            saida_pwm_atual = 0;
            status_atual = "DESLIGADO";
        } else {
            resetar_controlador();
            float erro = temperatura_atual - TEMPERATURA_ALVO;
            if (erro > LIMITE_INICIAL) {
                iniciar_resfriamento();
            }
        }
        Serial.println("Sistema " + String(sistema_ligado ? "LIGADO" : "DESLIGADO"));
        request->send(200, "text/plain", "OK");
    });

    servidor.on("/resetarLimite", HTTP_POST, [](AsyncWebServerRequest *request) {
        limite_sistema_atingido = false;
        tempo_no_maximo = 0;
        Serial.println("Limite resetado pelo usuário");
        request->send(200, "text/plain", "OK");
    });

    servidor.on("/definirPID", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
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

    servidor.begin();
    Serial.println("=== Sistema de Controle Peltier Iniciado ===");
    Serial.println("Versão com correções técnicas implementadas");
    Serial.println("Acesse: http://" + WiFi.softAPIP().toString());
}

void loop() {
    unsigned long agora = millis();
    static unsigned long ultima_amostra = 0;
    
    // Sample time rigoroso
    if (agora - ultima_amostra < (TEMPO_AMOSTRA_SEG * 1000)) {
        return;
    }
    ultima_amostra = agora;
    
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
    static float buffer_temperatura[3] = {0};
    buffer_temperatura[0] = buffer_temperatura[1];
    buffer_temperatura[1] = buffer_temperatura[2];
    buffer_temperatura[2] = temperatura_atual;
    temperatura_atual = (buffer_temperatura[0] + buffer_temperatura[1] + buffer_temperatura[2]) / 3.0;
    
    int saida_pwm = 0;
    
    if (sistema_ligado) {
        saida_pwm = calcular_pid(temperatura_atual);
    } else {
        status_atual = "DESLIGADO";
    }
    
    saida_pwm_atual = saida_pwm;
    ledcWrite(0, saida_pwm);
    
    // Log detalhado
    Serial.print("Temp: ");
    Serial.print(temperatura_atual, 2);
    Serial.print("°C | Alvo: ");
    Serial.print(TEMPERATURA_ALVO, 1);
    Serial.print("°C | Erro: ");
    Serial.print(temperatura_atual - TEMPERATURA_ALVO, 2);
    Serial.print("°C | PWM: ");
    Serial.print(saida_pwm);
    Serial.print(" | Status: ");
    Serial.println(status_atual);
}