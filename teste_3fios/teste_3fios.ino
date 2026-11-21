// Testa a tela em SPI de 3 fios (9 bits: 1º bit = 0 comando / 1 dado), em bit-bang.
// Lê 0x0A e 0x04 pelo SDA e pelo SDO; depois inicializa e pinta a tela de VERMELHO e BRANCO.
#include <Arduino_GFX_Library.h>
#include "Arduino_GC9B72.h"   // só para reaproveitar a tabela gc9b72_init_operations

#define PINO_SCL 19
#define PINO_SDA 18
#define PINO_SDO 20
#define PINO_CS   1
#define PINO_DC   2
#define PINO_RST 21
#define PINO_BL   0

inline void clk() { delayMicroseconds(2); digitalWrite(PINO_SCL, HIGH); delayMicroseconds(2); digitalWrite(PINO_SCL, LOW); }

void bit9(bool dado, uint8_t b) {
  digitalWrite(PINO_SDA, dado); clk();
  for (int i = 7; i >= 0; i--) { digitalWrite(PINO_SDA, (b >> i) & 1); clk(); }
}
void cmd(uint8_t c) { bit9(false, c); }
void dat(uint8_t d) { bit9(true, d); }

uint32_t ler(uint8_t c, int nbits, int pino) {
  pinMode(PINO_SDA, OUTPUT);
  digitalWrite(PINO_CS, LOW);
  cmd(c);
  if (pino == PINO_SDA) pinMode(PINO_SDA, INPUT);
  uint32_t v = 0;
  for (int i = 0; i < nbits; i++) {
    delayMicroseconds(2); digitalWrite(PINO_SCL, HIGH); delayMicroseconds(2);
    v = (v << 1) | digitalRead(pino);
    digitalWrite(PINO_SCL, LOW);
  }
  digitalWrite(PINO_CS, HIGH);
  pinMode(PINO_SDA, OUTPUT);
  return v;
}

// Executa a tabela de init da Arduino_GFX convertendo para 9 bits
void executarTabela(const uint8_t *op, size_t len) {
  size_t i = 0;
  while (i < len) {
    uint8_t o = op[i++];
    switch (o) {
      case BEGIN_WRITE: digitalWrite(PINO_CS, LOW); break;
      case END_WRITE: digitalWrite(PINO_CS, HIGH); break;
      case DELAY: delay(op[i++]); break;
      case WRITE_COMMAND_8: cmd(op[i++]); break;
      case WRITE_C8_D8: cmd(op[i++]); dat(op[i++]); break;
      case WRITE_C8_D16: cmd(op[i++]); dat(op[i++]); dat(op[i++]); break;
      case WRITE_BYTES: { uint8_t n = op[i++]; while (n--) dat(op[i++]); } break;
      default: Serial.printf("op desconhecida %d\n", o); return;
    }
  }
}

void pintar(uint16_t cor, int y0, int h) {
  digitalWrite(PINO_CS, LOW);
  cmd(0x2A); dat(0); dat(0); dat(359 >> 8); dat(359 & 0xFF);
  cmd(0x2B); dat(y0 >> 8); dat(y0 & 0xFF); dat((y0 + h - 1) >> 8); dat((y0 + h - 1) & 0xFF);
  cmd(0x2C);
  for (uint32_t k = 0; k < 360UL * h; k++) { dat(cor >> 8); dat(cor & 0xFF); }
  digitalWrite(PINO_CS, HIGH);
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  pinMode(PINO_SCL, OUTPUT); digitalWrite(PINO_SCL, LOW);
  pinMode(PINO_SDA, OUTPUT);
  pinMode(PINO_CS, OUTPUT); digitalWrite(PINO_CS, HIGH);
  pinMode(PINO_DC, OUTPUT); digitalWrite(PINO_DC, LOW);
  pinMode(PINO_RST, OUTPUT);
  pinMode(PINO_SDO, INPUT_PULLDOWN);
  pinMode(PINO_BL, OUTPUT); digitalWrite(PINO_BL, HIGH);
}

void loop() {
  digitalWrite(PINO_RST, LOW); delay(120); digitalWrite(PINO_RST, HIGH); delay(120);
  Serial.println("\n=== SPI 3 fios ===");
  Serial.printf("após reset: 0x0A SDA=%08lX SDO=%08lX | 0x04 SDA=%08lX SDO=%08lX\n",
                ler(0x0A, 16, PINO_SDA), ler(0x0A, 16, PINO_SDO), ler(0x04, 32, PINO_SDA), ler(0x04, 32, PINO_SDO));
  executarTabela(gc9b72_init_operations, sizeof(gc9b72_init_operations));
  Serial.printf("após init : 0x0A SDA=%08lX SDO=%08lX\n", ler(0x0A, 16, PINO_SDA), ler(0x0A, 16, PINO_SDO));
  Serial.println("pintando: metade de cima VERMELHA, metade de baixo BRANCA (leva ~10 s)");
  pintar(0xF800, 0, 180);
  pintar(0xFFFF, 180, 180);
  Serial.println("pronto");
  delay(5000);
}
