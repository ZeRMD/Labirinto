#include <Wire.h>
#include "Grove_LED_Matrix_Driver_HT16K33.h"
#include <ArduinoBLE.h>
#include <Arduino_FreeRTOS.h>

#define addr0 0x70
#define addr1 0x71
#define addr2 0x72
#define addr3 0x73

TaskHandle_t Mapat, BTT;
QueueHandle_t Queue;

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
//FUNCTIONS

Mapa mapaAtual;

bool faseLimitada = false;

Matrix_8x8 matrix0;
Matrix_8x8 matrix1;
Matrix_8x8 matrix2;
Matrix_8x8 matrix3;

bool BTT_conected = false;

BLEService MapService("19B10010-E8F2-537E-4F6C-D104768A1214");  // create service

// create switch characteristic and allow remote device to read and write
BLEByteCharacteristic MapCharacteristic("19B10011-E8F2-537E-4F6C-D104768A1214", BLEWrite);

void setup() {

  Wire.begin();

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

  ClearAllMatrixes();

  Serial.begin(9600);

  BLE.begin();

  // set the local name peripheral advertises
  BLE.setLocalName("Mapa");
  // set the UUID for the service this peripheral advertises:
  BLE.setAdvertisedService(MapService);

  // add the characteristics to the service
  MapService.addCharacteristic(MapCharacteristic);

  // add the service
  BLE.addService(MapService);

  MapCharacteristic.writeValue(0);

  // start advertising
  BLE.advertise();

  Serial.println("BluetoothÂ® device active, waiting for connections...");
  /////////////////////////////////////////////////////////////////////////////////////////////////

}

void TaskMapa( uint8_t data) {
    Serial.print("Fase é limitada? ");
    Serial.println(faseLimitada);
    switch (data) {
      case 0:
        if (faseLimitada) {
          ClearAllMatrixes();
          DesenhaMapa(0, matrix0);
        }
        break;
      case 1:
        if (faseLimitada) {
          ClearAllMatrixes();
          DesenhaMapa(1, matrix1);
        }
        break;
      case 2:
        if (faseLimitada) {
          ClearAllMatrixes();
          DesenhaMapa(2, matrix2);
        }
        break;
      case 3:
        if (faseLimitada) {
          ClearAllMatrixes();
          DesenhaMapa(3, matrix3);
        }
        break;
      case 4:
        AtribuiMapa1();
        break;
      case 5:
        AtribuiMapa2();
        break;
      case 6:
        AtribuiMapa3();
        break;
      case 7:
        AtribuiMapa4();
        break;
      case 8:
        AtribuiMapa5();
        break;
      case 9:
        faseLimitada = true;
        break;
      case 10:
        faseLimitada = false;
        DesenhaMapaInteiro();
        break;
      case 11:
        ClearMatrix(matrix0);
        ClearMatrix(matrix1);
        ClearMatrix(matrix2);
        ClearMatrix(matrix3);
        break;
    }
}

void TaskBTT() {
    BLEDevice central = BLE.central();

    if (central) {
      Serial.print("Connected to central: ");
      Serial.println(central.address());

      while (central.connected()) {
        if (MapCharacteristic.written()) {
          uint8_t novaFase;
          Serial.print("Received: ");
          novaFase = MapCharacteristic.value();
          Serial.print(novaFase);
          TaskMapa( novaFase);
        }
      }
      ClearAllMatrixes();
      Serial.print("Disconnected from central: ");
      Serial.println(central.address());
    }
}

void loop() {
  TaskBTT();
}

void ClearAllMatrixes() {
  ClearMatrix(matrix0);
  ClearMatrix(matrix1);
  ClearMatrix(matrix2);
  ClearMatrix(matrix3);
}

void ClearMatrix(Matrix_8x8 matrix) {
  matrix.clear();
  matrix.display();
}

void AtribuiMapa1() {

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
  inicioVector2.x = 0;
  inicioVector2.y = 0;

  Coordenadas chaveVector2;
  chaveVector2.x = 9;
  chaveVector2.y = 13;

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
}

void AtribuiMapa2() {

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
}

void AtribuiMapa3() {

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
  inicioVector2.x = 2;
  inicioVector2.y = 15;

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
}

void AtribuiMapa4() {

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
  inicioVector2.y = 0;

  Coordenadas chaveVector2;
  chaveVector2.x = 14;
  chaveVector2.y = 10;

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
}

void AtribuiMapa5() {

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
  inicioVector2.x = 0;
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
}

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
