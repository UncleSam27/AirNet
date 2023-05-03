
#include "airnet.h"

#define Network 1
#define Address 1
#define RemoteAddress 2

AirNet Net(Network, Address, PRI_HIGH);  //Network - адрес сети, Address - адрес хоста, PRI_HIGH - приоритет захвата эфира

////////////////////////////////////////////////////////////////
// Узнать как именно будет это вызываться в реальном проекте и перенести эту инициализацию внутрь библиотеки.
xTaskHandle TX_Proc = NULL;
xTaskHandle RX_Proc = NULL;
xTaskHandle Timer_Proc = NULL;

void TX_Processing(void *pvParameters) { Net.TX_processing(); }

void RX_Processing(void *pvParameters) { Net.RX_processing(); }

void Timer_Processing(void *pvParameters) {
  if (Net.FrameTimer != NULL) Net.FrameTimer->Timer();
}

void StartNetProc() {  //запускааем задачи обработки сети на нулевом ядре ESP32 (программа пользователя запускается на первом)
  xTaskCreatePinnedToCore(TX_Processing, "TXProc", 5000, NULL, tskIDLE_PRIORITY + 1, &TX_Proc, 0);
  xTaskCreatePinnedToCore(RX_Processing, "RXProc", 5000, NULL, tskIDLE_PRIORITY + 1, &RX_Proc, 0);
  xTaskCreatePinnedToCore(Timer_Processing, "TimerProc", 5000, NULL, tskIDLE_PRIORITY + 1, &Timer_Proc, 0);
}
////////////////////////////////////////////////////////////////



void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  StartNetProc();
}

void loop() {
  uint16_t Timer = 0;
  vTaskDelay(5000);

  while (1) {
    Timer++;
    if (Timer == 1000) {                         // раз в секунду отправляем данные
      Serial.println("Send Data.");
      Net.SendEvent(RemoteAddress, 0x10, 0x55);  //Event ID = 10hex   Event value = 55hex
      Timer = 0;
    }

    if (Net.NewEvent()) {
      uint16_t EventID, EventState;

      Net.ReciveEvent(&EventID, &EventState);

      Serial.println("Packet recived");
      Serial.println(EventID);
      Serial.println(EventState);
    }
    vTaskDelay(Delay1ms);
  }
}
