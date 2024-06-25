#include <Wire.h>
#include "Grove_LED_Matrix_Driver_HT16K33.h"
#include <ArduinoBLE.h>

#include <Arduino_FreeRTOS.h>

#include <LiquidCrystal.h>

//JOYSTICK COLOCADO COM PINS PARA A SUA ESQUERDA
#define STICK_VRx A0
#define STICK_VRy A1

#define RED_LED_PIN 10
#define GREEN_LED_PIN 11

#define TIMER 16

#define BUTTON_FASE_PIN 4
#define BUTTON_START_PIN 5

#define STICK_THRESHOLD 300

#define addr0 0x70
#define addr1 0x71
#define addr2 0x72
#define addr3 0x73

#define ADXL345 0x53  // The ADXL345 sensor I2C address

const int rs = 2,
          en = 3,
          d4 = 6,
          d5 = 7,
          d6 = 8,
          d7 = 9;

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

TaskHandle_t Botao, Player, BTT, Time, Cobra;

struct Coordenadas {
  int x, y;
};
struct Fase {
  bool fase[8][8];
};

struct Mapa {
  Fase fases[4];
  Coordenadas inicio, fim, chave;
};
struct Database {
  Mapa mapas[5];
};

Mapa mapaAtual;
int mapaAtualInt;
//FUNCTIONS
int IntrepretarJoystick();
int MovePlayer(int direction);

//VARIABLES
Coordenadas antigoJogadorCoordenadas;
Coordenadas jogadorCoordenadas;
bool playerTemChave = false;

bool modePicker = false;
bool mapMode = false;
bool chaveDesenhada = true;

bool jogoON = false;
bool conecta = false;
bool novojogo = true;
int modofase = 0;

int tempoAgora;
int tempoComparacao;
int timer = TIMER;

bool mapaCompleto[16][16];

Matrix_8x8 matrix0;
Matrix_8x8 matrix1;
Matrix_8x8 matrix2;
Matrix_8x8 matrix3;

int ultimaInfoEnviada;

int gameSelected; 
bool gameSelecteddone = false;

BLECharacteristic MapCharacteristic;
bool BTT_conected = false;

byte customChar1[] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};

byte customChar2[] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};

// COBRA
#define QUEUESIZE 255

Coordenadas frutaCoordenadas;

int Rear = -1;
int Front = -1;
Coordenadas queue[QUEUESIZE];

void setup() {

  lcd.begin(16, 2);
  lcd.createChar(0, customChar2);
  lcd.createChar(1, customChar1);
  lcd.clear();
  Wire.begin();

  Wire.beginTransmission(ADXL345);  // Start communicating with the device
  Wire.write(0x2D);                 // Access/ talk to POWER_CTL Register - 0x2D
  // Enable measurement
  Wire.write(8);  // (8dec -> 0000 1000 binary) Bit D3 High for measuring enable
  Wire.endTransmission();

  matrix0.init(addr0);
  matrix0.setBrightness(0);
  matrix0.setBlinkRate(BLINK_OFF);
  matrix0.setDisplayOrientation(3);

  matrix1.init(addr1);
  matrix1.setBrightness(0);
  matrix1.setBlinkRate(BLINK_OFF);
  matrix1.setDisplayOrientation(3);

  matrix2.init(addr2);
  matrix2.setBrightness(0);
  matrix2.setBlinkRate(BLINK_OFF);
  matrix2.setDisplayOrientation(3);

  matrix3.init(addr3);
  matrix3.setBrightness(0);
  matrix3.setBlinkRate(BLINK_OFF);
  matrix3.setDisplayOrientation(3);

  ClearMatrixTotal();

  Serial.begin(9600);

  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);

  pinMode(STICK_VRx, INPUT);
  pinMode(STICK_VRy, INPUT);
  pinMode(BUTTON_FASE_PIN, INPUT);
  pinMode(BUTTON_START_PIN, INPUT);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Seleciona");
  lcd.setCursor(0, 2);
  lcd.print("Jogo:");

  while(!gameSelecteddone){
    
    if(BUTTON_FASE_PIN && gameSelected !=1){
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Jogo:");
      lcd.setCursor(0, 2);
      lcd.print("Labirinto");
      gameSelected = 1;
      while (digitalRead(BUTTON_FASE_PIN)) {}
    }
    if(BUTTON_FASE_PIN && gameSelected !=2){
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Jogo:");
      lcd.setCursor(0, 2);
      lcd.print("Cobra");
      gameSelected = 2;
      while (digitalRead(BUTTON_FASE_PIN)) {}
    }
    if(BUTTON_START_PIN){
      gameSelecteddone = true;
      while (digitalRead(BUTTON_START_PIN)) {}
    }
  }

  switch(gameSelected){
    case 1:

      xTaskCreate(
      TaskBotao,
      static_cast<const char *>("Buttons"),
      512,     // usStackDepth in words 
      nullptr, // pvParameters 
      1,       // uxPriority 
      &Botao   // pxCreatedTask 
      );

      xTaskCreate(
        TaskPlayer,
        static_cast<const char *>("Players"),
        512,     // usStackDepth in words 
        nullptr, // pvParameters 
        1,       // uxPriority 
        &Player  // pxCreatedTask 
      );
      xTaskCreate(
        TaskBTT,
        static_cast<const char *>("BTT"),
        512,      // usStackDepth in words
        nullptr,  // pvParameters
        1,        // uxPriority
        &BTT      // pxCreatedTask
      );
      
      lcd.setCursor(0, 0);
      lcd.print(" Clica no botao");
      lcd.setCursor(0, 2);
      lcd.print("  para comecar");

      break;
    case 2:
        /*
      xTaskCreate(
      TaskUpdateJogoCobra,
      static_cast<const char *>("Cobration"),
      512,      // usStackDepth in words
      nullptr,  // pvParameters
      1,        // uxPriority
      &Cobra    // pxCreatedTask
      );
      */
      break;
  }
  vTaskStartScheduler();
}

void TaskPlayer(void *pvParameters) {
  while (1) {
    if (BTT_conected && jogoON && !mapMode) {
      tempoAgora = millis();
      if (novojogo) {
        MostraPlayer(GetFaseAtual(jogadorCoordenadas));
        novojogo = false;
        tempoComparacao = tempoAgora;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Timer: ");
        lcd.setCursor(0, 2);
        lcdPrintBarra(16);
      }
      if ((tempoAgora - tempoComparacao) > 10000) {
        timer--;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Timer: ");
        lcd.setCursor(0, 2);
        lcdPrintBarra(timer);

        if (timer == 1) {
          lcd.write(byte(0));
        }
        if (timer == 0) {
          jogoON = false;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("PERDESTE :(");
          vTaskDelay(5000);
          NVIC_SystemReset();
        }
        tempoComparacao = tempoAgora;
      }
      int val = TrataFisicas();
      if (val == 0) {
        MostraPlayer(GetFaseAtual(jogadorCoordenadas));
      }
    }
    vTaskDelay(1);
  }
}

void TaskBotao(void *pvParameters) {
  while (1) {
    if (digitalRead(BUTTON_FASE_PIN) && !jogoON) {
      if (!modePicker) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Selecionar modo");
        modePicker = true;
        while (digitalRead(BUTTON_FASE_PIN)) {
        }
      } else {
        modofase++;
        if (modofase == 4) {
          modofase = 0;
        }
        lcd.clear();
        switch (modofase) {
          case 0:
            lcd.setCursor(0, 0);
            lcd.print("Modo Normal");
            lcd.setCursor(0, 2);
            lcd.print("Multiplayer");
            break;
          case 1:
            lcd.setCursor(0, 0);
            lcd.print("Modo Fases");
            lcd.setCursor(0, 2);
            lcd.print("Multiplayer");
            break;
          case 2:
            lcd.setCursor(0, 0);
            lcd.print("Modo Normal");
            lcd.setCursor(0, 2);
            lcd.print("Singleplayer");
            break;
          case 3:
            lcd.setCursor(0, 0);
            lcd.print("Modo Fases");
            lcd.setCursor(0, 2);
            lcd.print("Singleplayer");
            break;
        }
        while (digitalRead(BUTTON_FASE_PIN)) {
        }
      }
    }

    if (digitalRead(BUTTON_START_PIN) && !jogoON && modePicker) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" Clica no botao");
      lcd.setCursor(0, 2);
      lcd.print("  para comecar");
      modePicker = false;
      while (digitalRead(BUTTON_START_PIN)) {
      }
    }

    if (digitalRead(BUTTON_START_PIN) && !jogoON && !modePicker) {
      ///////////////////////////////////////////////////////////////////////////
      AtribuiMapaAleatorio();
      //AtribuiMapa5(); // Para Teste
      ///////////////////////////////////////////////////////////////////////////
      jogadorCoordenadas = mapaAtual.inicio;
      conecta = true;
      lcd.clear();
      lcd.setCursor(0, 0);
      //lcd.print("Starting in 5");
      lcd.print("Comeca em 5");
      lcd.setCursor(0, 2);
      lcd.print("Prepara-te!");
      vTaskDelay(1000);
      lcd.setCursor(0, 0);
      lcd.print("Comeca em 4");
      //lcd.print("Starting in 4");
      vTaskDelay(1000);
      lcd.setCursor(0, 0);
      lcd.print("Comeca em 3");
      //lcd.print("Starting in 3");
      vTaskDelay(1000);
      lcd.setCursor(0, 0);
      lcd.print("Comeca em 2");
      //lcd.print("Starting in 2");
      vTaskDelay(1000);
      lcd.setCursor(0, 0);
      lcd.print("Comeca em 1");
      //lcd.print("Starting in 1");
      vTaskDelay(1000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("COMECA!!!!");
      vTaskDelay(1000);
      lcd.clear();
      jogoON = true;
      //Mostrar algo no LCD
      //Comecar countdown e comecar timer
      while (digitalRead(BUTTON_START_PIN)) {
      }
    }

    if (digitalRead(BUTTON_START_PIN) && jogoON && (modofase > 1) && !modePicker) {
      ///////////////////////////////////////////////////////////////////////////
      mapMode = true;
      ClearMatrixTotal();
      if (modofase == 2) {
        DesenhaMapaInteiro();
      } else {
        switch (GetFaseAtual(jogadorCoordenadas)) {
          case 0:
            DesenhaMapa(0, matrix0);
            break;
          case 1:
            DesenhaMapa(1, matrix1);
            break;
          case 2:
            DesenhaMapa(2, matrix2);
            break;
          case 3:
            DesenhaMapa(3, matrix3);
            break;
        }
      }

      while (digitalRead(BUTTON_START_PIN)) {
      }
      delay(1000);
      ClearMatrixTotal();
      MostraPlayer(GetFaseAtual(jogadorCoordenadas));
      mapMode = false;
    }

    vTaskDelay(1);
  }
}

void TaskBTT(void *pvParameters) {
  BLE.begin();
  while (1) {
    if (conecta) {
      // start scanning for peripherals
      BLE.scanForUuid("19B10010-E8F2-537E-4F6C-D104768A1214");
      BLEDevice peripheral = BLE.available();
      Serial.print("Tentar ");
      if (peripheral) {
        // discovered a peripheral, print out address, local name, and advertised service
        Serial.print("Found ");
        Serial.print(peripheral.address());
        Serial.print(" '");
        Serial.print(peripheral.localName());
        Serial.print("' ");
        Serial.print(peripheral.advertisedServiceUuid());
        Serial.println();

        if (peripheral.localName() == "Mapa") {
          BLE.stopScan();
        }

        controlMap(peripheral);
      }
    }
    vTaskDelay(1);
  }
}

void loop() {}

void controlMap(BLEDevice peripheral) {

  if (peripheral.connect()) {
    Serial.println("Connected");
  } else {
    Serial.println("Failed to connect!");
    return;
  }

  // discover peripheral attributes
  Serial.println("Discovering attributes ...");
  if (peripheral.discoverAttributes()) {
    Serial.println("Attributes discovered");
  } else {
    Serial.println("Attribute discovery failed!");
    peripheral.disconnect();
    return;
  }

  // retrieve the LED characteristic
  MapCharacteristic = peripheral.characteristic("19B10011-E8F2-537E-4F6C-D104768A1214");

  if (!MapCharacteristic) {
    Serial.println("Peripheral does not have Map characteristic!");
    peripheral.disconnect();
    return;
  } else if (!MapCharacteristic.canWrite()) {
    Serial.println("Peripheral does not have a writable Map characteristic!");
    peripheral.disconnect();
    return;
  }

  BTT_conected = true;
  if (modofase < 2) {
    MapCharacteristic.writeValue((byte)mapaAtualInt);
    switch (modofase) {
      case 0:
        MapCharacteristic.writeValue((byte)10);
        break;
      case 1:
        MapCharacteristic.writeValue((byte)9);
        break;
    }
  }


  while (peripheral.connected()) {
    vTaskDelay(1);
  }
  BTT_conected = false;

  //MapCharacteristic.writeValue((byte)0x01);
  //Funcao que manda

  Serial.println("Peripheral disconnected");
}

void ClearMatrix(Matrix_8x8 matrix) {
  matrix.clear();
  matrix.display();
}

int TrataFisicas() {
  float X_out, Y_out, Z_out;  // Outputs

  Wire.beginTransmission(ADXL345);
  Wire.write(0x32);  // Start with register 0x32 (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(ADXL345, 6, true);  // Read 6 registers total, each axis value is stored in 2 registers

  X_out = (Wire.read() | Wire.read() << 8);  // X-axis value
  X_out = X_out / 256;                       //For a range of +-2g, we need to divide the raw values by 256, according to the datasheet
  Y_out = (Wire.read() | Wire.read() << 8);  // Y-axis value
  Y_out = Y_out / 256;
  Z_out = (Wire.read() | Wire.read() << 8);  // Z-axis value
  Z_out = Z_out / 256;

  if (X_out < 255.6 && X_out > 2) {
    return MovePlayer(0);  //DIREITA
  } else if (X_out > 0.4 && X_out < 2) {
    return MovePlayer(1);  //ESQUERDA
  }
  if (Y_out < 255.6 && Y_out > 2) {
    return MovePlayer(2);  //CIMA
  } else if (Y_out > 0.4 && Y_out < 2) {
    return MovePlayer(3);  //BAIXO
  }
  return -1;
}

int GetFaseAtual(Coordenadas coord) {
  int fase = -1;
  if (coord.x < 8 && coord.y < 8) {
    fase = 0;
  } else if (coord.x < 8 && coord.y >= 8) {
    fase = 3;
  } else if (coord.x >= 8 && coord.y >= 8) {
    fase = 2;
  } else if (coord.x >= 8 && coord.y < 8) {
    fase = 1;
  }
  return fase;
}

void MostraPlayer(int faseAtual) {
  //Chave
  if (!playerTemChave) {
    int fase = GetFaseAtual(mapaAtual.chave);

    int chaveX = mapaAtual.chave.x;
    int chaveY = mapaAtual.chave.y;

    if (chaveX >= 8) {
      chaveX -= 8;
    }

    if (chaveY >= 8) {
      chaveY -= 8;
    }

    switch (fase) {
      case 0:
        matrix0.writePixel(chaveX, chaveY);
        matrix0.writePixel(chaveX + 1, chaveY);
        matrix0.writePixel(chaveX - 1, chaveY);
        matrix0.writePixel(chaveX, chaveY + 1);
        matrix0.writePixel(chaveX, chaveY - 1);
        matrix0.display();
        break;
      case 1:
        matrix1.writePixel(chaveX, chaveY);
        matrix1.writePixel(chaveX + 1, chaveY);
        matrix1.writePixel(chaveX - 1, chaveY);
        matrix1.writePixel(chaveX, chaveY + 1);
        matrix1.writePixel(chaveX, chaveY - 1);
        matrix1.display();
        break;
      case 2:
        matrix2.writePixel(chaveX, chaveY);
        matrix2.writePixel(chaveX + 1, chaveY);
        matrix2.writePixel(chaveX - 1, chaveY);
        matrix2.writePixel(chaveX, chaveY + 1);
        matrix2.writePixel(chaveX, chaveY - 1);
        matrix2.display();
        break;
      case 3:
        matrix3.writePixel(chaveX, chaveY);
        matrix3.writePixel(chaveX + 1, chaveY);
        matrix3.writePixel(chaveX - 1, chaveY);
        matrix3.writePixel(chaveX, chaveY + 1);
        matrix3.writePixel(chaveX, chaveY - 1);
        matrix3.display();
        break;
    }
  } else {
    if (chaveDesenhada) {
      int fase = GetFaseAtual(mapaAtual.chave);

      int chaveX = mapaAtual.chave.x;
      int chaveY = mapaAtual.chave.y;

      if (chaveX >= 8) {
        chaveX -= 8;
      }

      if (chaveY >= 8) {
        chaveY -= 8;
      }

      switch (fase) {
        case 0:
          matrix0.writePixel(chaveX, chaveY, false);
          matrix0.writePixel(chaveX + 1, chaveY, false);
          matrix0.writePixel(chaveX - 1, chaveY, false);
          matrix0.writePixel(chaveX, chaveY + 1, false);
          matrix0.writePixel(chaveX, chaveY - 1, false);
          matrix0.display();
          break;
        case 1:
          matrix1.writePixel(chaveX, chaveY, false);
          matrix1.writePixel(chaveX + 1, chaveY, false);
          matrix1.writePixel(chaveX - 1, chaveY, false);
          matrix1.writePixel(chaveX, chaveY + 1, false);
          matrix1.writePixel(chaveX, chaveY - 1, false);
          matrix1.display();
          break;
        case 2:
          matrix2.writePixel(chaveX, chaveY, false);
          matrix2.writePixel(chaveX + 1, chaveY, false);
          matrix2.writePixel(chaveX - 1, chaveY, false);
          matrix2.writePixel(chaveX, chaveY + 1, false);
          matrix2.writePixel(chaveX, chaveY - 1, false);
          matrix2.display();
          break;
        case 3:
          matrix3.writePixel(chaveX, chaveY, false);
          matrix3.writePixel(chaveX + 1, chaveY, false);
          matrix3.writePixel(chaveX - 1, chaveY, false);
          matrix3.writePixel(chaveX, chaveY + 1, false);
          matrix3.writePixel(chaveX, chaveY - 1, false);
          matrix3.display();
          break;
      }
    }
    //saida

    int fase = GetFaseAtual(mapaAtual.fim);
    int saidaX = mapaAtual.fim.x;
    int saidaY = mapaAtual.fim.y;
    if (saidaX >= 8) {
      saidaX -= 8;
    }

    if (saidaY >= 8) {
      saidaY -= 8;
    }
    switch (fase) {
      case 0:
        matrix0.writePixel(saidaX, saidaY);
        matrix0.writePixel(saidaX + 1, saidaY + 1);
        matrix0.writePixel(saidaX - 1, saidaY + 1);
        matrix0.writePixel(saidaX - 1, saidaY - 1);
        matrix0.writePixel(saidaX + 1, saidaY - 1);
        matrix0.display();
        break;
      case 1:
        matrix1.writePixel(saidaX, saidaY);
        matrix1.writePixel(saidaX + 1, saidaY + 1);
        matrix1.writePixel(saidaX - 1, saidaY + 1);
        matrix1.writePixel(saidaX - 1, saidaY - 1);
        matrix1.writePixel(saidaX + 1, saidaY - 1);
        matrix1.display();
        break;
      case 2:
        matrix2.writePixel(saidaX, saidaY);
        matrix2.writePixel(saidaX + 1, saidaY + 1);
        matrix2.writePixel(saidaX - 1, saidaY + 1);
        matrix2.writePixel(saidaX - 1, saidaY - 1);
        matrix2.writePixel(saidaX + 1, saidaY - 1);
        matrix2.display();
        break;
      case 3:
        matrix3.writePixel(saidaX, saidaY);
        matrix3.writePixel(saidaX + 1, saidaY + 1);
        matrix3.writePixel(saidaX - 1, saidaY + 1);
        matrix3.writePixel(saidaX - 1, saidaY - 1);
        matrix3.writePixel(saidaX + 1, saidaY - 1);
        matrix3.display();
        break;
    }
  }
  int x = jogadorCoordenadas.x;
  int y = jogadorCoordenadas.y;

  if (x >= 8) {
    x -= 8;
  }

  if (y >= 8) {
    y -= 8;
  }
  switch (faseAtual) {
    case 0:
      matrix0.writePixel(x, y, true);
      matrix0.display();
      if (ultimaInfoEnviada != 0 && (modofase < 2)) {
        MapCharacteristic.writeValue((byte)0);
        ultimaInfoEnviada = 0;
      }
      break;
    case 1:
      matrix1.writePixel(x, y, true);
      matrix1.display();
      if (ultimaInfoEnviada != 1 && (modofase < 2)) {
        MapCharacteristic.writeValue((byte)1);
        ultimaInfoEnviada = 1;
      }
      break;
    case 2:
      matrix2.writePixel(x, y, true);
      matrix2.display();
      if (ultimaInfoEnviada != 2 && (modofase < 2)) {
        MapCharacteristic.writeValue((byte)2);
        ultimaInfoEnviada = 2;
      }
      break;
    case 3:
      matrix3.writePixel(x, y, true);
      matrix3.display();
      if (ultimaInfoEnviada != 3 && (modofase < 2)) {
        MapCharacteristic.writeValue((byte)3);
        ultimaInfoEnviada = 3;
      }
      break;
  }

  if (antigoJogadorCoordenadas.x == jogadorCoordenadas.x && antigoJogadorCoordenadas.y == jogadorCoordenadas.y)
    return;

  x = antigoJogadorCoordenadas.x;
  y = antigoJogadorCoordenadas.y;
  if (x >= 8) {
    x -= 8;
  }

  if (y >= 8) {
    y -= 8;
  }
  int faseAnterior = GetFaseAtual(antigoJogadorCoordenadas);
  switch (faseAnterior) {
    case 0:
      matrix0.writePixel(x, y, false);
      matrix0.display();
      break;
    case 1:
      matrix1.writePixel(x, y, false);
      matrix1.display();
      break;
    case 2:
      matrix2.writePixel(x, y, false);
      matrix2.display();
      break;
    case 3:
      matrix3.writePixel(x, y, false);
      matrix3.display();
      break;
  }
}

int MovePlayer(int direction) {
  bool posicaoFutura;
  antigoJogadorCoordenadas.x = jogadorCoordenadas.x;
  antigoJogadorCoordenadas.y = jogadorCoordenadas.y;
  switch (direction) {
    case 0:
      posicaoFutura = mapaCompleto[jogadorCoordenadas.x][jogadorCoordenadas.y + 1];
      if (jogadorCoordenadas.y >= 15 || posicaoFutura) {
        FlashLedVermelho();
        return -1;
      }
      jogadorCoordenadas.y++;
      break;
    case 1:
      posicaoFutura = mapaCompleto[jogadorCoordenadas.x][jogadorCoordenadas.y - 1];
      if (jogadorCoordenadas.y <= 0 || posicaoFutura) {
        FlashLedVermelho();
        return -1;
      }
      jogadorCoordenadas.y--;
      break;
    case 2:
      posicaoFutura = mapaCompleto[jogadorCoordenadas.x - 1][jogadorCoordenadas.y];
      if (jogadorCoordenadas.x <= 0 || posicaoFutura) {
        FlashLedVermelho();
        return -1;
      }
      jogadorCoordenadas.x--;
      break;
    case 3:
      posicaoFutura = mapaCompleto[jogadorCoordenadas.x + 1][jogadorCoordenadas.y];
      if (jogadorCoordenadas.x >= 15 || posicaoFutura) {
        FlashLedVermelho();
        return -1;
      }
      jogadorCoordenadas.x++;
      break;
  }
  if (jogadorCoordenadas.x == mapaAtual.chave.x && jogadorCoordenadas.y == mapaAtual.chave.y) {
    playerTemChave = true;
    digitalWrite(GREEN_LED_PIN, HIGH);
  }
  if (jogadorCoordenadas.x == mapaAtual.fim.x && jogadorCoordenadas.y == mapaAtual.fim.y && playerTemChave) {
    //GANHOU
    jogoON = false;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("GANHASTE!");
    if (modofase < 2) {
      MapCharacteristic.writeValue((byte)11);
    }
    DesenhaMapaInteiro();
    delay(1000);
    ClearMatrixTotal();
    delay(1000);
    DesenhaMapaInteiro();
    delay(1000);
    ClearMatrixTotal();
    //resetFunc();
    //digitalWrite(RESET, LOW);
    NVIC_SystemReset();
  }
  return 0;
}

void FlashLedVermelho() {
  for (int fadeValue = 255; fadeValue >= 0; fadeValue -= 5) {
    analogWrite(RED_LED_PIN, fadeValue);
    delay(3);
  }
}

void FlashLedVerde() {
  for (int fadeValue = 255; fadeValue >= 0; fadeValue -= 5) {
    analogWrite(GREEN_LED_PIN, fadeValue);
    delay(3);
  }
}

void AtribuiMapaAleatorio() {
  AtribuiMapa2();
  /*
  randomSeed(millis());
  switch (random(1, 6)) {
    case 1:
      AtribuiMapa1();
      break;
    case 2:
      AtribuiMapa2();
      break;
    case 3:
      AtribuiMapa3();
      break;
    case 4:
      AtribuiMapa4();
      break;
    case 5:
      AtribuiMapa5();
      break;
  }
  */
}
/*
void AtribuiMapa1() {
  mapaAtualInt = 4;
  bool mapa1fase1[8][8] = {
    { 0, 1, 0, 0, 0, 1, 1, 1 },
    { 0, 1, 1, 1, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 1, 1, 0 },
    { 1, 0, 1, 0, 0, 0, 0, 0 },
    { 0, 0, 1, 1, 1, 0, 1, 0 },
    { 1, 0, 1, 0, 0, 0, 1, 1 },
    { 1, 0, 1, 0, 1, 0, 0, 0 },
    { 1, 0, 1, 0, 1, 1, 1, 1 }
  };

  Fase fase1;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      fase1.fase[i][j] = mapa1fase1[i][j];
    }
  }

  bool mapa1fase2[8][8] = {
    { 1, 0, 1, 0, 0, 0, 0, 1 },
    { 1, 0, 1, 1, 1, 1, 0, 1 },
    { 1, 0, 0, 0, 0, 1, 1, 1 },
    { 1, 0, 0, 1, 0, 1, 0, 0 },
    { 1, 0, 1, 1, 0, 0, 0, 0 },
    { 0, 0, 0, 1, 1, 1, 1, 1 },
    { 1, 1, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 1, 1, 1, 1, 1 }
  };

  Fase fase2;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      fase2.fase[i][j] = mapa1fase2[i][j];
    }
  }

  bool mapa1fase3[8][8] = {
    { 1, 1, 0, 0, 0, 1, 0, 0 },
    { 0, 0, 0, 1, 1, 1, 0, 1 },
    { 0, 1, 0, 0, 0, 1, 0, 1 },
    { 0, 1, 1, 1, 1, 1, 0, 1 },
    { 1, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 0, 1, 1, 0, 1, 1, 1 },
    { 0, 1, 1, 1, 0, 0, 1, 0 },
    { 0, 0, 0, 1, 1, 0, 0, 0 }
  };

  Fase fase3;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      fase3.fase[i][j] = mapa1fase3[i][j];
    }
  }

  bool mapa1fase4[8][8] = {
    { 1, 1, 1, 0, 0, 0, 1, 0 },
    { 0, 0, 1, 0, 1, 0, 1, 0 },
    { 1, 0, 1, 1, 1, 0, 0, 0 },
    { 1, 0, 0, 0, 0, 0, 1, 1 },
    { 0, 1, 1, 1, 0, 1, 0, 0 },
    { 0, 1, 0, 1, 0, 1, 1, 1 },
    { 0, 1, 0, 1, 0, 0, 0, 0 },
    { 0, 1, 0, 1, 0, 1, 1, 0 }
  };

  Fase fase4;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      fase4.fase[i][j] = mapa1fase4[i][j];
    }
  }
  Coordenadas inicioVector2;
  inicioVector2.x = 2;
  inicioVector2.y = 1;

  Coordenadas chaveVector2;
  chaveVector2.x = 9;
  chaveVector2.y = 14;

  Coordenadas fimVector2;
  fimVector2.x = 11;
  fimVector2.y = 1;

  mapaAtual.fases[0] = fase1;
  mapaAtual.fases[1] = fase2;
  mapaAtual.fases[2] = fase3;
  mapaAtual.fases[3] = fase4;

  mapaAtual.inicio = inicioVector2;
  mapaAtual.chave = chaveVector2;
  mapaAtual.fim = fimVector2;

  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      mapaCompleto[i][j] = mapa1fase1[i][j];
      mapaCompleto[i + 8][j] = mapa1fase2[i][j];
      mapaCompleto[i + 8][j + 8] = mapa1fase3[i][j];
      mapaCompleto[i][j + 8] = mapa1fase4[i][j];
    }
  }
  Serial.println("MAPA COMPLETO:");
  for (int i = 0; i < 16; i++) {
    for (int j = 0; j < 16; j++) {
      Serial.print(mapaCompleto[i][j]);
    }
    Serial.println();
  }
}
*/
void AtribuiMapa2() {
  mapaAtualInt = 5;
  bool mapa2fase1[8][8] = {
    { 0, 0, 0, 1, 0, 0, 0, 0 },
    { 1, 1, 0, 1, 1, 0, 1, 1 },
    { 0, 1, 0, 0, 1, 0, 1, 0 },
    { 0, 1, 0, 1, 1, 1, 0, 0 },
    { 0, 1, 0, 0, 0, 0, 1, 0 },
    { 0, 0, 0, 1, 1, 0, 1, 0 },
    { 0, 0, 1, 0, 0, 0, 1, 1 },
    { 0, 1, 1, 0, 1, 1, 0, 1 }
  };

  Fase fase1;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      fase1.fase[i][j] = mapa2fase1[i][j];
    }
  }

  bool mapa2fase2[8][8] = {
    { 0, 1, 0, 1, 0, 1, 0, 1 },
    { 0, 1, 0, 1, 0, 1, 0, 0 },
    { 0, 1, 0, 1, 0, 1, 0, 1 },
    { 0, 0, 0, 0, 0, 1, 1, 1 },
    { 1, 0, 1, 0, 0, 0, 0, 0 },
    { 0, 0, 1, 1, 1, 0, 0, 0 },
    { 1, 0, 0, 0, 0, 1, 1, 1 },
    { 1, 1, 0, 1, 0, 0, 0, 0 }
  };

  Fase fase2;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      fase2.fase[i][j] = mapa2fase2[i][j];
    }
  }

  bool mapa2fase3[8][8] = {
    { 0, 1, 1, 0, 0, 1, 1, 0 },
    { 0, 0, 1, 1, 0, 0, 1, 0 },
    { 1, 0, 0, 1, 1, 0, 1, 0 },
    { 0, 0, 0, 0, 0, 0, 1, 0 },
    { 0, 1, 0, 0, 1, 0, 1, 0 },
    { 0, 1, 1, 1, 1, 1, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1 }
  };

  Fase fase3;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      fase3.fase[i][j] = mapa2fase3[i][j];
    }
  }

  bool mapa2fase4[8][8] = {
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 1, 0 },
    { 1, 1, 0, 0, 0, 0, 1, 0 },
    { 1, 0, 0, 1, 1, 0, 1, 0 },
    { 1, 0, 1, 0, 1, 0, 1, 0 },
    { 0, 1, 0, 0, 0, 0, 1, 0 },
    { 0, 1, 0, 1, 0, 1, 1, 0 },
    { 0, 1, 0, 0, 1, 0, 0, 0 }
  };

  Fase fase4;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      fase4.fase[i][j] = mapa2fase4[i][j];
    }
  }
  Coordenadas inicioVector2;
  inicioVector2.x = 15;
  inicioVector2.y = 2;

  Coordenadas chaveVector2;
  chaveVector2.x = 2;
  chaveVector2.y = 3;

  Coordenadas fimVector2;
  fimVector2.x = 2;
  fimVector2.y = 13;

  mapaAtual.fases[0] = fase1;
  mapaAtual.fases[1] = fase2;
  mapaAtual.fases[2] = fase3;
  mapaAtual.fases[3] = fase4;

  mapaAtual.inicio = inicioVector2;
  mapaAtual.chave = chaveVector2;
  mapaAtual.fim = fimVector2;

  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      mapaCompleto[i][j] = mapa2fase1[i][j];
      mapaCompleto[i + 8][j] = mapa2fase2[i][j];
      mapaCompleto[i + 8][j + 8] = mapa2fase3[i][j];
      mapaCompleto[i][j + 8] = mapa2fase4[i][j];
    }
  }
}
/*
void AtribuiMapa3() {
  mapaAtualInt = 6;
  bool mapa3fase1[8][8] = {
    { 0, 0, 1, 1, 0, 1, 0, 0 },
    { 0, 1, 1, 0, 0, 0, 1, 1 },
    { 0, 1, 0, 0, 1, 0, 0, 1 },
    { 0, 0, 0, 1, 1, 0, 1, 1 },
    { 1, 1, 1, 0, 0, 0, 1, 0 },
    { 1, 0, 1, 1, 0, 1, 1, 0 },
    { 1, 0, 1, 0, 0, 1, 0, 1 },
    { 1, 0, 0, 1, 0, 0, 0, 1 }
  };

  Fase fase1;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      fase1.fase[i][j] = mapa3fase1[i][j];
    }
  }

  bool mapa3fase2[8][8] = {
    { 1, 0, 1, 1, 0, 0, 1, 0 },
    { 0, 0, 1, 0, 0, 1, 1, 1 },
    { 1, 0, 1, 0, 1, 1, 1, 1 },
    { 1, 0, 1, 0, 0, 0, 1, 0 },
    { 1, 0, 1, 0, 1, 1, 0, 1 },
    { 0, 0, 0, 0, 0, 1, 1, 0 },
    { 0, 1, 1, 1, 0, 1, 0, 0 },
    { 0, 0, 0, 1, 0, 0, 0, 1 }
  };

  Fase fase2;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      fase2.fase[i][j] = mapa3fase2[i][j];
    }
  }

  bool mapa3fase3[8][8] = {
    { 0, 1, 1, 1, 1, 1, 0, 0 },
    { 0, 0, 1, 0, 1, 0, 0, 1 },
    { 1, 1, 0, 0, 1, 0, 1, 1 },
    { 0, 0, 0, 1, 1, 0, 1, 1 },
    { 0, 1, 0, 1, 1, 0, 0, 1 },
    { 0, 1, 0, 1, 1, 1, 0, 1 },
    { 1, 1, 0, 0, 0, 0, 0, 1 },
    { 1, 1, 1, 1, 1, 1, 0, 0 }
  };

  Fase fase3;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      fase3.fase[i][j] = mapa3fase3[i][j];
    }
  }

  bool mapa3fase4[8][8] = {
    { 0, 1, 0, 0, 0, 0, 0, 0 },
    { 0, 1, 0, 1, 0, 1, 0, 0 },
    { 0, 0, 0, 1, 0, 1, 0, 1 },
    { 0, 1, 1, 1, 0, 1, 0, 1 },
    { 0, 1, 1, 0, 0, 1, 0, 1 },
    { 1, 1, 0, 0, 1, 0, 0, 0 },
    { 1, 0, 0, 1, 0, 0, 1, 1 },
    { 0, 0, 1, 0, 0, 0, 0, 1 }
  };

  Fase fase4;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      fase4.fase[i][j] = mapa3fase4[i][j];
    }
  }
  Coordenadas inicioVector2;
  inicioVector2.x = 13;
  inicioVector2.y = 2;

  Coordenadas chaveVector2;
  chaveVector2.x = 2;
  chaveVector2.y = 2;

  Coordenadas fimVector2;
  fimVector2.x = 9;
  fimVector2.y = 9;

  mapaAtual.fases[0] = fase1;
  mapaAtual.fases[1] = fase2;
  mapaAtual.fases[2] = fase3;
  mapaAtual.fases[3] = fase4;

  mapaAtual.inicio = inicioVector2;
  mapaAtual.chave = chaveVector2;
  mapaAtual.fim = fimVector2;

  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      mapaCompleto[i][j] = mapa3fase1[i][j];
      mapaCompleto[i + 8][j] = mapa3fase2[i][j];
      mapaCompleto[i + 8][j + 8] = mapa3fase3[i][j];
      mapaCompleto[i][j + 8] = mapa3fase4[i][j];
    }
  }
}

void AtribuiMapa4() {
  mapaAtualInt = 7;
  bool mapa4fase1[8][8] = {
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 1, 1, 0, 0, 1, 0, 1 },
    { 0, 1, 0, 1, 1, 0, 0, 0 },
    { 0, 1, 0, 0, 1, 1, 0, 0 },
    { 0, 1, 0, 0, 0, 0, 1, 1 },
    { 0, 0, 0, 1, 0, 0, 0, 1 },
    { 0, 1, 0, 0, 1, 1, 0, 0 },
    { 0, 0, 1, 0, 0, 0, 1, 1 }
  };

  Fase fase1;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      fase1.fase[i][j] = mapa4fase1[i][j];
    }
  }

  bool mapa4fase2[8][8] = {
    { 0, 0, 0, 1, 0, 1, 0, 1 },
    { 0, 1, 0, 0, 1, 0, 0, 0 },
    { 0, 0, 1, 0, 0, 0, 1, 0 },
    { 1, 1, 0, 0, 1, 1, 1, 1 },
    { 0, 1, 1, 0, 0, 0, 0, 1 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 1, 1, 1, 1, 1, 0, 0 },
    { 0, 0, 0, 0, 0, 1, 1, 0 }
  };

  Fase fase2;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      fase2.fase[i][j] = mapa4fase2[i][j];
    }
  }

  bool mapa4fase3[8][8] = {
    { 1, 0, 0, 1, 1, 0, 0, 0 },
    { 1, 1, 0, 0, 1, 0, 1, 0 },
    { 0, 1, 1, 0, 0, 1, 0, 0 },
    { 0, 0, 1, 1, 0, 0, 1, 1 },
    { 1, 0, 0, 1, 1, 0, 0, 1 },
    { 1, 1, 0, 0, 1, 1, 0, 0 },
    { 0, 1, 1, 0, 1, 0, 1, 0 },
    { 0, 0, 1, 0, 1, 0, 0, 0 }
  };

  Fase fase3;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      fase3.fase[i][j] = mapa4fase3[i][j];
    }
  }

  bool mapa4fase4[8][8] = {
    { 0, 1, 0, 1, 0, 0, 0, 0 },
    { 0, 0, 1, 0, 1, 0, 1, 0 },
    { 1, 0, 0, 0, 0, 0, 1, 1 },
    { 0, 1, 1, 1, 0, 0, 1, 0 },
    { 0, 0, 0, 0, 1, 0, 0, 0 },
    { 1, 0, 1, 0, 0, 1, 1, 1 },
    { 0, 1, 0, 0, 1, 0, 0, 0 },
    { 0, 0, 1, 0, 0, 0, 1, 1 }
  };

  Fase fase4;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      fase4.fase[i][j] = mapa4fase4[i][j];
    }
  }
  Coordenadas inicioVector2;
  inicioVector2.x = 15;
  inicioVector2.y = 4;

  Coordenadas chaveVector2;
  chaveVector2.x = 14;
  chaveVector2.y = 11;

  Coordenadas fimVector2;
  fimVector2.x = 14;
  fimVector2.y = 13;

  mapaAtual.fases[0] = fase1;
  mapaAtual.fases[1] = fase2;
  mapaAtual.fases[2] = fase3;
  mapaAtual.fases[3] = fase4;

  mapaAtual.inicio = inicioVector2;
  mapaAtual.chave = chaveVector2;
  mapaAtual.fim = fimVector2;

  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      mapaCompleto[i][j] = mapa4fase1[i][j];
      mapaCompleto[i + 8][j] = mapa4fase2[i][j];
      mapaCompleto[i + 8][j + 8] = mapa4fase3[i][j];
      mapaCompleto[i][j + 8] = mapa4fase4[i][j];
    }
  }
}

void AtribuiMapa5() {
  mapaAtualInt = 8;
  bool mapa5fase1[8][8] = {
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 1, 1, 1, 1, 1, 1, 1 },
    { 0, 1, 0, 0, 0, 0, 0, 0 },
    { 0, 1, 0, 1, 0, 1, 1, 1 },
    { 0, 1, 0, 1, 0, 0, 0, 0 },
    { 0, 1, 0, 1, 0, 0, 1, 1 },
    { 0, 1, 0, 1, 0, 0, 1, 0 },
    { 0, 1, 0, 1, 0, 0, 1, 0 }
  };

  Fase fase1;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      fase1.fase[i][j] = mapa5fase1[i][j];
    }
  }

  bool mapa5fase2[8][8] = {
    { 0, 1, 0, 1, 0, 0, 1, 0 },
    { 0, 1, 0, 1, 0, 0, 1, 0 },
    { 0, 1, 0, 1, 0, 0, 1, 0 },
    { 0, 1, 0, 1, 0, 0, 0, 0 },
    { 0, 1, 0, 1, 1, 1, 1, 1 },
    { 0, 1, 0, 0, 0, 0, 0, 0 },
    { 0, 1, 1, 1, 1, 1, 1, 1 },
    { 0, 0, 0, 0, 0, 0, 0, 0 }
  };

  Fase fase2;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      fase2.fase[i][j] = mapa5fase2[i][j];
    }
  }

  bool mapa5fase3[8][8] = {
    { 0, 1, 0, 0, 1, 0, 1, 0 },
    { 1, 0, 0, 0, 1, 0, 1, 0 },
    { 1, 0, 0, 0, 1, 0, 1, 0 },
    { 0, 0, 0, 0, 1, 0, 1, 0 },
    { 1, 1, 1, 1, 1, 0, 1, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 1, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 }
  };

  Fase fase3;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      fase3.fase[i][j] = mapa5fase3[i][j];
    }
  }

  bool mapa5fase4[8][8] = {
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 1, 0 },
    { 0, 0, 0, 0, 0, 0, 1, 0 },
    { 1, 1, 1, 1, 1, 0, 1, 0 },
    { 0, 0, 0, 0, 1, 0, 1, 0 },
    { 1, 1, 0, 0, 1, 0, 1, 0 },
    { 0, 1, 0, 0, 1, 0, 1, 0 },
    { 1, 1, 0, 0, 1, 0, 1, 0 }
  };

  Fase fase4;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      fase4.fase[i][j] = mapa5fase4[i][j];
    }
  }
  Coordenadas inicioVector2;
  inicioVector2.x = 1;
  inicioVector2.y = 0;

  Coordenadas chaveVector2;
  chaveVector2.x = 9;
  chaveVector2.y = 9;

  Coordenadas fimVector2;
  fimVector2.x = 13;
  fimVector2.y = 14;

  mapaAtual.fases[0] = fase1;
  mapaAtual.fases[1] = fase2;
  mapaAtual.fases[2] = fase3;
  mapaAtual.fases[3] = fase4;

  mapaAtual.inicio = inicioVector2;
  mapaAtual.chave = chaveVector2;
  mapaAtual.fim = fimVector2;

  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      mapaCompleto[i][j] = mapa5fase1[i][j];
      mapaCompleto[i + 8][j] = mapa5fase2[i][j];
      mapaCompleto[i + 8][j + 8] = mapa5fase3[i][j];
      mapaCompleto[i][j + 8] = mapa5fase4[i][j];
    }
  }
}
*/
void DesenhaMapa(int fase, Matrix_8x8 matrixParaDesenhar) {
  int faseDoMapa = fase;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      matrixParaDesenhar.writePixel(i, j, mapaAtual.fases[fase].fase[i][j]);
    }
  }
  matrixParaDesenhar.display();
}

void DesenhaMapaInteiro() {
  DesenhaMapa(0, matrix0);
  DesenhaMapa(1, matrix1);
  DesenhaMapa(2, matrix2);
  DesenhaMapa(3, matrix3);
}

void ClearMatrixTotal() {
  ClearMatrix(matrix0);
  ClearMatrix(matrix1);
  ClearMatrix(matrix2);
  ClearMatrix(matrix3);
}

void lcdPrintBarra(int caracteresAEscrever) {
  for(int a = 0; a < caracteresAEscrever; a++ )
  {
    lcd.write(byte(1));
  }
}

//*****************************************************
//
// JOGO DA COBRA
//
//*****************************************************

int randomNumber(int inclusiveMin, int inclusiveMax) {
  randomSeed(millis());
  return random(inclusiveMin, inclusiveMax + 1);
}

void enqueue(Coordenadas insertItem) {
  if (Rear == QUEUESIZE - 1) {
    Serial.println("Overflow \n");
    for (int i = 0; i < 10; i++) {
      FlashLedVermelho();
    }
  } else {
    if (Front == -1)

      Front = 0;
    Rear = Rear + 1;
    queue[Rear] = insertItem;
  }
}

Coordenadas dequeue() {
  if (Front == -1 || Front > Rear) {
    Serial.println("Underflow \n");
    for (int i = 0; i < 10; i++) {
      FlashLedVermelho();
    }
  } else {
    Coordenadas elementoRemovido = queue[Front];
    Front = Front + 1;
    return elementoRemovido;
  }
}

void DesenhaCoordenadas(Coordenadas pixel, bool active) {
  int x = pixel.x;
  int y = pixel.y;
  if (x >= 8) {
    x -= 8;
  }

  if (y >= 8) {
    y -= 8;
  }
  int faseDesenhar = GetFaseAtual(pixel);
  switch (faseDesenhar) {
    case 0:
      matrix0.writePixel(x, y, active);
      matrix0.display();
      break;
    case 1:
      matrix1.writePixel(x, y, active);
      matrix1.display();
      break;
    case 2:
      matrix2.writePixel(x, y, active);
      matrix2.display();
      break;
    case 3:
      matrix3.writePixel(x, y, active);
      matrix3.display();
      break;
  }
}

void StartJogoCobra() {
  delay(300);
  jogadorCoordenadas.x = randomNumber(0, 15);
  jogadorCoordenadas.y = randomNumber(0, 15);

  delay(300);
  frutaCoordenadas.x = randomNumber(0, 15);
  frutaCoordenadas.y = randomNumber(0, 15);

  enqueue(jogadorCoordenadas);

  DesenhaCoordenadas(jogadorCoordenadas, true);
  DesenhaCoordenadas(frutaCoordenadas, true);

  Serial.println(jogadorCoordenadas.x);
  Serial.println(jogadorCoordenadas.y);
  Serial.println(frutaCoordenadas.x);
  Serial.println(frutaCoordenadas.y);
  // while handle movement
}

void TaskUpdateJogoCobra(void *pvParameters) {

  StartJogoCobra();
  int dir;
  int direcaoAtual = -1;
  bool isGameOver = false;
  
  while(direcaoAtual == -1){
    direcaoAtual = TrataFisicas();
  }
  dir = direcaoAtual;
  while (1) {
    direcaoAtual = TrataFisicas();
    
    if (direcaoAtual != -1) {
      dir=direcaoAtual;
    }

    switch (dir)
	  {
	  case 0:
		  jogadorCoordenadas.y++;
		  if(jogadorCoordenadas.y>15){
			  isGameOver=true;
		  }
		  break;
	  case 1:
		  jogadorCoordenadas.y--;
		  if(jogadorCoordenadas.y<0){
			  isGameOver=true;
		  }
		  break;
	  case 2:
		  jogadorCoordenadas.x--;
		  if(jogadorCoordenadas.x<0){
			  isGameOver=true;
		  }
		  break;
	  case 3:
		  jogadorCoordenadas.x++;
		  if(jogadorCoordenadas.x>15){
			  isGameOver=true;
		  }
		  break;
	  }

	  isGameOver |= BateuEmSiMesmo();

	  if(isGameOver){
	  	GameOver();
	  	return;
	  }

    enqueue(jogadorCoordenadas);

    bool apanhouFruta = (jogadorCoordenadas.x == frutaCoordenadas.x && jogadorCoordenadas.y == frutaCoordenadas.y);
    if (apanhouFruta) {
      FlashLedVerde();
      RerollFruta();
      DesenhaCobra();
      DesenhaCoordenadas(frutaCoordenadas, true);
    } else {
      DesenhaCobra();
      DesenhaCoordenadas(dequeue(), false);
    }

    vTaskDelay(1);
  }
}

void RerollFruta() {
  do {
    frutaCoordenadas.x = randomNumber(0, 15);
    frutaCoordenadas.y = randomNumber(0, 15);
  } while (frutaCoordenadas.x == jogadorCoordenadas.x && frutaCoordenadas.y == jogadorCoordenadas.y);
}

void DesenhaCobra() {
  for (int i = Front; i <= Rear; i++) {
    Coordenadas pixel = queue[i];
    int x = pixel.x;
    int y = pixel.y;
    if (x >= 8) {
      x -= 8;
    }

    if (y >= 8) {
      y -= 8;
    }
    int faseDesenhar = GetFaseAtual(pixel);
    switch (faseDesenhar) {
      case 0:
        matrix0.writePixel(x, y, true);
        break;
      case 1:
        matrix1.writePixel(x, y, true);
        break;
      case 2:
        matrix2.writePixel(x, y, true);
        break;
      case 3:
        matrix3.writePixel(x, y, true);
        break;
    }
  }
  matrix0.display();
  matrix1.display();
  matrix2.display();
  matrix3.display();
}

bool BateuEmSiMesmo() {
  for (int i = Front; i <= Rear; i++) {
    Coordenadas corpoCobra = queue[i];
    if (jogadorCoordenadas.x == corpoCobra.x && jogadorCoordenadas.y == corpoCobra.y) {
      return true;
    }
  }
  return false;
}

void GameOver() {
  jogoON = false;
  ClearMatrixTotal();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("PERDESTE :(");
  delay(5000);
  NVIC_SystemReset();
}
