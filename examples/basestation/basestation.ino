/* Creates an RF24 Command station for DCC control */

#include <DCCPacket.h>
#include <DCCCommandStation.h>
#include <DCCHardware.h>
#include <SPI.h>
#include "RF24.h"

#define MAX_DATA_SIZE 3

#define CMD_ADDR 0
#define CMD_SPEED 1

typedef struct train_packet {
  uint8_t cmd;
  uint16_t addr;
  uint8_t data[MAX_DATA_SIZE];
} Train_Packet;

typedef struct speed_packet {
  uint8_t speed;
  uint8_t dir;
} Speed_Packet;

typedef struct addr_packet {
  uint8_t new_addr;
} Addr_Packet;

typedef struct cv_packet {
  uint16_t cv_addr;
  uint8_t cv_data;
} CV_Packet;

#define FORWARD 1
#define REVERSE 2
#define SPEEDINCREMENT 2
#define STOPLIMIT 20

RF24 radio(4, 7);
const uint64_t pipes[2] = {0xE7E7E7E7E7, 0xF0F0F0F0D2LL};

DCCCommandStation dcc;

void setup() {
  Serial.begin(57600);
  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  Serial.print("Radio working: ");
  Serial.println(radio.getPALevel() ? "True" : "False");
  radio.setDataRate(RF24_2MBPS);
  radio.setChannel(2);
  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1, pipes[0]);
  radio.enableDynamicPayloads();
  radio.setAutoAck(true);
  radio.powerUp();
  radio.startListening();
  dcc.setup();
}

Train_Packet tPkt;
Speed_Packet *sPkt;
Addr_Packet *aPkt;

void loop() {
  if (radio.available()){
    // Fetch the payload.
    radio.read(&tPkt, sizeof(Train_Packet));
    switch(tPkt.cmd) {
      case (CMD_ADDR):
        aPkt = (Addr_Packet *) &(tPkt.data);
        dcc.setAddrShort(tPkt.addr, aPkt->new_addr);
        Serial.println("Set addr");
        break;
      case (CMD_SPEED):
        sPkt = (Speed_Packet *) &(tPkt.data);
        dcc.setSpeed128(tPkt.addr, DCC_ADDR_SHORT, sPkt->speed, (DCCDirection) sPkt->dir);
        Serial.println("Set speed");
        break;
    }   
  }
}
