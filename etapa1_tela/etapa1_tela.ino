// Etapa 1 do Olho de Sauron: acender a tela GC9B72 com cores sólidas.
// Placa: Seeed XIAO ESP32-C6. Gravar com CDCOnBoot=cdc para ver a serial.
//
// A tela mostra, 1,5 s cada: VERMELHO, VERDE, AZUL, BRANCO, PRETO e um olho de teste.
// O nome de cada cor sai na serial, para comparar com o que aparece.
//
// Comandos pela serial (115200), sem regravar:
//   i  inverte as cores   (use se o PRETO aparecer branco)
//   b  troca RGB <-> BGR  (use se o VERMELHO aparecer azul)
//   r  gira a tela 90°
//   + / -  brilho da luz de fundo
//   p  pausa ou retoma a sequência

#include <Arduino_GFX_Library.h>
#include "Arduino_GC9B72.h"

// Ligações do doc (XIAO -> tela)
#define PINO_SCK  19  // D8  -> SCL
#define PINO_MOSI 18  // D10 -> SDA
#define PINO_CS    1  // D1  -> CS
#define PINO_DC    2  // D2  -> DC
#define PINO_RST  21  // D3  -> RES
#define PINO_BLK   0  // D0  -> BLK

// 40 MHz costuma funcionar com jumpers curtos. Se aparecer chiado ou cores erradas
// só em parte da tela, baixe para 20000000.
#define SPI_HZ 40000000

Arduino_DataBus *bus = new Arduino_ESP32SPI(PINO_DC, PINO_CS, PINO_SCK, PINO_MOSI, GFX_NOT_DEFINED);
Arduino_GC9B72 *tela = new Arduino_GC9B72(bus, PINO_RST, 0 /* rotação */, false /* ips */);

bool invertida = false;
bool pausado = false;
int brilho = 255;
int passo = 0;
unsigned long proximaTroca = 0;

void aplicarBrilho() {
  ledcWrite(PINO_BLK, brilho);
}

void desenharOlhoTeste() {
  const int c = GC9B72_TFTWIDTH / 2;
  tela->fillScreen(RGB565_BLACK);
  for (int r = 175; r > 0; r -= 5) {                      // íris em degradê, de vermelho escuro a amarelo
    uint8_t g = map(r, 175, 0, 20, 220);
    tela->fillCircle(c, c, r, tela->color565(255, g, 0));
  }
  tela->fillEllipse(c, c, 22, 150, RGB565_BLACK);          // pupila em fenda
  tela->drawCircle(c, c, 179, RGB565_WHITE);               // borda: mostra se o círculo está centrado
}

void mostrarPasso() {
  const char *nomes[] = {"VERMELHO", "VERDE", "AZUL", "BRANCO", "PRETO", "OLHO DE TESTE"};
  const uint16_t cores[] = {RGB565_RED, RGB565_GREEN, RGB565_BLUE, RGB565_WHITE, RGB565_BLACK};

  unsigned long t0 = micros();
  if (passo < 5) tela->fillScreen(cores[passo]);
  else desenharOlhoTeste();
  unsigned long dt = micros() - t0;

  Serial.printf("Tela: %-14s (desenhado em %lu ms)\n", nomes[passo], dt / 1000);
}

void lerComandos() {
  while (Serial.available()) {
    char ch = Serial.read();
    switch (ch) {
      case 'i':
        invertida = !invertida;
        tela->invertDisplay(invertida);
        Serial.printf("> inversão %s\n", invertida ? "LIGADA" : "desligada");
        break;
      case 'b':
        tela->setBGR(!tela->bgr());
        Serial.printf("> ordem das cores: %s\n", tela->bgr() ? "BGR" : "RGB");
        mostrarPasso();
        break;
      case 'r':
        tela->setRotation((tela->getRotation() + 1) % 4);
        Serial.printf("> rotação %d\n", tela->getRotation());
        mostrarPasso();
        break;
      case '+':
      case '-':
        brilho = constrain(brilho + (ch == '+' ? 32 : -32), 0, 255);
        aplicarBrilho();
        Serial.printf("> brilho %d/255\n", brilho);
        break;
      case 'p':
        pausado = !pausado;
        Serial.println(pausado ? "> pausado" : "> continuando");
        break;
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1500);  // dá tempo de abrir o monitor serial
  Serial.println("\nOlho de Sauron - etapa 1: teste da tela GC9B72");

  ledcAttach(PINO_BLK, 5000, 8);  // luz de fundo com PWM de 8 bits
  aplicarBrilho();

  if (!tela->begin(SPI_HZ)) {
    Serial.println("ERRO: tela->begin() falhou");
    while (true) delay(1000);
  }
  Serial.println("Tela iniciada. Comandos: i, b, r, +, -, p");
  mostrarPasso();
  proximaTroca = millis() + 1500;
}

void loop() {
  lerComandos();
  if (!pausado && millis() >= proximaTroca) {
    passo = (passo + 1) % 6;
    mostrarPasso();
    proximaTroca = millis() + (passo == 5 ? 4000 : 1500);
  }
}
