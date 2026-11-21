// Diagnóstico da tela GC9B72: testa barramentos SPI e sequências de inicialização.
// Cada teste dura cerca de 8 s e desenha N faixas brancas sobre fundo vermelho,
// onde N é o número do teste. Depois a placa reinicia e passa para o próximo.
// Conte as faixas: o número que aparecer diz qual combinação funciona.

#include <Arduino_GFX_Library.h>
#include <Preferences.h>
#include "Arduino_GC9B72.h"

#define PINO_SCK  19  // D8  -> SCL
#define PINO_MOSI 18  // D10 -> SDA
#define PINO_CS    1  // D1  -> CS
#define PINO_DC    2  // D2  -> DC
#define PINO_RST  21  // D3  -> RES
#define LARG 360
#define ALT  360

// Inicialização mínima, só comandos padrão (sem registradores do fabricante)
static const uint8_t init_minima[] = {
    BEGIN_WRITE, WRITE_COMMAND_8, 0x01, END_WRITE, DELAY, 150,  // reset por software
    BEGIN_WRITE, WRITE_COMMAND_8, 0x11, END_WRITE, DELAY, 150,  // sai do sleep
    BEGIN_WRITE,
    WRITE_C8_D8, 0x3A, 0x55,  // RGB565
    WRITE_C8_D8, 0x36, 0x00,
    WRITE_COMMAND_8, 0x21,    // inversão (comum em painéis IPS)
    WRITE_COMMAND_8, 0x29,    // liga a imagem
    END_WRITE, DELAY, 20};

struct Teste {
  const char *barramento;
  const char *init;
};

const Teste testes[] = {
    {"ESP32SPI (registradores)", "fabricante GC9B72"},
    {"HWSPI (SPIClass)",         "fabricante GC9B72"},
    {"SWSPI (bit-bang, lento)",  "fabricante GC9B72"},
    {"SWSPI (bit-bang, lento)",  "minima"},
    {"SWSPI (bit-bang, lento)",  "GC9A01"},
    {"HWSPI (SPIClass)",         "minima"},
};
const int NUM_TESTES = sizeof(testes) / sizeof(testes[0]);

Arduino_DataBus *bus = nullptr;

void retangulo(int x, int y, int w, int h, uint16_t cor) {
  bus->beginWrite();
  bus->writeC8D16D16(0x2A, x, x + w - 1);
  bus->writeC8D16D16(0x2B, y, y + h - 1);
  bus->writeCommand(0x2C);
  bus->writeRepeat(cor, (uint32_t)w * h);
  bus->endWrite();
}

void resetPorHardware() {
  pinMode(PINO_RST, OUTPUT);
  digitalWrite(PINO_RST, HIGH);
  delay(10);
  digitalWrite(PINO_RST, LOW);
  delay(20);
  digitalWrite(PINO_RST, HIGH);
  delay(150);
}

void setup() {
  pinMode(0, OUTPUT);       // D0 -> BLK: luz de fundo sempre ligada
  digitalWrite(0, HIGH);

  Serial.begin(115200);
  delay(1500);

  Preferences prefs;
  prefs.begin("diag", false);
  int n = prefs.getInt("teste", 0) % NUM_TESTES;
  prefs.putInt("teste", (n + 1) % NUM_TESTES);
  prefs.end();

  const Teste &t = testes[n];
  Serial.printf("\n=== TESTE %d de %d: barramento %s, inicialização %s -> %d faixa(s) branca(s)\n",
                n + 1, NUM_TESTES, t.barramento, t.init, n + 1);

  if (n == 0)
    bus = new Arduino_ESP32SPI(PINO_DC, PINO_CS, PINO_SCK, PINO_MOSI, GFX_NOT_DEFINED);
  else if (n == 1 || n == 5)
    bus = new Arduino_HWSPI(PINO_DC, PINO_CS, PINO_SCK, PINO_MOSI, GFX_NOT_DEFINED);
  else
    bus = new Arduino_SWSPI(PINO_DC, PINO_CS, PINO_SCK, PINO_MOSI, GFX_NOT_DEFINED);

  if (!bus->begin(4000000, SPI_MODE0)) {
    Serial.println("ERRO: bus->begin falhou");
  }

  resetPorHardware();
  unsigned long t0 = millis();
  if (strcmp(t.init, "minima") == 0)
    bus->batchOperation(init_minima, sizeof(init_minima));
  else if (strcmp(t.init, "GC9A01") == 0)
    bus->batchOperation(gc9a01_init_operations, sizeof(gc9a01_init_operations));
  else {
    bus->beginWrite();
    bus->writeC8D8(0x36, 0x00);
    bus->endWrite();
    bus->batchOperation(gc9b72_init_operations, sizeof(gc9b72_init_operations));
  }
  Serial.printf("init em %lu ms\n", millis() - t0);

  t0 = millis();
  retangulo(0, 0, LARG, ALT, RGB565_RED);
  int faixas = n + 1;
  int passo = ALT / (faixas + 1);
  for (int i = 1; i <= faixas; i++) {
    retangulo(30, i * passo - 10, LARG - 60, 20, RGB565_WHITE);
  }
  Serial.printf("desenho em %lu ms\n", millis() - t0);
}

void loop() {
  static unsigned long inicio = millis();
  if (millis() - inicio > 8000) {
    Serial.println("próximo teste...");
    Serial.flush();
    delay(100);
    ESP.restart();
  }
}
