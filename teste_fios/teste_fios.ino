// Teste de fiação: lê cada pino de sinal com pull-down e pull-up internos.
// Pino que lê ALTO mesmo com pull-down = há um pull-up externo do outro lado (fio chegando no módulo).
const int pinos[] = {19, 18, 1, 2, 21, 0};
const char *nomes[] = {"D8  SCL", "D10 SDA", "D1  CS ", "D2  DC ", "D3  RES", "D0  BLK"};

void setup() {
  Serial.begin(115200);
  delay(1500);
}

void loop() {
  Serial.println("\npino     pull-down  pull-up   tensao(mV, sem pull)");
  for (int i = 0; i < 6; i++) {
    pinMode(pinos[i], INPUT_PULLDOWN); delay(20);
    int baixo = digitalRead(pinos[i]);
    pinMode(pinos[i], INPUT_PULLUP); delay(20);
    int alto = digitalRead(pinos[i]);
    pinMode(pinos[i], INPUT); delay(20);
    int mv = (pinos[i] <= 6) ? analogReadMilliVolts(pinos[i]) : -1;
    Serial.printf("%s  %-9s  %-8s  %d\n", nomes[i], baixo ? "ALTO" : "baixo", alto ? "ALTO" : "baixo", mv);
  }
  delay(3000);
}
