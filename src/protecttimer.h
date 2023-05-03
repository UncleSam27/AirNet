#ifndef _protecttimer_h_
#define _protecttimer_h_
#include "transceiver.h"

#define PRI_LOW 0
#define PRI_HIGH 1
#define PRI_ULTRAHIGH 2
#define Delay1ms        1         //1 миллисекунда


class ProtectTimer{
  uint16_t TimerCounter;
  uint8_t  MinDelay;
  uint8_t  MaxDelay;

 public:
  void Timer(){
    //debug
    Serial.write("start timer proc\n");
    while(1){
      if(TimerCounter>0){
        TimerCounter--;
      }
      vTaskDelay(Delay1ms);
    }
  }
 
  //Constructor
  ProtectTimer(uint16_t InitsValue, uint8_t initPriority){
    if(initPriority==PRI_HIGH){
      MinDelay = 1;
      MaxDelay = initPriority / 2;
    }else if(initPriority==PRI_LOW){
      MinDelay = initPriority / 2;
      MaxDelay = initPriority;
    }else if(initPriority==PRI_ULTRAHIGH){
      MinDelay = Delay1ms;
      MaxDelay = Delay1ms;
    }
    TimerCounter = random(MinDelay, MaxDelay);
   }

  bool Ready(){
    if (TimerCounter==0){
      return true;
    } 
    return false;
  }

  void Reset(){
    TimerCounter = random(MinDelay, MaxDelay);
  }
};

#endif
