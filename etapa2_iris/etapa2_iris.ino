// Etapa 2 do Olho de Sauron: mostrar a íris gravada na flash, com a pupila calculada por cima.
// A íris (iris.h) vem de ferramentas/gerar_iris.py. A pupila usa a mesma fórmula do gerador,
// então a tela fica igual a previa/previa_olho.png.
//
// A pupila alterna entre contraída (sala vazia) e dilatada (alguém detectado) a cada 3 s.
// Serial: p pausa/retoma, d alterna na hora.

#include <Arduino_GFX_Library.h>
#include <math.h>
#include "Arduino_GC9B72.h"
#include "iris.h"

#define PINO_SCK  19  // D8  -> SCL
#define PINO_MOSI 18  // D10 -> SDA
#define PINO_CS    1  // D1  -> CS
#define PINO_DC    2  // D2  -> DC
#define PINO_RST  21  // D3  -> RST
#define PINO_BL    0  // D0  -> BL
#define SPI_HZ 40000000

// Pupila: semilarguras em px (mesmos valores da prévia interativa)
#define PUPILA_W_CONTRAIDA 7.0f
#define PUPILA_H_CONTRAIDA 132.0f
#define PUPILA_W_DILATADA 16.0f
#define PUPILA_H_DILATADA 142.0f
#define MARGEM_BRILHO 45      // até onde o halo laranja chega, em px
#define SIGMA_BRILHO 11.0f
const float COR_BRILHO[3] = {255, 190, 70};

Arduino_DataBus *bus = new Arduino_ESP32SPI(PINO_DC, PINO_CS, PINO_SCK, PINO_MOSI, GFX_NOT_DEFINED);
Arduino_GC9B72 *tela = new Arduino_GC9B72(bus, PINO_RST, 0, false);

// Região que a pupila pode ocupar (calculada para a maior pupila, para apagar a anterior)
const int MEIA_L = (int)PUPILA_W_DILATADA + MARGEM_BRILHO;   // 61
const int MEIA_A = min(180, (int)PUPILA_H_DILATADA + MARGEM_BRILHO);
const int REG_X = 180 - MEIA_L, REG_Y = 180 - MEIA_A;
const int REG_L = 2 * MEIA_L, REG_A = 2 * MEIA_A;
uint16_t *regiao = nullptr;

bool dilatada = false;
bool pausado = false;
unsigned long proximaTroca = 0;

// Distância de um ponto (y0, y1 >= 0, fora da elipse) até a elipse de semieixos e0 >= e1.
// Método de Eberly por bissecção, igual ao gerar_iris.py.
float distanciaElipse(float y0, float y1, float e0, float e1) {
  float z0 = y0 / e0, z1 = y1 / e1;
  float r0 = (e0 / e1) * (e0 / e1);
  float n0 = r0 * z0;
  float s0 = z1 - 1, s1 = sqrtf(n0 * n0 + z1 * z1) - 1;
  for (int i = 0; i < 40; i++) {
    float s = (s0 + s1) * 0.5f;
    float a = n0 / (s + r0), b = z1 / (s + 1);
    float g = a * a + b * b - 1;
    if (g > 0) s0 = s; else s1 = s;
  }
  float s = (s0 + s1) * 0.5f;
  float x0 = r0 * y0 / (s + r0), x1 = y1 / (s + 1);
  return sqrtf((x0 - y0) * (x0 - y0) + (x1 - y1) * (x1 - y1));
}

inline uint16_t misturar(uint16_t base, float a, float aBrilho) {
  float r = ((base >> 11) & 0x1F) * (255.0f / 31);
  float g = ((base >> 5) & 0x3F) * (255.0f / 63);
  float b = (base & 0x1F) * (255.0f / 31);
  r = r * (1 - a) + COR_BRILHO[0] * aBrilho;
  g = g * (1 - a) + COR_BRILHO[1] * aBrilho;
  b = b * (1 - a) + COR_BRILHO[2] * aBrilho;
  uint16_t R5 = (uint16_t)fminf(r + 0.5f, 255) >> 3;
  uint16_t G6 = (uint16_t)fminf(g + 0.5f, 255) >> 2;
  uint16_t B5 = (uint16_t)fminf(b + 0.5f, 255) >> 3;
  return (R5 << 11) | (G6 << 5) | B5;
}

// Monta a região com íris + pupila e manda para a tela
void desenharPupila(float pw, float ph) {
  unsigned long t0 = millis();
  // um quadrante basta: a forma é simétrica; a íris de baixo é lida em cada posição
  for (int j = 0; j < MEIA_A; j++) {
    float dy = j + 0.5f;
    for (int i = 0; i < MEIA_L; i++) {
      float dx = i + 0.5f;
      float e = sqrtf((dx / pw) * (dx / pw) + (dy / ph) * (dy / ph));
      float a = 0, aBrilho = 0;
      bool dentro = e <= 1;
      if (!dentro && (e - 1) * pw < MARGEM_BRILHO) {
        float d = distanciaElipse(dy, dx, ph, pw);
        a = 0.95f * 0.5f * erfcf(d / (SIGMA_BRILHO * 1.41421356f));
        float r = sqrtf(dx * dx + dy * dy);
        float v = 1 - fminf(fmaxf((r - 126.0f) / 54.0f, 0), 1) * 0.85f;
        aBrilho = a * v;
      }
      // as quatro posições espelhadas
      int xs[2] = {MEIA_L - 1 - i, MEIA_L + i};
      int ys[2] = {MEIA_A - 1 - j, MEIA_A + j};
      for (int yy : ys) {
        for (int xx : xs) {
          uint16_t base = iris[(REG_Y + yy) * IRIS_LARG + (REG_X + xx)];
          regiao[yy * REG_L + xx] = dentro ? 0 : (a > 0.002f ? misturar(base, a, aBrilho) : base);
        }
      }
    }
  }
  unsigned long t1 = millis();
  tela->draw16bitRGBBitmap(REG_X, REG_Y, regiao, REG_L, REG_A);
  Serial.printf("pupila %s: calculada em %lu ms, enviada em %lu ms\n",
                dilatada ? "DILATADA" : "contraída", t1 - t0, millis() - t1);
}

void mostrarPupila() {
  if (dilatada) desenharPupila(PUPILA_W_DILATADA, PUPILA_H_DILATADA);
  else desenharPupila(PUPILA_W_CONTRAIDA, PUPILA_H_CONTRAIDA);
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("\nOlho de Sauron - etapa 2: íris da flash + pupila");

  pinMode(PINO_BL, OUTPUT);
  digitalWrite(PINO_BL, HIGH);

  regiao = (uint16_t *)malloc(REG_L * REG_A * sizeof(uint16_t));
  if (!regiao || !tela->begin(SPI_HZ)) {
    Serial.println("ERRO: memória ou tela");
    while (true) delay(1000);
  }

  unsigned long t0 = millis();
  tela->draw16bitRGBBitmap(0, 0, (uint16_t *)iris, IRIS_LARG, IRIS_ALT);
  Serial.printf("íris desenhada em %lu ms\n", millis() - t0);

  mostrarPupila();
  proximaTroca = millis() + 3000;
}

void loop() {
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == 'p') { pausado = !pausado; Serial.println(pausado ? "> pausado" : "> continuando"); }
    if (ch == 'd') { dilatada = !dilatada; mostrarPupila(); }
  }
  if (!pausado && millis() >= proximaTroca) {
    dilatada = !dilatada;
    mostrarPupila();
    proximaTroca = millis() + 3000;
  }
}
