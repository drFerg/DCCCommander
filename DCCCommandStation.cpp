#include "DCCCommandStation.h"
#include "DCCScheduler.h"
#include "DCCPacket.h"

#define INSTR_ADV_OPS   0x20
#define INSTR_14_28_SPEED 0x40
#define INSTR_128_SPEED 0x1F
#define DIR_BIT_28  5
#define DIR_BIT_128 7
#define STOP 0x01
#define ESTOP 0x00

DCCCommandStation::DCCCommandStation(void) {}

void DCCCommandStation::setup(void) {
  DCCPacket p, q;
  uint8_t data[] = {0x00};
  //Following RP 9.2.4, begin by putting 20 reset packets and 10 idle packets on the rails.
  dccshed_init();/* Init DCC hardware and scheduler */
  /* reset packet: address 0x00, data 0x00, XOR 0x00; S 9.2 line 75 */
  dccpkt_init(&p, DCC_ADDR_SHORT, DCC_BROADCAST_ADDR, PKT_RESET, data, 1, 20);
  dccshed_send(DCC_EPRI, &p);

  //idle packet: address 0xFF, data 0x00, XOR 0xFF; S 9.2 line 90
  dccpkt_init(&q, DCC_ADDR_SHORT, 0xFF, PKT_IDLE, data, 1, 10);
  dccshed_send(DCC_HIPRI, &q);
  printf(">> DCC Command Station setup completed!\n");
}

bool DCCCommandStation::setSpeed14(uint16_t address, DCCAddrType addr_type, uint8_t speed, DCCDirection dir) {
  DCCPacket p;
  uint8_t data[] = {INSTR_14_28_SPEED};

  if (dir == DCC_ESTOP || speed == ESTOP) return eStop(address, addr_type); /* e-stop is 0x01 */
  else if (dir == DCC_STOP || speed == STOP) data[0] |= 0x00; //stop
  else data[0] |= (speed | (dir << DIR_BIT_28));//convert from [2-127] to [1-14]
  
  dccpkt_init(&p, addr_type, address, PKT_SPEED, data, 1, SPEED_REPEAT);
  return dccshed_send(DCC_HIPRI, &p);
}

bool DCCCommandStation::setSpeed28(uint16_t addr, DCCAddrType addr_type, uint8_t speed, DCCDirection dir) {
  DCCPacket p;
  uint8_t data[] = {INSTR_14_28_SPEED};

  if (dir == DCC_ESTOP || speed == ESTOP) return eStop(addr, addr_type);/* e-stop is 0x01 */
  else if (dir == DCC_STOP || speed == STOP) data[0] |= 0x00; //stop
  else {
    data[0] |= (speed | (dir << DIR_BIT_28));
    /* least significant speed bit is moved to bit 4 (MSB), and rest is shifted down */
    data[0] |= ((data[0] & 0x1F) >> 1) | ((data[0] & 0x01) << 4);
  }
  dccpkt_init(&p, addr_type, addr, PKT_SPEED, data, 1, SPEED_REPEAT);
  return dccshed_send(DCC_HIPRI, &p);
}

bool DCCCommandStation::setSpeed128(uint16_t addr, DCCAddrType addr_type, uint8_t speed, DCCDirection dir) {
  DCCPacket p;
  uint8_t data[] = {INSTR_ADV_OPS | INSTR_128_SPEED, 0x00};

  if (dir == DCC_ESTOP || speed == ESTOP) return eStop(addr, addr_type); /* e-stop is 0x01 */
  else if (dir == DCC_STOP || speed == STOP) data[1] = 0x00; /* convert to regular stop */
  else data[1] = (speed | (dir << DIR_BIT_128));

  dccpkt_init(&p, addr_type, addr, PKT_SPEED, data, 2, SPEED_REPEAT);
  return dccshed_send(DCC_HIPRI, &p);
}

bool DCCCommandStation::setFunctions(uint16_t address, DCCAddrType addr_type, uint16_t functions) {
  if(setFunctions0to4(address, addr_type, functions&0x1F))
    if(setFunctions5to8(address, addr_type, (functions>>5)&0x0F))
      if(setFunctions9to12(address, addr_type, (functions>>9)&0x0F))
        return true;
  return false;
}

bool DCCCommandStation::setFunctions(uint16_t address, DCCAddrType addr_type, uint8_t F0to4, uint8_t F5to8, uint8_t F9to12)
{
  if(setFunctions0to4(address, addr_type, F0to4))
    if(setFunctions5to8(address, addr_type, F5to8))
      if(setFunctions9to12(address, addr_type, F9to12))
        return true;
  return false;
}

bool DCCCommandStation::setFunctions0to4(uint16_t address, DCCAddrType addr_type, uint8_t functions)
{
//  Serial.println("setFunctions0to4");
//  Serial.println(functions,HEX);
  DCCPacket p;
  uint8_t data[] = {0x80};
  
  //Obnoxiously, the headlights (F0, AKA FL) are not controlled
  //by bit 0, but by bit 4. Really?
  
  //get functions 1,2,3,4
  data[0] |= (functions>>1) & 0x0F;
  //get functions 0
  data[0] |= (functions&0x01) << 4;
  dccpkt_init(&p, addr_type, address, PKT_FUNCTION_1, data, 1, FUNCTION_REPEAT);
  return dccshed_send(DCC_LOPRI, &p);
}


bool DCCCommandStation::setFunctions5to8(uint16_t address, DCCAddrType addr_type, uint8_t functions)
{
//  Serial.println("setFunctions5to8");
//  Serial.println(functions,HEX);
  DCCPacket p;
  uint8_t data[] = {0xB0};
  
  data[0] |= functions & 0x0F;
  dccpkt_init(&p, addr_type, address, PKT_FUNCTION_2, data, 1, FUNCTION_REPEAT);
  return dccshed_send(DCC_LOPRI, &p);
}

bool DCCCommandStation::setFunctions9to12(uint16_t address, DCCAddrType addr_type, uint8_t functions)
{
//  Serial.println("setFunctions9to12");
//  Serial.println(functions,HEX);
  DCCPacket p;
  uint8_t data[] = {0xA0};
  
  //least significant four functions (F5--F8)
  data[0] |= functions & 0x0F;
  dccpkt_init(&p, addr_type, address, PKT_FUNCTION_3, data, 1, FUNCTION_REPEAT);
  return dccshed_send(DCC_LOPRI, &p);
}


//other cool functions to follow. Just get these working first, I think.

//bool DCCCommandStation::setTurnout(uint16_t address)
//bool DCCCommandStation::unsetTurnout(uint16_t address)

bool DCCCommandStation::opsProgramCV(uint16_t address, DCCAddrType addr_type, uint16_t CV, uint8_t CV_data)
{
  //format of packet:
  // {preamble} 0 [ AAAAAAAA ] 0 111011VV 0 VVVVVVVV 0 DDDDDDDD 0 EEEEEEEE 1 (write)
  // {preamble} 0 [ AAAAAAAA ] 0 111001VV 0 VVVVVVVV 0 DDDDDDDD 0 EEEEEEEE 1 (verify)
  // {preamble} 0 [ AAAAAAAA ] 0 111010VV 0 VVVVVVVV 0 DDDDDDDD 0 EEEEEEEE 1 (bit manipulation)
  // only concerned with "write" form here.
  
  DCCPacket p;
  uint8_t data[] = {0xEC, 0x00, 0x00};
  
  // split the CV address up among data uint8_ts 0 and 1
  data[0] |= ((CV-1) & 0x3FF) >> 8;
  data[1] = (CV-1) & 0xFF;
  data[2] = CV_data;
  
  dccpkt_init(&p, addr_type, address, PKT_OPS_MODE, data, 3, OPS_MODE_PROGRAMMING_REPEAT);
  return dccshed_send(DCC_LOPRI, &p);
}
//more specific functions

//broadcast e-stop command
bool DCCCommandStation::eStop(void)
{
    // 111111111111 0 00000000 0 01DC0001 0 EEEEEEEE 1
    DCCPacket p; //address 0
    uint8_t data[] = {0x71}; //01110001
    dccpkt_init(&p, DCC_ADDR_SHORT, 0x00, PKT_E_STOP, data, 1, 10);
    dccshed_send(DCC_EPRI, &p);
    return true;
}
    
bool DCCCommandStation::eStop(uint16_t address, DCCAddrType addr_type)
{
    // 111111111111 0	0AAAAAAA 0 01001001 0 EEEEEEEE 1
    // or
    // 111111111111 0	0AAAAAAA 0 01000001 0 EEEEEEEE 1
    DCCPacket p;
    uint8_t data[] = {0x41}; //01000001
    dccpkt_init(&p, addr_type, address, PKT_E_STOP, data, 1, 10);
    dccshed_send(DCC_EPRI, &p);
    return true;
}

bool DCCCommandStation::setBasicAccessory(uint16_t address, uint8_t function)
{
    DCCPacket p;

	  uint8_t data[] = { 0x01 | ((function & 0x03) << 1) };
    dccpkt_init(&p, DCC_ADDR_ACCESS, address, PKT_BASIC_ACCESSORY, data, 1, OTHER_REPEAT);
	  return dccshed_send(DCC_LOPRI, &p);

}

bool DCCCommandStation::unsetBasicAccessory(uint16_t address, uint8_t function)
{
		// DCCPacket p;

		// uint8_t data[] = { ((function & 0x03) << 1) };
  //   dccpkt_init(&p, DCC_ADDR_ACCESS, address, PKT_BASIC_ACCESSORY, data, 1, OTHER_REPEAT);
	 //  return dccshed_send(DCC_LOPRI, &p);
  return true;
}
