#define RED_LED_PIN 12
#define GREEN_LED_PIN 14

#define BUTTON_FASE_PIN 32
#define BUTTON_START_PIN 33

void setup() {
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BUTTON_FASE_PIN, INPUT);
  pinMode(BUTTON_START_PIN, INPUT);
  Serial.begin(115200);
}

void loop() {
  
  if (digitalRead(BUTTON_FASE_PIN)) {
    digitalWrite(GREEN_LED_PIN, HIGH);
    Serial.println("verde on");
    while (digitalRead(BUTTON_FASE_PIN)) {}
  }
  else {
    digitalWrite(GREEN_LED_PIN, LOW);
    Serial.println("verde off");
  }

  if (digitalRead(BUTTON_START_PIN)) {
    digitalWrite(RED_LED_PIN, HIGH);
    Serial.println("vermelho on");
    while (digitalRead(BUTTON_START_PIN)) {}
  }
  else {
    digitalWrite(RED_LED_PIN, LOW);
    Serial.println("vermelho off");
  }
}
