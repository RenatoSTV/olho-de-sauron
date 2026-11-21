// Teste de continuidade dos jumpers usando o pull-up que existe no pino SDA da tela.
// Coloque a ponta (lado da tela) de um jumper no pino SDA da tela:
// se o fio estiver bom, o pino do XIAO lê ALTO e a luz de fundo (D0) ACENDE.
const int pinos[] = {19, 18, 1, 2, 21};
const char *nomes[] = {"D8 (fio do SCL)", "D10 (fio do SDA)", "D1 (fio do CS)", "D2 (fio do DC)", "D3 (fio do RST)"};

void setup() {
  Serial.begin(115200);
  pinMode(0, OUTPUT);
  for (int p : pinos) pinMode(p, INPUT_PULLDOWN);
}

void loop() {
  bool algum = false;
  String s = "";
  for (int i = 0; i < 5; i++) {
    if (digitalRead(pinos[i])) { algum = true; s += String(nomes[i]) + " LIGADO  "; }
  }
  digitalWrite(0, algum ? HIGH : LOW);
  Serial.println(algum ? s : "nenhum fio no SDA");
  delay(300);
}
