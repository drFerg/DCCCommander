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
    
    //for configuration
    void setup(void); //for any post-constructor initialization
    
    //for enqueueing packets
    bool setSpeed14(uint16_t address, DCCAddrType addr_type, uint8_t speed, DCCDirection dir); //new_speed: [-13,13], and optionally F0 settings.
    bool setSpeed28(uint16_t addr, DCCAddrType addr_type, uint8_t speed, DCCDirection dir); // speed: [0, 32]
    bool setSpeed128(uint16_t addr, DCCAddrType addr_type, uint8_t speed, DCCDirection dir); //new_speed: [0,127]
    
    //the function methods are NOT stateful; you must specify all functions each time you call one
    //keeping track of function state is the responsibility of the calling program.
    bool setFunctions(uint16_t address, DCCAddrType addr_type, uint8_t F0to4, uint8_t F5to9=0x00, uint8_t F9to12=0x00);
    bool setFunctions(uint16_t address, DCCAddrType addr_type, uint16_t functions);
    bool setFunctions0to4(uint16_t address, DCCAddrType addr_type, uint8_t functions);
    bool setFunctions5to8(uint16_t address, DCCAddrType addr_type, uint8_t functions);
    bool setFunctions9to12(uint16_t address, DCCAddrType addr_type, uint8_t functions);
    //other cool functions to follow. Just get these working first, I think.
    
    bool setBasicAccessory(uint16_t address, uint8_t function);
    bool unsetBasicAccessory(uint16_t address, uint8_t function);
    
    bool opsProgramCV(uint16_t address, DCCAddrType addr_type, uint16_t CV, uint8_t CV_data);

    //more specific functions
    bool eStop(void); //all locos
    bool eStop(uint16_t address, DCCAddrType addr_type); //just one specific loco
};
#endif //__DCC_COMMANDSTATION_H__
