// Teste da montagem na protoboard (pinagem de 20/11): le D0..D6 com pull-down e pull-up internos.
// A tela tem pull-up no SDA: com a montagem certa, so o D5 (SDA) le ALTO com pull-down.
// Se for o D4 ou o D6, as fileiras do XIAO e da tela estao deslocadas uma da outra.
const int pinos[] = {16, 23, 22, 21, 2, 1, 0};
const char *nomes[] = {"D6 SCL", "D5 SDA", "D4 RST", "D3 DC ", "D2 CS ", "D1 BL ", "D0 SDO"};

void setup() {
  Serial.begin(115200);
  Serial.setTxTimeoutMs(0);
  delay(1500);
}

void loop() {
  Serial.println("\npino    pull-down  pull-up   tensao(mV, sem pull)");
  for (int i = 0; i < 7; i++) {
    pinMode(pinos[i], INPUT_PULLDOWN); delay(20);
    int baixo = digitalRead(pinos[i]);
    pinMode(pinos[i], INPUT_PULLUP); delay(20);
    int alto = digitalRead(pinos[i]);
    pinMode(pinos[i], INPUT); delay(20);
    int mv = (pinos[i] <= 6) ? analogReadMilliVolts(pinos[i]) : -1;
    Serial.printf("%s  %-9s  %-8s  %d\n", nomes[i], baixo ? "ALTO" : "baixo", alto ? "ALTO" : "baixo", mv);
  }
  delay(2000);
}
