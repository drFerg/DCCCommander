#include "DCCPacket.h"


DCCPacket::DCCPacket(unsigned int decoder_address) : address(decoder_address), kind(idle_packet_kind), size_repeat(0x40) //size(1), repeat(0)
{
  data[0] = 0x00; //default to idle packet
  data[1] = 0x00;
  data[2] = 0x00;
}

byte DCCPacket::getBitstream(byte rawbytes[]) //returns size of array.
{
  int total_size = 1; //minimum size
  
  if(kind == idle_packet_kind)  //idle packets work a bit differently:
  // since the "address" field is 0xFF, the logic below will produce C0 FF 00 3F instead of FF 00 FF
  {
    rawbytes[0] = 0xFF;
  }
  else if(address <= 127) //addresses of 127 or higher are 14-bit "extended" addresses
  {
    rawbytes[0] = (byte)address;
  }
  else //we have a 14-bit extended ("4-digit") address
  {
    rawbytes[0] = (byte)((address >> 8)|0xC0);
    rawbytes[1] = (byte)(address & 0xFF);
    ++total_size;
  }
  
  byte i;
  for(i = 0; i < (size_repeat>>6); ++i,++total_size)
  {
    rawbytes[total_size] = data[i];
  }

  byte XOR = 0;
  for(i = 0; i < total_size; ++i)
  {
    XOR ^= rawbytes[i];
  }
  rawbytes[total_size] = XOR;

  return total_size+1;  
}

byte DCCPacket::getSize(void)
{
  return (size_repeat>>6);
}

byte DCCPacket::addData(byte new_data[], byte new_size) //insert freeform data.
{
  for(int i = 0; i < new_size; ++i)
    data[i] = new_data[i];
  size_repeat = (size_repeat & 0x3F) | (new_size<<6);
}