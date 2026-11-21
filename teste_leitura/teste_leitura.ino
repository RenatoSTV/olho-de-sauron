// Teste de comunicação com a GC9B72, tudo em bit-bang lento (~100 kHz).
// 1) Pisca o BL (D0) para confirmar que o fio branco do BL está mesmo no D0.
// 2) Lê o estado da tela (0x0A, 0x09) e o ID (0x04) antes e depois de mandar
//    SLPOUT + DISPON. Se a resposta mudar, a tela está recebendo comandos.
// A leitura é feita pelo SDO (fio opcional no D9) e pelo próprio SDA (modo 3 fios).

#define PINO_SCL 19  // D8
#define PINO_SDA 18  // D10
#define PINO_SDO 20  // D9 (opcional: fio do SDO da tela)
#define PINO_CS   1  // D1
#define PINO_DC   2  // D2
#define PINO_RST 21  // D3
#define PINO_BL   0  // D0

void meioCiclo() { delayMicroseconds(5); }

void escreveByte(uint8_t b) {
  pinMode(PINO_SDA, OUTPUT);
  for (int i = 7; i >= 0; i--) {
    digitalWrite(PINO_SDA, (b >> i) & 1);
    meioCiclo();
    digitalWrite(PINO_SCL, HIGH);
    meioCiclo();
    digitalWrite(PINO_SCL, LOW);
  }
}

void comando(uint8_t c) {
  digitalWrite(PINO_CS, LOW);
  digitalWrite(PINO_DC, LOW);
  escreveByte(c);
  digitalWrite(PINO_DC, HIGH);
  digitalWrite(PINO_CS, HIGH);
}

// Manda o comando e lê n bits pelo pino indicado
uint32_t ler(uint8_t c, int nbits, int pinoLeitura) {
  digitalWrite(PINO_CS, LOW);
  digitalWrite(PINO_DC, LOW);
  escreveByte(c);
  digitalWrite(PINO_DC, HIGH);
  if (pinoLeitura == PINO_SDA) pinMode(PINO_SDA, INPUT);
  uint32_t v = 0;
  for (int i = 0; i < nbits; i++) {
    meioCiclo();
    digitalWrite(PINO_SCL, HIGH);
    meioCiclo();
    v = (v << 1) | digitalRead(pinoLeitura);
    digitalWrite(PINO_SCL, LOW);
  }
  digitalWrite(PINO_CS, HIGH);
  pinMode(PINO_SDA, OUTPUT);
  return v;
}

void bits(uint32_t v, int n) {
  for (int i = n - 1; i >= 0; i--) {
    Serial.print((v >> i) & 1);
    if (i % 8 == 0 && i) Serial.print(' ');
  }
}

void relatorio(const char *quando) {
  Serial.printf("\n--- %s ---\n", quando);
  const struct { uint8_t c; int n; const char *nome; } leituras[] = {
      {0x0A, 16, "0x0A estado de energia"},
      {0x09, 40, "0x09 estado geral    "},
      {0x04, 32, "0x04 ID              "},
  };
  for (auto &l : leituras) {
    uint32_t sdo = ler(l.c, l.n > 32 ? 32 : l.n, PINO_SDO);
    uint32_t sda = ler(l.c, l.n > 32 ? 32 : l.n, PINO_SDA);
    Serial.printf("%s  SDO: ", l.nome);
    bits(sdo, l.n > 32 ? 32 : l.n);
    Serial.print("   SDA: ");
    bits(sda, l.n > 32 ? 32 : l.n);
    Serial.println();
  }
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  pinMode(PINO_SCL, OUTPUT); digitalWrite(PINO_SCL, LOW);
  pinMode(PINO_SDA, OUTPUT); digitalWrite(PINO_SDA, LOW);
  pinMode(PINO_CS, OUTPUT);  digitalWrite(PINO_CS, HIGH);
  pinMode(PINO_DC, OUTPUT);  digitalWrite(PINO_DC, HIGH);
  pinMode(PINO_RST, OUTPUT); digitalWrite(PINO_RST, HIGH);
  pinMode(PINO_SDO, INPUT_PULLDOWN);
  pinMode(PINO_BL, OUTPUT);
}

void loop() {
  Serial.println("\n===== PISCA BL: a luz de fundo deve apagar e acender 3 vezes =====");
  for (int i = 0; i < 3; i++) {
    digitalWrite(PINO_BL, LOW);  Serial.println("BL apagado"); delay(1000);
    digitalWrite(PINO_BL, HIGH); Serial.println("BL aceso");   delay(1000);
  }

  // reset por hardware com pulso longo, como na biblioteca de referência
  digitalWrite(PINO_RST, LOW);  delay(120);
  digitalWrite(PINO_RST, HIGH); delay(120);
  relatorio("logo após o reset (esperado: 0x0A = 00001000, dormindo)");

  comando(0x11);  // sleep out
  delay(150);
  comando(0x29);  // display on
  delay(50);
  relatorio("após SLPOUT + DISPON (esperado: 0x0A com bits 1001 1100)");

  delay(4000);
}
