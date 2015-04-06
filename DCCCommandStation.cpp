#include "DCCCommandStation.h"
#include "DCCScheduler.h"
#include "DCCPacket.h"

DCCCommandStation::DCCCommandStation(void) : default_speed_steps(128)
{
 
}
    
//for configuration
void DCCCommandStation::setDefaultSpeedSteps(uint8_t new_speed_steps)
{
  default_speed_steps = new_speed_steps;
}
    
void DCCCommandStation::setup(void) {
  DCCPacket p, q;
  uint8_t data[] = {0x00};
  //Following RP 9.2.4, begin by putting 20 reset packets and 10 idle packets on the rails.
  //use the e_stop_queue to do this, to ensure these packets go out first!

  /* Init DCC hardware and scheduler */
  Serial.print(dccshed_init());
  Serial.print("HIII\n");Serial.flush();
  /* reset packet: address 0x00, data 0x00, XOR 0x00; S 9.2 line 75 */
  dccpkt_init(&p, DCC_ADDR_SHORT, DCC_BROADCAST_ADDR, PKT_RESET, data, 1, 20);
  dccshed_send(DCC_HIPRI, &p);
  Serial.print("HIII\n");Serial.flush();

   
  //idle packet: address 0xFF, data 0x00, XOR 0xFF; S 9.2 line 90
  dccpkt_init(&q, DCC_ADDR_SHORT, 0xFF, PKT_IDLE, data, 1, 10);
  Serial.print("HIII\n");Serial.flush();
  dccshed_send(DCC_HIPRI, &p);
  
  Serial.println("SENT RESETS");Serial.flush();
}

//for enqueueing packets

//setSpeed* functions:
//new_speed contains the speed and direction.
// a value of 0 = estop
// a value of 1/-1 = stop
// a value >1 (or <-1) means go.
// valid non-estop speeds are in the range [1,127] / [-127,-1] with 1 = stop
bool DCCCommandStation::setSpeed(uint16_t address, DCCAddrType addr_type, int8_t new_speed, uint8_t steps) {
  uint8_t num_steps = (steps == 0 ? default_speed_steps : steps);
  /* Use the default; otherwise use the number of steps specified */

  switch(num_steps) {
    case 14: return(setSpeed14(address, addr_type, new_speed));
    case 28: return(setSpeed28(address, addr_type, new_speed));
    case 128: return(setSpeed128(address, addr_type, new_speed));
    default: return false; /* invalid number of steps */
  }
  
}

bool DCCCommandStation::setSpeed14(uint16_t address, DCCAddrType addr_type, int8_t new_speed, bool F0)
{
  DCCPacket p;
  uint8_t dir = 1;
  uint8_t speed[] = {0x40};
  uint16_t abs_speed = new_speed;
  if (new_speed < 0) {
    dir = 0;
    abs_speed = -new_speed;
  }
  if (new_speed == 0) //estop!
    return eStop(address, addr_type);//speed[0] |= 0x01; //estop
    
  else if (abs_speed == 1) //regular stop!
    speed[0] |= 0x00; //stop
  else //movement
    speed[0] |= map(abs_speed, 2, 127, 2, 15); //convert from [2-127] to [1-14]
  speed[0] |= (0x20*dir); //flip bit 3 to indicate direction;
  //Serial.println(speed_data_uint8_ts[0],BIN);
  dccpkt_init(&p, addr_type, address, PKT_SPEED, speed, 1, SPEED_REPEAT);

  //speed packets get refreshed indefinitely, and so the repeat doesn't need to be set.
  //speed packets go to the high proirity queue
  return dccshed_send(DCC_HIPRI, &p);
}

bool DCCCommandStation::setSpeed28(uint16_t address, DCCAddrType addr_type, int8_t new_speed)
{
  DCCPacket p;
  uint8_t dir = 1;
  uint8_t speed_data_uint8_ts[] = {0x40};
  uint16_t abs_speed = new_speed;
  if(new_speed<0)
  {
    dir = 0;
    abs_speed = new_speed * -1;
  }
//  Serial.println(speed);
//  Serial.println(dir);
  if(new_speed == 0) //estop!
    return eStop(address, addr_type);//speed_data_uint8_ts[0] |= 0x01; //estop
  else if (abs_speed == 1) //regular stop!
    speed_data_uint8_ts[0] |= 0x00; //stop
  else //movement
  {
    speed_data_uint8_ts[0] |= map(abs_speed, 2, 127, 2, 0X1F); //convert from [2-127] to [2-31]  
    //most least significant bit has to be shufled around
    speed_data_uint8_ts[0] = (speed_data_uint8_ts[0]&0xE0) | ((speed_data_uint8_ts[0]&0x1F) >> 1) | ((speed_data_uint8_ts[0]&0x01) << 4);
  }
  speed_data_uint8_ts[0] |= (0x20*dir); //flip bit 3 to indicate direction;
//  Serial.println(speed_data_uint8_ts[0],BIN);
//  Serial.println("=======");
  dccpkt_init(&p, addr_type, address, PKT_SPEED, speed_data_uint8_ts, 1, SPEED_REPEAT);

    
  //speed packets get refreshed indefinitely, and so the repeat doesn't need to be set.
  //speed packets go to the high proirity queue
  //return(high_priority_queue.insertPacket(&p));
  return dccshed_send(DCC_HIPRI, &p);
}

bool DCCCommandStation::setSpeed128(uint16_t address, DCCAddrType addr_type, int8_t new_speed)
{
  //why do we get things like this?
  // 03 3F 16 15 3F (speed packet addressed to loco 03)
  // 03 3F 11 82 AF  (speed packet addressed to loco 03, speed hex 0x11);
  DCCPacket p;
  uint8_t dir = 1;
  uint16_t abs_speed = new_speed;
  uint8_t speed_data_uint8_ts[] = {0x3F,0x00};
  if(new_speed<0)
  {
    dir = 0;
    abs_speed = new_speed * -1;
  }
  if(!new_speed) //estop!
    return eStop(address, addr_type);//speed_data_uint8_ts[0] |= 0x01; //estop
  else if (abs_speed == 1) //regular stop!
    speed_data_uint8_ts[1] = 0x00; //stop
  else //movement
    speed_data_uint8_ts[1] = abs_speed; //no conversion necessary.

  speed_data_uint8_ts[1] |= (0x80 * dir); //flip bit 7 to indicate direction;
  dccpkt_init(&p, addr_type, address, PKT_SPEED, speed_data_uint8_ts, 2, SPEED_REPEAT);
  //speed packets get refreshed indefinitely, and so the repeat doesn't need to be set.
  //speed packets go to the high proirity queue
  return dccshed_send(DCC_HIPRI, &p);
}

bool DCCCommandStation::setFunctions(uint16_t address, DCCAddrType addr_type, uint16_t functions)
{
//  Serial.println(functions,HEX);
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