#ifndef _transceiver_h_
#define _transceiver_h_

class Transceiver{
  public:
  // constructor iniit serial port
  Transceiver(){
    Serial2.begin(115200);
  }

  // Процедура возвращает True если есть новые данные для приема False если нет
  bool NewAvailByte(){
    if( Serial2.available()>0 )  return true;
    return false;
  }

  // Процедура возвращает True если ведется прием или передача False если нет (serial порт не поддерживает данную возможность для него просто проверяем наличие не принятых байт)
  bool AirIsBusy(){
    if( Serial2.available()>0 )  return true;
    return false;
  }

  // Процедура считывает данные из приемопередатчика
  uint8_t ReadByte(){
      return Serial2.read();
  }

  // Процедура считывает данные из приемопередатчика
  uint8_t WriteByte(uint8_t SendByte){  
      return Serial2.write(SendByte);
  }
};

#endif
