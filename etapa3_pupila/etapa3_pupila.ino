// Etapa 3 do Olho de Sauron: a pupila vai e volta sozinha, redesenhando só a faixa onde ela passa.
//
// A íris (iris.h) e o formato da pupila em 8 níveis (pupila.h) vêm de ferramentas/gerar_iris.py.
// O envio para a tela usa o driver esp_lcd da Espressif: a faixa vai por DMA em segundo plano
// enquanto o XIAO já monta a próxima, em dois buffers alternados.
// A posição é calculada pelo tempo real entre quadros, então o movimento não dá trancos
// mesmo quando um quadro demora mais que outro.
//
// Serial: p pausa o movimento, d força dilatar/contrair, s liga/desliga o relatório.

#include <Arduino_GFX_Library.h>   // só pelas constantes da tabela de inicialização
#include <driver/spi_master.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_io_spi.h>
#include <esp_heap_caps.h>
#include <math.h>
#include "Arduino_GC9B72.h"        // tabela gc9b72_init_operations
#include "iris.h"
#include "pupila.h"

#define PINO_SCK  19  // D8  -> SCL
#define PINO_MOSI 18  // D10 -> SDA
#define PINO_CS    1  // D1  -> CS
#define PINO_DC    2  // D2  -> DC
#define PINO_RST  21  // D3  -> RST
#define PINO_BL    0  // D0  -> BL
#define SPI_HZ (40 * 1000 * 1000)

// Comportamento (mesmos valores do plano)
#define DESLOC_MAX 95          // px do centro até o extremo do movimento
#define SUAVIZACAO 0.12f       // fração do caminho andada a cada 20 ms
#define SUAV_DILATACAO 0.05f

#define BRILHO_R 255
#define BRILHO_G 190
#define BRILHO_B 70

#define BUF_COLS 128           // faixa máxima por envio; faixas maiores vão em partes
uint16_t *bufs[2];
int bufAtual = 0;
esp_lcd_panel_io_handle_t io = nullptr;

#define VIN_INI (126 * 126)
uint16_t vinheta[(2 * 180 * 180 - VIN_INI) / 16 + 2];
int meiaFaixa = PUPILA_QL;     // recalculado no setup: até onde o halo é visível

float olhar = 0, dilatacao = 0;
int pupilaXAnt = -1000, nivelAnt = -1;
bool pausado = false, forcarDilatada = false, relatorio = true;
uint32_t tAnterior = 0, tempoTeste = 0, maiorPausa = 0;
uint32_t quadros = 0, somaUsCalculo = 0, ultimoRelatorio = 0;

inline uint16_t fatorVinheta(int x, int y) {
  int dx = 2 * x + 1 - 360, dy = 2 * y + 1 - 360;
  int r2 = (dx * dx + dy * dy) >> 2;
  if (r2 < VIN_INI) return 256;
  return vinheta[(r2 - VIN_INI) >> 4];
}

inline uint16_t misturar(uint16_t base, uint8_t m, int x, int y) {
  uint32_t A = m;
  uint32_t AB = (A * fatorVinheta(x, y)) >> 8;
  uint32_t r5 = base >> 11, g6 = (base >> 5) & 0x3F, b5 = base & 0x1F;
  uint32_t r = (r5 << 3) | (r5 >> 2), g = (g6 << 2) | (g6 >> 4), b = (b5 << 3) | (b5 >> 2);
  r = (r * (512 - A) + BRILHO_R * AB) >> 9;
  g = (g * (512 - A) + BRILHO_G * AB) >> 9;
  b = (b * (512 - A) + BRILHO_B * AB) >> 9;
  return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

// Monta as colunas [x0, x0 + larg) de todas as linhas no buffer, já na ordem de bytes da tela
void montar(uint16_t *dst, int x0, int larg, int px, int nv) {
  for (int y = 0; y < IRIS_ALT; y++) {
    int qj = y >= 180 ? y - 180 : 179 - y;
    const uint8_t *masc = pupilaMascara[nv][qj];
    const uint16_t *lin = &iris[y * IRIS_LARG];
    for (int x = x0; x < x0 + larg; x++) {
      int i = x - px;
      int qi = i >= 0 ? i : -1 - i;
      uint8_t m = qi < meiaFaixa ? masc[qi] : 0;
      uint16_t p;
      if (m == 0) p = lin[x];
      else if (m == 255) p = 0;
      else p = misturar(lin[x], m, x, y);
      *dst++ = __builtin_bswap16(p);
    }
  }
}

void enviarJanela(int x0, int larg) {
  uint8_t c[4] = {(uint8_t)(x0 >> 8), (uint8_t)x0, (uint8_t)((x0 + larg - 1) >> 8), (uint8_t)(x0 + larg - 1)};
  uint8_t r[4] = {0, 0, (uint8_t)((IRIS_ALT - 1) >> 8), (uint8_t)(IRIS_ALT - 1)};
  esp_lcd_panel_io_tx_param(io, 0x2A, c, 4);   // espera o envio anterior terminar
  esp_lcd_panel_io_tx_param(io, 0x2B, r, 4);
}

// Redesenha as colunas [x0, x1) com a pupila centrada em px
void desenharFaixa(int x0, int x1, int px, int nv) {
  x0 = max(x0, 0);
  x1 = min(x1, IRIS_LARG);
  for (int sx = x0; sx < x1; sx += BUF_COLS) {
    int larg = min(BUF_COLS, x1 - sx);
    uint16_t *b = bufs[bufAtual];
    uint32_t t0 = micros();
    montar(b, sx, larg, px, nv);                 // enquanto isso o buffer anterior segue pelo DMA
    somaUsCalculo += micros() - t0;
    enviarJanela(sx, larg);
    esp_lcd_panel_io_tx_color(io, 0x2C, b, larg * IRIS_ALT * 2);
    bufAtual ^= 1;
  }
}

void iniciarTela() {
  spi_bus_config_t bc = {};
  bc.mosi_io_num = PINO_MOSI;
  bc.miso_io_num = -1;
  bc.sclk_io_num = PINO_SCK;
  bc.quadwp_io_num = -1;
  bc.quadhd_io_num = -1;
  bc.max_transfer_sz = BUF_COLS * IRIS_ALT * 2;
  ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bc, SPI_DMA_CH_AUTO));

  esp_lcd_panel_io_spi_config_t ic = {};
  ic.cs_gpio_num = PINO_CS;
  ic.dc_gpio_num = PINO_DC;
  ic.spi_mode = 0;
  ic.pclk_hz = SPI_HZ;
  ic.trans_queue_depth = 10;
  ic.lcd_cmd_bits = 8;
  ic.lcd_param_bits = 8;
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &ic, &io));

  pinMode(PINO_RST, OUTPUT);
  digitalWrite(PINO_RST, HIGH); delay(10);
  digitalWrite(PINO_RST, LOW);  delay(20);
  digitalWrite(PINO_RST, HIGH); delay(120);

  uint8_t madctl = 0x00;
  esp_lcd_panel_io_tx_param(io, 0x36, &madctl, 1);

  // Percorre a mesma tabela da etapa 1, no formato da Arduino_GFX
  const uint8_t *op = gc9b72_init_operations;
  size_t n = sizeof(gc9b72_init_operations), i = 0;
  while (i < n) {
    uint8_t o = op[i++];
    if (o == BEGIN_WRITE || o == END_WRITE) continue;
    if (o == DELAY) { delay(op[i++]); continue; }
    if (o == WRITE_C8_D8) { esp_lcd_panel_io_tx_param(io, op[i], &op[i + 1], 1); i += 2; continue; }
    if (o == WRITE_C8_D16) { esp_lcd_panel_io_tx_param(io, op[i], &op[i + 1], 2); i += 3; continue; }
    if (o == WRITE_COMMAND_8) {
      uint8_t cmd = op[i++];
      if (i < n && op[i] == WRITE_BYTES) {          // comando seguido dos seus parâmetros
        uint8_t len = op[i + 1];
        esp_lcd_panel_io_tx_param(io, cmd, &op[i + 2], len);
        i += 2 + len;
      } else {
        esp_lcd_panel_io_tx_param(io, cmd, nullptr, 0);
      }
      continue;
    }
    Serial.printf("ERRO: operação %u não tratada na tabela\n", o);
    return;
  }
}

void setup() {
  Serial.begin(115200);
  // Sem isto, com o USB ligado e nenhum programa lendo a porta, cada print espera até 2 s
  // (20 x 100 ms no HWCDC do núcleo 3.3.x) e a pupila congela e depois salta.
  Serial.setTxTimeoutMs(0);
  delay(1500);
  Serial.println("\nOlho de Sauron - etapa 3: pupila em movimento (esp_lcd + DMA)");

  pinMode(PINO_BL, OUTPUT);
  digitalWrite(PINO_BL, HIGH);

  for (size_t k = 0; k < sizeof(vinheta) / sizeof(vinheta[0]); k++) {
    float r = sqrtf(VIN_INI + k * 16.0f);
    vinheta[k] = (uint16_t)((1 - fminf(fmaxf((r - 126.0f) / 54.0f, 0), 1) * 0.85f) * 256 + 0.5f);
  }

  // Até onde o halo tem algum valor, no nível mais dilatado
  meiaFaixa = 0;
  for (int n = 0; n < PUPILA_NIVEIS; n++)
    for (int j = 0; j < PUPILA_QA; j++)
      for (int k = PUPILA_QL - 1; k >= 0; k--)
        if (pupilaMascara[n][j][k]) { meiaFaixa = max(meiaFaixa, k + 1); break; }
  Serial.printf("meia faixa útil: %d px (de %d)\n", meiaFaixa, PUPILA_QL);

  for (int k = 0; k < 2; k++) {
    bufs[k] = (uint16_t *)heap_caps_malloc(BUF_COLS * IRIS_ALT * 2, MALLOC_CAP_DMA);
    if (!bufs[k]) { Serial.println("ERRO: sem memória para os buffers"); while (true) delay(1000); }
  }

  iniciarTela();
  desenharFaixa(0, IRIS_LARG, 180, 0);   // tela inteira: íris + pupila no centro
  pupilaXAnt = 180;
  nivelAnt = 0;
  tAnterior = millis();
  ultimoRelatorio = millis();
}

void loop() {
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == 'p') { pausado = !pausado; Serial.println(pausado ? "> pausado" : "> movendo"); }
    if (ch == 'd') { forcarDilatada = !forcarDilatada; Serial.printf("> dilatação forçada %s\n", forcarDilatada ? "LIGADA" : "desligada"); }
    if (ch == 's') relatorio = !relatorio;
  }

  // Movimento pelo tempo real: o mesmo resultado de passos de 20 ms, sem depender do ritmo dos quadros
  uint32_t agora = millis();
  uint32_t pausa = agora - tAnterior;
  if (agora > 5000 && pausa > maiorPausa) maiorPausa = pausa;   // maior intervalo entre voltas do loop
  float dt = (float)pausa;
  tAnterior = agora;
  if (!pausado) {
    tempoTeste += (uint32_t)dt;
    float alvo = sinf(tempoTeste / 1500.0f);                       // valor de teste: vai e volta
    olhar += (alvo - olhar) * (1 - powf(1 - SUAVIZACAO, dt / 20));
    bool dilatar = forcarDilatada || ((tempoTeste / 4000) % 2 == 1);
    dilatacao += ((dilatar ? 1.0f : 0.0f) - dilatacao) * (1 - powf(1 - SUAV_DILATACAO, dt / 20));
  }
  int pupilaX = 180 + (int)lroundf(olhar * DESLOC_MAX);
  int nivel = (int)(dilatacao * (PUPILA_NIVEIS - 1) + 0.5f);

  if (pupilaX != pupilaXAnt || nivel != nivelAnt) {
    desenharFaixa(min(pupilaX, pupilaXAnt) - meiaFaixa, max(pupilaX, pupilaXAnt) + meiaFaixa, pupilaX, nivel);
    pupilaXAnt = pupilaX;
    nivelAnt = nivel;
    quadros++;
  }

  if (relatorio && millis() - ultimoRelatorio >= 2000) {
    Serial.printf("%.1f quadros/s, montagem %lu us/quadro, maior pausa desde o início %lu ms, ligado há %lu s\n",
                  quadros * 1000.0f / (millis() - ultimoRelatorio),
                  quadros ? somaUsCalculo / quadros : 0, maiorPausa, millis() / 1000);
    quadros = 0;
    somaUsCalculo = 0;
    ultimoRelatorio = millis();
  }
}
