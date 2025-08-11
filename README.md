# 🧊 Sistema de Controle de Temperatura com Célula Peltier - ESP32

## Descrição

Este projeto implementa um sistema de controle de temperatura utilizando ESP32, sensor DS18B20 e célula Peltier com controle PID avançado. O sistema mantém a temperatura alvo de 4°C (configurável) com precisão e oferece uma interface web moderna e responsiva para monitoramento e controle.

## 🚀 Características

- **Controle PID Otimizado**: Implementação com anti-windup para evitar saturação do termo integral
- **Interface Web Moderna**: Dashboard responsivo com atualização em tempo real
- **Três Estados de Operação**:
  - Resfriamento Inicial (PWM máximo)
  - Controle PID (ajuste fino)
  - Estabilizado (manutenção da temperatura)
- **Configuração Remota**: Ajuste de temperatura alvo e parâmetros PID via web
- **Monitoramento Visual**: Indicadores de status, gráficos de progresso e métricas em tempo real
- **Anti-Windup Inteligente**: Previne acúmulo excessivo do termo integral
- **Controle não-bloqueante**: Loop principal otimizado para responsividade

## 🛠️ Hardware Necessário

- **ESP32** (qualquer modelo)
- **Sensor DS18B20** (sensor de temperatura à prova d'água recomendado)
- **Célula Peltier** (TEC1-12706 ou similar)
- **Driver para Peltier** (módulo com MOSFET ou ponte H)
- **Resistor de pull-up** 4.7kΩ para o sensor DS18B20
- **Fonte de alimentação** adequada para a célula Peltier

## 🔌 Conexões

```
ESP32          |  Componente
---------------|------------------
GPIO 4         |  DS18B20 (Data)
GPIO 5         |  Driver Peltier (PWM)
3.3V          |  DS18B20 (VCC)
GND           |  DS18B20 (GND)
VIN/5V        |  Driver Peltier (VCC)
GND           |  Driver Peltier (GND)
```

**Importante**: Conecte um resistor de pull-up de 4.7kΩ entre o pino de dados do DS18B20 (GPIO 4) e VCC (3.3V).

## 📦 Dependências

O projeto utiliza as seguintes bibliotecas (instaladas via PlatformIO):

- `OneWire` - Comunicação com sensor DS18B20
- `DallasTemperature` - Interface simplificada para DS18B20
- `WiFi` - Conectividade WiFi (ESP32 core)
- `ESPAsyncWebServer` - Servidor web assíncrono
- `ArduinoJson` - Manipulação de dados JSON

## 🔧 Instalação e Configuração

### Pré-requisitos
- [PlatformIO](https://platformio.org/) instalado no VS Code ou como CLI
- ESP32 configurado no PlatformIO

### Configuração do Projeto

1. **Clone ou baixe o projeto**
2. **Abra no PlatformIO** (VS Code + extensão PlatformIO)
3. **Configure o arquivo `platformio.ini`**:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps = 
    oneWire
    milesburton/DallasTemperature@^3.9.1
    me-no-dev/ESPAsyncWebServer@^1.2.3
    bblanchon/ArduinoJson@^6.21.3
monitor_speed = 115200
```

4. **Ajuste as configurações no código** (se necessário):
```cpp
// Configurações WiFi
const char *ssid = "ESP32";           // Nome da rede WiFi
const char *password = "12345678";    // Senha da rede WiFi

// Configurações de temperatura
float TEMP_ALVO = 4.0;               // Temperatura alvo inicial
const float TEMP_TOLERANCE = 0.2;    // Tolerância para estabilização

// Parâmetros PID iniciais
float kp = 30.0;  // Ganho proporcional
float ki = 0.1;   // Ganho integral
float kd = 10.0;  // Ganho derivativo
```

### Compilação e Upload

1. **Compile o projeto**: `Ctrl+Alt+B` no VS Code ou `pio run`
2. **Faça o upload**: `Ctrl+Alt+U` no VS Code ou `pio run --target upload`
3. **Monitor serial**: `Ctrl+Alt+S` no VS Code ou `pio device monitor`

## 🌐 Acesso à Interface Web

1. Após o upload, abra o **Monitor Serial** (115200 baud)
2. Anote o **IP do Access Point** (geralmente 192.168.4.1)
3. Conecte seu dispositivo à rede WiFi "ESP32" (senha: 12345678)
4. Acesse o IP no navegador

### Funcionalidades da Interface

- **Dashboard Principal**: Temperatura atual, alvo, erro e status do sistema
- **Controle de PWM**: Visualização da potência aplicada na Peltier
- **Configurações**: Ajuste de temperatura alvo e parâmetros PID
- **Status Visual**: Indicadores coloridos para diferentes estados
- **Responsivo**: Interface adaptada para desktop e mobile

## ⚙️ Configuração dos Parâmetros

### Parâmetros PID

- **Kp (Proporcional)**: Controla a resposta imediata ao erro (padrão: 30.0)
- **Ki (Integral)**: Elimina erro em regime permanente (padrão: 0.1)
- **Kd (Derivativo)**: Amortece oscilações (padrão: 10.0)

### Ajuste Fino

Para otimizar o controle para seu sistema específico:

1. **Comece com Kp**: Aumente até obter resposta rápida sem oscilação excessiva
2. **Ajuste Ki**: Adicione para eliminar erro residual (valores baixos)
3. **Configure Kd**: Use para reduzir overshoot e oscilações

## 📊 Estados do Sistema

- **RESFRIAMENTO_INICIAL**: PWM máximo até se aproximar do alvo
- **CONTROLE_PID**: Controle fino com algoritmo PID
- **ESTABILIZADO**: Temperatura dentro da tolerância, manutenção ativa

## 🔍 Monitoramento e Debug

O sistema fornece logs detalhados via Serial Monitor:

```
Temp: 4.12°C | Alvo: 4.00°C | PWM: 125 | Status: ESTABILIZADO
```

## 🚨 Características de Segurança

- **Detecção de sensor desconectado**: Para o sistema em caso de falha
- **Anti-windup**: Previne saturação do controlador
- **Limites de PWM**: Proteção contra valores inválidos
- **Tolerância configurável**: Evita oscilações desnecessárias

## 🔧 Troubleshooting

### Problemas Comuns

1. **Sensor não detectado**:
   - Verifique conexões e resistor de pull-up
   - Teste com código simples de leitura do DS18B20

2. **Controle instável**:
   - Reduza Kp e Kd
   - Aumente Ki gradualmente
   - Verifique isolamento térmico

3. **Interface web não carrega**:
   - Confirme conexão à rede "ESP32"
   - Verifique IP no monitor serial
   - Reinicie o ESP32

### Otimizações Sugeridas

- **Isolamento térmico**: Melhore o isolamento do sistema para maior eficiência
- **Dissipador**: Use dissipador adequado no lado quente da Peltier
- **Alimentação**: Fonte estável e adequada para a corrente da Peltier
