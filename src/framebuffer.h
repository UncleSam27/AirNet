
#ifndef _framebuffer_h_
#define _framebuffer_h_

#include "airnetframe.h"


#define RepeatDelay 50

//тут все неоптимально, сделано напрямую и тупо в лоб 
// можно все сильно оптимизировать ускорив в десяток раз, но это только тестовое задание так что пока так

struct BufType{
 AirNetFrame * Frame;    // ссылка на кадр
 uint16_t TimerCounter;  // Таймер до следующей отправки (уменьшается при каждом поиске кадров для обработки при достижении 0 кадр отправляется)
 uint8_t  Attempt;       // Счетчик количества попыток отправки (уменьшается при отправке при достижении 0 кадр полностью удаляется из очереди)
};


//class Frame buffer
class FrameBuffer{
  private:
  uint16_t Lenght;
  BufType *Buf;

  public:
  // constructor
  FrameBuffer(uint16_t initLenght){
    Lenght = initLenght;
    Buf = new BufType[Lenght];
    if(Buf !=NULL){
      for(uint16_t Counter = 0; Counter<Lenght; Counter++){
        Buf[Counter].Frame = NULL;   //Clear Buffers
        Buf[Counter].Attempt = 1;
        Buf[Counter].TimerCounter = 1;
      }
    }
  }

  // destructor
  ~FrameBuffer(){
    //Удаляем все объекты в буфере
    for(uint16_t Counter = 0; Counter<Lenght; Counter++){
      if(Buf[Counter].Frame != NULL){
        delete Buf[Counter].Frame;
        Buf[Counter].Frame = NULL;
      }
    }  
    //удаляем буфер
    delete Buf;
  }

    //Процедура добавляет кадр в буффер
    bool AddFrameToBuf(AirNetFrame* Frame, uint8_t InitAttempt){
      for(uint16_t Counter = 0; Counter<Lenght; Counter++){
        if(Buf[Counter].Frame == NULL){
          Buf[Counter].Frame = Frame;
          Buf[Counter].Attempt = InitAttempt;
          Buf[Counter].TimerCounter = 1;     // кадр может быть отдан обработчику сразу после добавления
          return true;
        }
      } 
      delete Frame;    // если свободный слот не найден удаляем кадр (возможно стоит сделать не удаление последнего, а выбрасывание случайного хз)
      return false;
    }

    //Процедура добавляет кадр в буффер с одной попыткой отправки
    void AddFrameToBuf(AirNetFrame* Frame){
      AddFrameToBuf(Frame, 1);  //одна попытка для отправки по умолчанию
    }

    // Пытается удалить кадр из буфера, если счетчик попыток истек то удаляем окончательно
    void RemoveFrameFromBuf(AirNetFrame *Frame){
      for(uint16_t Counter = 0; Counter<Lenght; Counter++){
        if(Buf[Counter].Frame == Frame){   //если это нужный кадр
          Buf[Counter].Attempt--;          // уменьшаем количество попыток отправки
          if(Buf[Counter].Attempt==0){     // Если счетчик попыток удаления достиг нуля - действительно  удаляем
            delete(Frame);
            Buf[Counter].Frame = NULL;
          }else{                           // если не достиг нуля заново инициализируем таймер до следующей передачи
            Buf[Counter].TimerCounter = RepeatDelay;
          }
        }
      } 
    }

    //Сразу удаляет указанный кадр из буфера
    void RemoveFrameFromBufNow(AirNetFrame *Frame){
      for(uint16_t Counter = 0; Counter<Lenght; Counter++){
        if(Buf[Counter].Frame == Frame){ 
          delete(Frame);
          Buf[Counter].Frame = NULL;
        }
      } 
    }

    //удаляем кадр по паре адрес источника/идентификатор кадра (при получении подтверждения)
    void RemoveFrameFromBufByID(AirNetFrame *Frame){
      AirNetFrame *FrameInBuf;
      for(uint16_t Counter = 0; Counter<Lenght; Counter++){
        if(Buf[Counter].Frame != NULL){ 
          if((Frame->FrameID == Buf[Counter].Frame->FrameID) && (Frame->SourceAddress == Buf[Counter].Frame->DestinationAddress)){
              delete Buf[Counter].Frame;
              Buf[Counter].Frame = NULL;
            }
        } 
      }     
    }

    //Get Frame From Bufer
    AirNetFrame* GetFrameFromBuf(){
      for(uint16_t Counter = 0; Counter<Lenght; Counter++){
        if(Buf[Counter].Frame != NULL){
          if(Buf[Counter].TimerCounter > 0){
            Buf[Counter].TimerCounter--;       //уменьшаем таймер до следующей передачи
          }
          if(Buf[Counter].TimerCounter == 0){  //если таймер истек возвращаем ссылку на кадр
            return Buf[Counter].Frame;
          }
        }
      }
      return NULL;
   }
};




#endif
