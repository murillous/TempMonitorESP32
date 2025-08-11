# TempMonitorESP32: Controle Termoelétrico para Biopreservação

Este repositório contém o firmware e a documentação do projeto **"Desenvolvimento de um Dispositivo Usando Placas Termoelétricas para a Preservação de Sêmen de Peixes"**. O projeto foi desenvolvido no âmbito do Programa Institucional de Bolsas de Iniciação Científica (PIBIC) da Universidade Estadual do Maranhão (UEMA).

O objetivo é apresentar uma solução de baixo custo e alta precisão para o controle de temperatura em aplicações de biopreservação, utilizando um microcontrolador ESP32 para controlar um sistema de refrigeração baseado em pastilhas Peltier.

## 📋 Contexto

A preservação de material genético, como o sêmen de peixes, é um desafio crucial para a sustentabilidade e o avanço da aquicultura. A manutenção de uma temperatura estável e controlada é um dos fatores mais críticos para garantir a viabilidade celular. Este projeto surge como uma solução tecnológica que une engenharia de computação, automação e biotecnologia para enfrentar esse desafio.

## 🚀 Funcionalidades

O firmware implementado oferece um sistema de controle robusto e de fácil utilização:

### Controle Avançado de Temperatura
- **Controle PID**: Utiliza um algoritmo Proporcional, Integral e Derivativo para manter a temperatura com alta precisão, evitando oscilações bruscas
- **Anti-Windup**: Lógica implementada para prevenir a saturação do termo integral, garantindo estabilidade
- **Derivativo sobre Medição**: Evita "chutes derivativos" ao alterar o setpoint, resultando em uma operação mais suave

### Interface Web Responsiva
- O ESP32 cria um ponto de acesso Wi-Fi e hospeda uma página web para monitoramento e controle total do sistema

### Controle Remoto Completo
Através da interface é possível:
- Ligar e desligar o sistema
- Ajustar a temperatura alvo em tempo real
- Visualizar a temperatura atual, o erro e a potência (PWM) aplicada
- Ajustar as constantes do PID (Kp, Ki, Kd) para sintonia fina

### Sistema Inteligente
- **Máquina de Estados**: O sistema opera em três estágios para maior eficiência: 
  - `RESFRIAMENTO_INICIAL`
  - `CONTROLE_PID` 
  - `ESTABILIZADO`
- **Operação Stand-Alone**: Não necessita de um computador conectado após a programação, funcionando de forma autônoma

## 🛠️ Hardware Utilizado

| Componente | Quantidade | Descrição |
|------------|------------|-----------|
| Microcontrolador | 1 | ESP32 DevKitC |
| Sensor de Temperatura | 1 | DS18B20 (à prova d'água) |
| Módulos Peltier | 4 | TEC1-12706 |
| Atuador de Potência | 1 | MOSFET Canal-N (ex: 2SK1388) |
| Fonte de Alimentação | 1 | 12V / 50A |
| Dissipador de Calor | 1 | Dissipador de CPU de alta performance |
| Ventoinha | 1 | 20 cm de diâmetro, 12V |

## 📦 Software e Dependências

O projeto foi desenvolvido em C++ utilizando o framework Arduino (recomenda-se o uso do Visual Studio Code com a extensão PlatformIO).

### Bibliotecas Necessárias:
- `OneWire`
- `DallasTemperature`
- `WiFi`
- `ESPAsyncWebServer`
- `ArduinoJson`

### Configuração PlatformIO

Se estiver usando PlatformIO, adicione as seguintes linhas ao seu arquivo `platformio.ini`:

```ini
lib_deps =
    paulstoffregen/OneWire
    milesburton/DallasTemperature
    esphome/ESPAsyncWebServer-esphome
    bblanchon/ArduinoJson
```

## ⚙️ Instalação e Uso

### 1. Clone o repositório:

```bash
git clone https://github.com/murillous/TempMonitorESP32.git
cd TempMonitorESP32
```

### 2. Montagem do Hardware:

1. Conecte o pino de dados do sensor DS18B20 ao **GPIO 4** do ESP32 (use um resistor de pull-up de 4.7kΩ entre o pino de dados e o 3.3V)
2. Conecte o **GPIO 5** do ESP32 ao pino Gate do MOSFET
3. Conecte o pino Source do MOSFET ao terminal negativo (GND) da fonte de 12V
4. Conecte o pino Drain do MOSFET ao fio negativo das pastilhas Peltier
5. Conecte o fio positivo das pastilhas Peltier ao terminal positivo (+12V) da fonte
6. Alimente o ESP32 via USB ou com uma fonte de 5V

### 3. Upload do Firmware:

1. Abra o projeto no VS Code (com PlatformIO) ou na Arduino IDE
2. Compile e envie o código para a sua placa ESP32

### 4. Operação:

1. Após a inicialização, o ESP32 criará uma rede Wi-Fi:
   - **SSID**: `ESP32`
   - **Senha**: `12345678`
2. Conecte seu celular ou computador a esta rede
3. Abra um navegador e acesse o endereço: **http://192.168.4.1**
4. Você terá acesso à interface de controle para operar o dispositivo

## 📊 Interface Web

A interface web é o principal meio de interação com o dispositivo, permitindo controle total sobre seu funcionamento.

## ✒️ Autores e Agradecimentos

- **Autor**: Sérgio Murilo de Andrade Castelhano Filho ([@murillous](https://github.com/murillous))
- **Orientador**: Prof. Carlos Riedel Porto Carreiro

Este projeto foi realizado com o apoio da **Universidade Estadual do Maranhão (UEMA)** e do **Programa Institucional de Bolsas de Iniciação Científica (PIBIC)**.

---

## 📄 Licença

Este projeto está sob a licença MIT. Veja o arquivo LICENSE para mais detalhes.
