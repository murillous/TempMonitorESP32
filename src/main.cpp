#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define PINO_DS18B20 4
#define pinoPeltier 5

const float TEMP_ALVO = 30.0;
const int MAX_SAIDA_PELTIER = 255;  // Potência máxima com fonte 5A
const int MIN_SAIDA_PELTIER = 0;

OneWire oneWire(PINO_DS18B20);
DallasTemperature sensores(&oneWire);

#define LARGURA_OLED 128
#define ALTURA_OLED 64
#define RESET_OLED -1

Adafruit_SSD1306 display(LARGURA_OLED, ALTURA_OLED, &Wire, RESET_OLED);

void setup() {
  Serial.begin(115200);
  sensores.begin();
  
  pinMode(pinoPeltier, OUTPUT);
  ledcAttachPin(pinoPeltier, 0);
  ledcSetup(0, 1000, 8);   // 1kHz - ideal para 2SK1388
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.display();
  
  Serial.println("=== Controle Peltier - Fonte 12V/5A ===");
}

void loop() {
  sensores.requestTemperatures();
  float temperatura = sensores.getTempCByIndex(0);
  
  // Verificar sensor
  if (temperatura == DEVICE_DISCONNECTED_C || temperatura < -50) {
    Serial.println("ERRO: Sensor desconectado!");
    return;
  }
  
  int saidaPeltier = 0;
  String status = "DESLIGADO";
  
  if (temperatura >= TEMP_ALVO) {
    saidaPeltier = MAX_SAIDA_PELTIER;  // MÁXIMA POTÊNCIA - fonte 5A aguenta!
    status = "RESFR. MÁXIMO";
  }
  
  ledcWrite(0, saidaPeltier);
  
  // Log detalhado
  Serial.print("Temp: ");
  Serial.print(temperatura, 2);
  Serial.print("°C | Alvo: ");
  Serial.print(TEMP_ALVO);
  Serial.print("°C | PWM: ");
  Serial.print(saidaPeltier);
  Serial.print(" (");
  Serial.print((saidaPeltier * 100) / 255);
  Serial.print("%) | Status: ");
  Serial.println(status);
  
  // Display
  display.clearDisplay();
  
  // Temperatura atual - grande
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Temperatura:");
  display.setTextSize(3);
  display.setCursor(0, 12);
  display.print(temperatura, 1);
  display.setTextSize(1);
  display.setCursor(100, 20);
  display.write(167);
  display.print("C");
  
  // Informações de controle
  display.setTextSize(1);
  display.setCursor(0, 40);
  display.print("PWM: ");
  display.print(saidaPeltier);
  display.print(" (");
  display.print((saidaPeltier * 100) / 255);
  display.print("%)");
  
  display.setCursor(0, 52);
  display.print("Status: ");
  display.print(status);
  
  display.display();
  delay(200);  // Delay maior para estabilizar leituras
}