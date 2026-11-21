// Etapa 4 - ESP32-CAM (AI-Thinker): detecta movimento e envia "X:nnn" pela serial.
// X vai de 0 (esquerda da imagem) a 159 (direita).
// Linhas que comecam com "#" sao so diagnostico; o XIAO ignora tudo que nao for "X:".
#include "esp_camera.h"

#define W 160
#define H 120
#define LIMIAR_PIXEL 25   // diferenca de brilho para um pixel contar como "mudou"
#define MIN_PIXELS   40   // pixels mudados (de 4800 amostrados) para valer como movimento

static uint8_t anterior[W * H];
static bool temAnterior = false;
static unsigned long ultimoDiag = 0;

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("# etapa4_camera iniciando");

  camera_config_t c = {};
  c.ledc_channel = LEDC_CHANNEL_0;
  c.ledc_timer   = LEDC_TIMER_0;
  c.pin_d0 = 5;  c.pin_d1 = 18; c.pin_d2 = 19; c.pin_d3 = 21;
  c.pin_d4 = 36; c.pin_d5 = 39; c.pin_d6 = 34; c.pin_d7 = 35;
  c.pin_xclk = 0;  c.pin_pclk = 22; c.pin_vsync = 25; c.pin_href = 23;
  c.pin_sccb_sda = 26; c.pin_sccb_scl = 27;
  c.pin_pwdn = 32; c.pin_reset = -1;
  c.xclk_freq_hz = 20000000;
  c.pixel_format = PIXFORMAT_GRAYSCALE;   // sem JPEG: mais simples de comparar
  c.frame_size   = FRAMESIZE_QQVGA;       // 160 x 120 basta
  c.fb_count     = 2;
  c.fb_location  = CAMERA_FB_IN_PSRAM;
  c.grab_mode    = CAMERA_GRAB_LATEST;    // sempre o quadro mais novo

  esp_err_t err = esp_camera_init(&c);
  if (err != ESP_OK) {
    while (true) {
      Serial.printf("# ERRO camera 0x%x\n", err);
      delay(2000);
    }
  }
  Serial.println("# camera OK");
}

void loop() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) { Serial.println("# sem quadro"); delay(100); return; }
  if (fb->len < W * H) { esp_camera_fb_return(fb); return; }

  long somaX = 0, qtd = 0, somaLuz = 0;
  for (int y = 0; y < H; y += 2) {
    for (int x = 0; x < W; x += 2) {
      int i = y * W + x;
      somaLuz += fb->buf[i];
      if (temAnterior && abs(fb->buf[i] - anterior[i]) > LIMIAR_PIXEL) { somaX += x; qtd++; }
    }
  }
  memcpy(anterior, fb->buf, W * H);
  temAnterior = true;
  esp_camera_fb_return(fb);

  if (qtd > MIN_PIXELS) {                  // movimento suficiente
    Serial.printf("X:%ld\n", somaX / qtd); // 0 a 159
  }

  if (millis() - ultimoDiag > 2000) {      // diagnostico: brilho medio 0-255 e pixels mudados
    ultimoDiag = millis();
    Serial.printf("# luz=%ld mudou=%ld\n", somaLuz / ((W / 2) * (H / 2)), qtd);
  }
  delay(60);
}
