#ifndef __DCCCOMMANDSTATION_H__
#define __DCCCOMMANDSTATION_H__
#include "DCCPacket.h"

typedef enum {
    DCC_STOP = 0,
    DCC_ESTOP = 1,
    DCC_FORWARD = 2,
    DCC_REVERSE = 3,
} DCCDirection;

class DCCCommandStation {
  public:
      
    DCCCommandStation(void);
    
    void setup(void); /* For any post-constructor initialization */
    
    /* Send a soft reset a loco */
	bool reset(uint16_t addr, DCCAddrType addr_type);
    
    /* 
     * Send a speed setting to a loco 
     * Returns true if it was successfully added to the scheduler
     */
    bool setSpeed14(uint16_t addr, DCCAddrType addr_type, uint8_t speed, DCCDirection dir); //new_speed: [-13,13], and optionally F0 settings.
    bool setSpeed28(uint16_t addr, DCCAddrType addr_type, uint8_t speed, DCCDirection dir); // speed: [0, 32]
    bool setSpeed128(uint16_t addr, DCCAddrType addr_type, uint8_t speed, DCCDirection dir); //new_speed: [0,127]
    
    /* Set functions for device at addr
     * P.s. the function methods are stateless; you must specify all functions each time you call one
     */
    bool setFunctions(uint16_t addr, DCCAddrType addr_type, uint8_t F0to4, uint8_t F5to9=0x00, uint8_t F9to12=0x00, uint8_t F13to20=0x00, uint8_t F21to28=0x00);
    bool setFunctions(uint16_t addr, DCCAddrType addr_type, uint32_t functions);
    bool setFunctions0to4(uint16_t addr, DCCAddrType addr_type, uint8_t functions);
    bool setFunctions5to8(uint16_t addr, DCCAddrType addr_type, uint8_t functions);
    bool setFunctions9to12(uint16_t addr, DCCAddrType addr_type, uint8_t functions);
	bool setFunctions13to20(uint16_t addr, DCCAddrType addr_type, uint8_t functions);
	bool setFunctions21to28(uint16_t addr, DCCAddrType addr_type, uint8_t functions);
    
    bool setBasicAccessory(uint16_t addr, uint8_t function);
    bool unsetBasicAccessory(uint16_t addr, uint8_t function);
    
    bool opsProgramCV(uint16_t addr, DCCAddrType addr_type, uint16_t CV, uint8_t CV_data);
    bool setAddrShort(uint16_t addr, uint16_t new_addr);

    /* Send eStop to all or a specfic loco 
     * These are emergency packets and sent immediately 
     */
    bool eStop(void);
    bool eStop(uint16_t addr, DCCAddrType addr_type);
};
#endif //__DCC_COMMANDSTATION_H__
