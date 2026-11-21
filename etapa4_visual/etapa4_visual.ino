// Etapa 4 (modo visual) - ESP32-CAM manda pela USB a imagem que a deteccao analisa
// e o resultado dela, para ver no computador com ferramentas/ver_camera.py.
// Mesma deteccao do etapa4_camera, mas a serial vai a 921600 e em binario
// (o CH340 do programador nao aguentou 2 Mbaud):
// NAO ligar no XIAO com este sketch. Depois do teste, regravar etapa4_camera.
//
// Pacote: "OLH4" | largura uint16 | altura uint16 | x int16 (-1 = sem movimento)
//         | qtd uint16 | luz uint8 | 160x120 pixels em 4 bits (2 por byte, o 1o no nibble baixo)
//         | mascara 80x60 bits (600 bytes, 1 = o pixel amostrado mudou)
#include "esp_camera.h"

#define W 160
#define H 120
#define GW (W / 2)
#define GH (H / 2)
#define LIMIAR_PIXEL 25
#define MIN_PIXELS   40

static uint8_t anterior[W * H];
static bool temAnterior = false;
static uint8_t mascara[GW * GH / 8];
static uint8_t img4[W * H / 2];

void setup() {
  Serial.begin(921600);

  camera_config_t c = {};
  c.ledc_channel = LEDC_CHANNEL_0;
  c.ledc_timer   = LEDC_TIMER_0;
  c.pin_d0 = 5;  c.pin_d1 = 18; c.pin_d2 = 19; c.pin_d3 = 21;
  c.pin_d4 = 36; c.pin_d5 = 39; c.pin_d6 = 34; c.pin_d7 = 35;
  c.pin_xclk = 0;  c.pin_pclk = 22; c.pin_vsync = 25; c.pin_href = 23;
  c.pin_sccb_sda = 26; c.pin_sccb_scl = 27;
  c.pin_pwdn = 32; c.pin_reset = -1;
  c.xclk_freq_hz = 20000000;
  c.pixel_format = PIXFORMAT_GRAYSCALE;
  c.frame_size   = FRAMESIZE_QQVGA;
  c.fb_count     = 2;
  c.fb_location  = CAMERA_FB_IN_PSRAM;
  c.grab_mode    = CAMERA_GRAB_LATEST;

  if (esp_camera_init(&c) != ESP_OK) {
    while (true) { Serial.println("# ERRO camera"); delay(2000); }
  }
}

void loop() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) { delay(50); return; }
  if (fb->len < W * H) { esp_camera_fb_return(fb); return; }

  long somaX = 0, qtd = 0, somaLuz = 0;
  memset(mascara, 0, sizeof(mascara));
  for (int y = 0; y < H; y += 2) {
    for (int x = 0; x < W; x += 2) {
      int i = y * W + x;
      int g = (y / 2) * GW + (x / 2);
      somaLuz += fb->buf[i];
      if (temAnterior && abs(fb->buf[i] - anterior[i]) > LIMIAR_PIXEL) {
        somaX += x; qtd++;
        mascara[g >> 3] |= 1 << (g & 7);
      }
    }
  }
  memcpy(anterior, fb->buf, W * H);
  temAnterior = true;

  int16_t xs = (qtd > MIN_PIXELS) ? (int16_t)(somaX / qtd) : -1;
  uint16_t q = (uint16_t)qtd;
  uint8_t luz = (uint8_t)(somaLuz / (GW * GH));
  uint16_t w = W, h = H;

  for (int i = 0; i < W * H / 2; i++) {
    img4[i] = (fb->buf[2 * i] >> 4) | (fb->buf[2 * i + 1] & 0xF0);
  }
  esp_camera_fb_return(fb);

  // Envio leva ~110 ms a 921600: ocupa o lugar do delay(60) do etapa4_camera
  Serial.write((const uint8_t *)"OLH4", 4);
  Serial.write((const uint8_t *)&w, 2);
  Serial.write((const uint8_t *)&h, 2);
  Serial.write((const uint8_t *)&xs, 2);
  Serial.write((const uint8_t *)&q, 2);
  Serial.write(&luz, 1);
  Serial.write(img4, sizeof(img4));
  Serial.write(mascara, sizeof(mascara));
}
