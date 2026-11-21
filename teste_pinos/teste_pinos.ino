// Autoteste: cada pino de sinal é acionado em BAIXO e ALTO e o nível real do pad é lido de volta.
#include "driver/gpio.h"
const int pinos[] = {19, 18, 1, 2, 21};
const char *nomes[] = {"D8  SCL", "D10 SDA", "D1  CS ", "D2  DC ", "D3  RES"};

void setup() { Serial.begin(115200); delay(1500); }

void loop() {
  Serial.println("\npino     manda 0 -> le   manda 1 -> le");
  for (int i = 0; i < 5; i++) {
    gpio_num_t p = (gpio_num_t)pinos[i];
    gpio_reset_pin(p);
    gpio_set_direction(p, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_level(p, 0); delayMicroseconds(200); int a = gpio_get_level(p);
    gpio_set_level(p, 1); delayMicroseconds(200); int b = gpio_get_level(p);
    Serial.printf("%s     %d              %d   %s\n", nomes[i], a, b, (a == 0 && b == 1) ? "ok" : "<<< PROBLEMA");
  }
  // SDO da tela no D9: com pull-up e pull-down, sem nenhum comando
  pinMode(20, INPUT_PULLUP);   delay(5); int up = digitalRead(20);
  pinMode(20, INPUT_PULLDOWN); delay(5); int down = digitalRead(20);
  Serial.printf("D9 (SDO): pull-up -> %d, pull-down -> %d  (1/0 = SDO solto ou em alta impedância)\n", up, down);
  delay(3000);
}
