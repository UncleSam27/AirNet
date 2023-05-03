#ifndef _airnetframe_h_
#define _airnetframe_h_

#pragma pack(push)
#pragma pack(1)
// RX Packet structure
struct AirNetFrame{
  uint8_t NetworkAddress;
  uint8_t DestinationAddress;
  uint8_t SourceAddress;  
  uint8_t FrameType;  
  uint16_t FrameID;
  uint16_t EventID;
  uint16_t EventState;
  uint8_t CRC8;
};
#pragma pack(pop)


struct RecivedFrame{
  uint8_t SourceAddress;  
  uint16_t FrameID;
};

#endif