#include "DCCCommandStation.h"
#include "DCCScheduler.h"
#include "DCCPacket.h"

#define DIR_BIT_28  5
#define DIR_BIT_128 7
#define STOP 0x00
#define ESTOP 0x01

#define INSTR_ADV_OPS   0x20
#define INSTR_14_28_SPEED 0x40
#define INSTR_128_SPEED 0x1F
#define INSTR_CV_WRITE 0xEC
#define INSTR_CV_VERIFY 0xE4
#define INSTR_CV_BIT_MAN 0xE8
#define INSTR_FNC_GROUP_ONE 0x80
#define INSTR_FNC_GROUP_TWO 0xA0
#define INSTR_FNC_GROUP_TWO_EXT 0xB0


#define CV_ADDR_SHORT 0x01
#define CV_START_VOLT 0x02
#define CV_ACC_RATE   0x03
#define CV_SLOW_RATE  0x04
#define CV_MAX_VOLT   0x05
#define CV_VOLT_MID   0x06
#define CV_RESET_DCC  0x08
#define CV_SPEED_STEPS 0x1D
#define CV_BEMF_EFFECT 0x39

#define SPEED_REPEAT      3
#define FUNCTION_REPEAT   3
#define E_STOP_REPEAT     5
#define OPS_MODE_PROGRAMMING_REPEAT 3
#define OTHER_REPEAT      2

DCCCommandStation::DCCCommandStation() {}

void DCCCommandStation::setup() {
  DCCPacket p, q;
  uint8_t data[] = {0x00};
  /* Following RP 9.2.4, begin by putting 20 reset packets and 10 idle packets on the rails.
   * Init DCC hardware and scheduler */
  dccshed_init();
  /* Reset all trains - S 9.2 line 75 */
  dccpkt_init(&p, DCC_ADDR_SHORT, DCC_BROADCAST_ADDR, PKT_RESET, 
              data, sizeof data, 20);
  dccshed_send(DCC_EPRI, &p);

  /* Send Idle packet after reset - S 9.2 line 90 */
  dccpkt_init(&q, DCC_ADDR_SHORT, 0xFF, PKT_IDLE, data, sizeof data, 10);
  dccshed_send(DCC_HIPRI, &q);
  printf(">> DCC Command Station setup completed!\n");
}

bool DCCCommandStation::reset(uint16_t addr, DCCAddrType addr_type) {
	  DCCPacket p;
	  uint8_t data[] = {0x00};
	  dccpkt_init(&p, addr_type, addr, PKT_RESET, data, sizeof data, 20);
	  return dccshed_send(DCC_EPRI, &p);
}

bool DCCCommandStation::setSpeed14(uint16_t addr, DCCAddrType addr_type, uint8_t speed, DCCDirection dir) {
  DCCPacket p;
  uint8_t data[] = {INSTR_14_28_SPEED};

  if (dir == DCC_ESTOP || speed == ESTOP) return eStop(addr, addr_type); /* e-stop is 0x01 */
  else if (dir == DCC_STOP || speed == STOP) data[0] |= 0x00; //stop
  else data[0] |= (speed | (dir << DIR_BIT_28));//convert from [2-127] to [1-14]
  
  dccpkt_init(&p, addr_type, addr, PKT_SPEED, data, sizeof data, SPEED_REPEAT);
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
    data[0] = ((data[0] & 0x1F) >> 1) | ((data[0] & 0x01) << 4) | (data[0]&0xE0);
  }
  dccpkt_init(&p, addr_type, addr, PKT_SPEED, data, sizeof data, SPEED_REPEAT);
  return dccshed_send(DCC_HIPRI, &p);
}

bool DCCCommandStation::setSpeed128(uint16_t addr, DCCAddrType addr_type, uint8_t speed, DCCDirection dir) {
  DCCPacket p;
  uint8_t data[] = {INSTR_ADV_OPS | INSTR_128_SPEED, 0x00};

  if (dir == DCC_ESTOP || speed == ESTOP) return eStop(addr, addr_type); /* e-stop is 0x01 */
  else if (dir == DCC_STOP || speed == STOP) data[1] = 0x00; /* convert to regular stop */
  else data[1] = (speed | (dir << DIR_BIT_128));

  dccpkt_init(&p, addr_type, addr, PKT_SPEED, data, sizeof data, SPEED_REPEAT);
  return dccshed_send(DCC_HIPRI, &p);
}

bool DCCCommandStation::setFunctions(uint16_t addr, DCCAddrType addr_type, uint16_t functions) {
  if(setFunctions0to4(addr, addr_type, functions & 0x1F))
    if(setFunctions5to8(addr, addr_type, (functions >> 5) & 0x0F))
      return setFunctions9to12(addr, addr_type, (functions >> 9) & 0x0F);
  return false;
}

bool DCCCommandStation::setFunctions(uint16_t addr, DCCAddrType addr_type, uint8_t F0to4, uint8_t F5to8, uint8_t F9to12) {
  if(setFunctions0to4(addr, addr_type, F0to4))
    if(setFunctions5to8(addr, addr_type, F5to8))
      return setFunctions9to12(addr, addr_type, F9to12);
  return false;
}

bool DCCCommandStation::setFunctions0to4(uint16_t addr, DCCAddrType addr_type, uint8_t functions) {
  DCCPacket p;
  uint8_t data[] = {INSTR_FNC_GROUP_ONE};
  
  /* The headlights (F0, AKA FL) are not controlled by bit 0, but by bit 4. */
  
  /* get functions 1,2,3,4 */
  data[0] |= (functions >> 1) & 0x0F;
  /* get functions 0 */
  data[0] |= (functions & 0x01) << 4;
  dccpkt_init(&p, addr_type, addr, PKT_FUNCTION_1,
              data, sizeof data, FUNCTION_REPEAT);
  return dccshed_send(DCC_LOPRI, &p);
}


bool DCCCommandStation::setFunctions5to8(uint16_t addr, DCCAddrType addr_type, uint8_t functions) {
  DCCPacket p;
  uint8_t data[] = {INSTR_FNC_GROUP_TWO};
  
  data[0] |= functions & 0x0F;
  dccpkt_init(&p, addr_type, addr, PKT_FUNCTION_2,
              data, sizeof data, FUNCTION_REPEAT);
  return dccshed_send(DCC_LOPRI, &p);
}

bool DCCCommandStation::setFunctions9to12(uint16_t addr, DCCAddrType addr_type, uint8_t functions) {
  DCCPacket p;
  uint8_t data[] = {INSTR_FNC_GROUP_TWO_EXT};
  
  //least significant four functions (F5--F8)
  data[0] |= functions & 0x0F;
  dccpkt_init(&p, addr_type, addr, PKT_FUNCTION_3,
              data, sizeof data, FUNCTION_REPEAT);
  return dccshed_send(DCC_LOPRI, &p);
}

//bool DCCCommandStation::setTurnout(uint16_t addr)
//bool DCCCommandStation::unsetTurnout(uint16_t addr)

bool DCCCommandStation::opsProgramCV(uint16_t addr, DCCAddrType addr_type, uint16_t cv, uint8_t cv_data) {
  /* s-9.2.1_2012_07 page 8
   * format of packet:
   * {preamble} 0 [ AAAAAAAA ] 0 111011VV 0 VVVVVVVV 0 DDDDDDDD 0 EEEEEEEE 1 (write) 
   */
  DCCPacket p;
  uint8_t data[] = {INSTR_CV_WRITE, 0x00, 0x00};
  
  /* Split up 10-bit cv addr among first and second data bytes */
  data[0] |= ((cv - 1) & 0x3FF) >> 8; /* 2 most significant bits in 1st byte */
  data[1] = (cv - 1) & 0xFF; /* Other 8 in 2nd byte */
  data[2] = cv_data;
  
  dccpkt_init(&p, addr_type, addr, PKT_OPS_MODE, 
              data, sizeof data, OPS_MODE_PROGRAMMING_REPEAT);
  return dccshed_send(DCC_LOPRI, &p);
}


bool DCCCommandStation::setAddrShort(uint16_t addr, uint16_t new_addr) {
  return opsProgramCV(addr, DCC_ADDR_SHORT, CV_ADDR_SHORT, (uint8_t) new_addr);
}

/* broadcast e-stop command */
bool DCCCommandStation::eStop() {
    // 111111111111 0 00000000 0 01DC0001 0 EEEEEEEE 1
    DCCPacket p;
    uint8_t data[] = {0x71}; //01110001
    dccpkt_init(&p, DCC_ADDR_SHORT, DCC_BROADCAST_ADDR, PKT_E_STOP,
                data, sizeof data, E_STOP_REPEAT);
    return dccshed_send(DCC_EPRI, &p);
}
    
bool DCCCommandStation::eStop(uint16_t addr, DCCAddrType addr_type) {
    // 111111111111 0	0AAAAAAA 0 01001001 0 EEEEEEEE 1
    // or
    // 111111111111 0	0AAAAAAA 0 01000001 0 EEEEEEEE 1
    DCCPacket p;
    uint8_t data[] = {0x41}; //01000001
    dccpkt_init(&p, addr_type, addr, PKT_E_STOP,
                data, sizeof data, E_STOP_REPEAT);
    return dccshed_send(DCC_EPRI, &p);
}

bool DCCCommandStation::setBasicAccessory(uint16_t addr, uint8_t function) {
    DCCPacket p;
	  uint8_t data[] = {0x01 | ((function & 0x03) << 1)};
    dccpkt_init(&p, DCC_ADDR_ACCESS, addr, PKT_BASIC_ACCESSORY,
                data, sizeof data, OTHER_REPEAT);
	  return dccshed_send(DCC_LOPRI, &p);

}

bool DCCCommandStation::unsetBasicAccessory(uint16_t addr, uint8_t function)
{
		// DCCPacket p;

		// uint8_t data[] = { ((function & 0x03) << 1) };
  //   dccpkt_init(&p, DCC_ADDR_ACCESS, addr, PKT_BASIC_ACCESSORY, data, 1, OTHER_REPEAT);
	 //  return dccshed_send(DCC_LOPRI, &p);
  return true;
}
