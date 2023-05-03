#ifndef _airnet_h_
#define _airnet_h_

/* Библиотека для обмена информацией через радиоканал */

#include "airnetframe.h"
#include "framebuffer.h"
#include "transceiver.h"
#include "protecttimer.h"

#define FrameSize sizeof(AirNetFrame)

#define RXBufLen 32
#define TXBufLen 64
#define RXIDBufLen 32

#define RetransmitTry 10  //количество попыток передачи фрейма

//Timings
#define Delay1ms 1           //1 миллисекунда
#define RXMaxDelay 5         //Максимальная задержка между получением байт в кадре (мс)
#define RXCheckInterval 10   //интервал проверки поступления нового кадра (мс)
#define TXCheckInterval 5    //интервал проверки поступления нового кадра в буфер отправки (мс)
#define TimerMaxInterval 30  //Максимальный интервал ожидания до разрешения передачи после последнего использования приемопередатчика(мс)

// IDs of Frame Types
#define DATA_FRAME 1
#define ACK_FRAME 2
#define PING_FRAME 3
#define RETRANSMIT_FRAME 4
#define Delay1ms        1         //1 миллисекунда

// Priority
#define PRI_LOW 0
#define PRI_HIGH 1
#define PRI_ULTRAHIGH 2

#define Loopback  0
#define Broadcast 0x0ff

#define Debug     false

// Main AirNet Class
class AirNet {
public:
  ProtectTimer *FrameTimer;

private:
  uint8_t NetworkAddress;  //Адрес сети
  uint8_t HostAddress;     //Адрес устройства
  uint16_t FrameID;        //Идентификатор пакета
  //uint8_t Priority;

  RecivedFrame LastReceivedFrames[RXIDBufLen];  // буфер идентификатоов последних полученных сообщений (чтобы исключить )
  uint16_t LastReceivedFramesIndex;


  Transceiver Trans;

  FrameBuffer *RXBuf;  //Принятые и отправляемые кадры будем хранить в классе буфере
  FrameBuffer *TXBuf;


  ////////////////////////////////////////////////////////////////
  //CRC Function
  void CRC8_byte(uint8_t &crc, uint8_t data) {
    uint8_t i = 8;
    while (i--) {
      crc = ((crc ^ data) & 1) ? (crc >> 1) ^ 0x8C : (crc >> 1);
      data >>= 1;
    }
  }

  uint8_t CRC8(uint8_t *buffer, uint8_t size) {
    uint8_t crc = 0;
    for (uint8_t Counter = 0; Counter < size; Counter++) CRC8_byte(crc, buffer[Counter]);
    return crc;
  }


  ////////////////////////////////////////////////////////////////
  // return frame id
  uint16_t GetNewFrameID() {
    return FrameID++;
  }

  //////////////////////////////////////////////////////////////////
  //возвращает True если пакет есть в списке последних принятых
  bool InLastReceivedFrames(AirNetFrame *Frame) {
    for (uint8_t Counter = 0; Counter < RXIDBufLen; Counter++) {
      if ((LastReceivedFrames[Counter].SourceAddress == Frame->SourceAddress) && (LastReceivedFrames[Counter].FrameID == Frame->FrameID))
        return true;
    }
    return false;
  }

  //Добавляем фрейм в список недавно полученых чтобы исключить повторную обработку
  void AddFrameToLastRecieved(AirNetFrame *Frame) {
    LastReceivedFrames[LastReceivedFramesIndex].SourceAddress = Frame->SourceAddress;
    LastReceivedFrames[LastReceivedFramesIndex].FrameID = Frame->FrameID;

    LastReceivedFramesIndex++;
    if (LastReceivedFramesIndex >= RXIDBufLen) {
      LastReceivedFramesIndex = 0;
    }
  }


  //процедура формирует ACK кадр для кадра и помещает его в буфер отправки.
  void SendACK(AirNetFrame *OriginFrame) {
    AirNetFrame *Frame;
    Frame = new (AirNetFrame);
    if (Frame != NULL) {
      Frame->NetworkAddress = NetworkAddress;
      Frame->DestinationAddress = OriginFrame->SourceAddress;
      Frame->SourceAddress = HostAddress;
      Frame->FrameType = ACK_FRAME;
      Frame->FrameID = OriginFrame->FrameID;
      Frame->EventID = 0;
      Frame->EventState = 0;
      Frame->CRC8 = CRC8((uint8_t *)Frame, FrameSize - sizeof(Frame->CRC8));
      TXBuf->AddFrameToBuf(Frame);
    }
  }

  //отправляем данные в указанном кадре
  inline void SendFrame(AirNetFrame *Frame) {
    if (Frame != NULL) {
      uint8_t *Pointer = (uint8_t *)Frame;
      if (Debug) Serial.println("Send Byte to Trans");
      char Str[30];
      for (uint8_t Counter = 0; Counter < FrameSize; Counter++) {
        if (Debug) {
          sprintf(Str, "Sended %02X ", Pointer[Counter]);
          Serial.println(Str);
        }
        Trans.WriteByte(Pointer[Counter]);
      }
      FrameTimer->Reset();
    }
  }


  ///////////////////////////////////////////////////////////////////////////
  // обработчик принятого кадра
  inline void RX_Working(uint8_t *Buffer) {
    AirNetFrame *Frame;

    Frame = (AirNetFrame *)Buffer;

    if (Frame->NetworkAddress != NetworkAddress) return;                                             //Если кадр не из нашей сети прекращаем обработку
    if (Frame->DestinationAddress != HostAddress && Frame->DestinationAddress != Broadcast) return;  //Если кадр не для нас прекращаем обработку
    if (Frame->CRC8 != CRC8((uint8_t *)Frame, FrameSize - sizeof(Frame->CRC8))) return;              //Если CRC8 не совпадает прекращаем обработку

    //Если мы добрались сюда то кадр для нас и не битый
    //копируем буфер в кадр
    Frame = new (AirNetFrame);
    if (Frame != NULL) {
      uint8_t *Pointer = (uint8_t *)Frame;
      for (uint8_t Counter = 0; Counter < FrameSize; Counter++) {  //Copy buffer to frame
        Pointer[Counter] = Buffer[Counter];
      }

      if (Frame->FrameType == DATA_FRAME) {  // если это кадр данных
        SendACK(Frame);                      // отправляем подтверждение
        if (InLastReceivedFrames(Frame)) {   // если данный кадр уже в списке последних принятых
          delete Frame;                      // просто удаляем его
        } else {                             // если он еще не принимался
          AddFrameToLastRecieved(Frame);     // Добавляем в список последних полученных
          RXBuf->AddFrameToBuf(Frame);       // Добавляем в буфер принятых кадров
        }
      } else if (Frame->FrameType == ACK_FRAME) {  //Если получено подтверждение
        TXBuf->RemoveFrameFromBufByID(Frame);      //удаляем из буфера
        delete Frame;

      } else if (Frame->FrameType == PING_FRAME) {
        SendACK(Frame);  //если это пинг, только отправляем подтверждение и никак не обрабатываем
        delete Frame;
      }
    }
  }

public:
  //////////////////////////////////////////////////////////////
  // Главный цикл обработки входящих данных должен периодически вызываться программой пользователя (пока переписал под работу в FreeRTOS))
  void RX_processing() {
    uint8_t Buffer[FrameSize];  //Буфер для получения кадров
    uint8_t Index = 0;
    uint8_t TimerCounter;

    //debug
    Serial.write("start RX proc\n");
    while (1) {
      if (Trans.NewAvailByte()) {  // если есть кадр для приема
        FrameTimer->Reset();       // сбрасываем таймер разрешения передачи
        Buffer[Index] = Trans.ReadByte();
        Index++;
        TimerCounter = 0;          // сбрасываем таймер ожидания следующего байта
        if (Index == FrameSize) {  //Если получили полный кадр - обрабатываем его.
          Index = 0;
          RX_Working(Buffer);
        }
      } else {
        if (Index > 0) {           //если есть данные в буфере ожидаем миллисекунду
          vTaskDelay(Delay1ms);    // ожидаем миллисекунду    ///потом проработать тайминги в зависимости от реальной скорости обмена
          TimerCounter++;          //Увеличиваем таймер счетчик

          if (TimerCounter > RXMaxDelay) {  //если после приема последнего байта прошло более RXMaxDelay сбрасываем принятые данные
            Index = 0;
            TimerCounter = 0;
          }
        } else {
          if (Trans.AirIsBusy()) {  // если ведется прием или передача
            FrameTimer->Reset();    // сбрасываем таймер передачи
          }
          vTaskDelay(RXCheckInterval);  //если данных для обработки нет - засыпаем
        }
      }
    }
  }

  //////////////////////////////////////////////////////////////
  // Главный цикл обработки исходящих данных должен периодически вызываться программой пользователя (пока переписал под работу в FreeRTOS))
  void TX_processing() {
    AirNetFrame *Frame = NULL;
    while (1) {
      if (FrameTimer->Ready()) {  //если таймер передачи готов
        Frame = NULL;
        Frame = TXBuf->GetFrameFromBuf();    //Проверяем есть ли данные для передачи (вернет ссылку на новый кадр только если пришло время его передачи)
        if (Frame != NULL) {                 //Если есть кадр для передачи
          SendFrame(Frame);                  //Отправляем кадр
          TXBuf->RemoveFrameFromBuf(Frame);  //Пытаемся удалить кадр из буфера (полностью будет удален только при достижении предельного количества попыток)
        }
      }
      vTaskDelay(TXCheckInterval);
    }
  }


  // Constructor
  AirNet(uint8_t InitNetworkAddress, uint8_t InitHostAddress, uint8_t Priority) {
    NetworkAddress = InitNetworkAddress;
    HostAddress = InitHostAddress;
    FrameID = 1;

    RXBuf = new FrameBuffer(RXBufLen);
    TXBuf = new FrameBuffer(TXBufLen);
    FrameTimer = new ProtectTimer(TimerMaxInterval, Priority);

    LastReceivedFramesIndex = 0;
    for (uint16_t Counter = 0; Counter < RXIDBufLen; Counter++) LastReceivedFrames[Counter].SourceAddress = 0;
  }

  // Destructor
  ~AirNet() {
    delete RXBuf;
    delete TXBuf;
    delete FrameTimer;
  }

  ////////////////////////////////////////////////////////////////
  //Send Event то
  void SendEvent(uint8_t DestinationAddress, uint16_t EventID, uint16_t EventState) {
    AirNetFrame *Frame = new AirNetFrame;
    if (Frame != NULL) {
      Frame->NetworkAddress = NetworkAddress;
      Frame->DestinationAddress = DestinationAddress;
      Frame->SourceAddress = HostAddress;
      Frame->FrameType = DATA_FRAME;
      Frame->FrameID = GetNewFrameID();
      Frame->EventID = EventID;
      Frame->EventState = EventState;
      Frame->CRC8 = CRC8((uint8_t *)Frame, FrameSize - sizeof(Frame->CRC8));
      TXBuf->AddFrameToBuf(Frame, RetransmitTry);
    }
  }



  ////////////////////////////////////////////////////////////////
  // возвращает true если имеются новые неполученные данные
  bool NewEvent() {
    if (RXBuf->GetFrameFromBuf() == NULL) {
      return false;
    }
    return true;
  }

  // отдает программе пользователя данные
  bool ReciveEvent(uint16_t *ID, uint16_t *State) {
    AirNetFrame *Frame = RXBuf->GetFrameFromBuf();
    if (Frame != NULL) {
      *ID = Frame->EventID;
      *State = Frame->EventState;
      RXBuf->RemoveFrameFromBufNow(Frame);
      return true;
    }
    return false;
  }


  //////////////////// DEBUG FUNCTIONS //////////////////////////

  ////////////////////////////////////////////////////////////////
  // Get Fake frame Event то RX Buffer
  void MakeFakeRecivedFrame(uint8_t SourceAddress, uint16_t EventID, uint16_t EventState) {
    AirNetFrame *Frame = new AirNetFrame;
    if (Frame != NULL) {
      Frame->NetworkAddress = NetworkAddress;
      Frame->DestinationAddress = HostAddress;
      Frame->SourceAddress = SourceAddress;
      Frame->FrameType = DATA_FRAME;
      Frame->FrameID = GetNewFrameID();
      Frame->EventID = EventID;
      Frame->EventState = EventState;
      Frame->CRC8 = CRC8((uint8_t *)Frame, FrameSize - sizeof(Frame->CRC8));
      RXBuf->AddFrameToBuf(Frame);
    }
  }

  ////////////////////////////////////////////////////////////////
  // Get Fake Buffer Event то RX Working
  void MakeFakeRecivedBuffer(uint8_t SourceAddress, uint16_t EventID, uint16_t EventState) {
    AirNetFrame *Frame = new AirNetFrame;
    if (Frame != NULL) {
      Frame->NetworkAddress = NetworkAddress;
      Frame->DestinationAddress = HostAddress;
      Frame->SourceAddress = SourceAddress;
      Frame->FrameType = DATA_FRAME;
      Frame->FrameID = GetNewFrameID();
      Frame->EventID = EventID;
      Frame->EventState = EventState;
      Frame->CRC8 = CRC8((uint8_t *)Frame, FrameSize - sizeof(Frame->CRC8));

      uint8_t *Pointer = (uint8_t *)Frame;
      char TmpBuf[FrameSize];
      RX_Working(Pointer);
      delete Frame;
    }
  }



};


#endif
