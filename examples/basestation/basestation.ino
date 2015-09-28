/* Creates an RF24 Command station for DCC control */

#include <DCCPacket.h>
#include <DCCCommandStation.h>
#include <DCCHardware.h>
#include <SPI.h>
#include "RF24.h"

#define MAX_DATA_SIZE 3

#define CMD_ADDR 0
#define CMD_SPEED 1
#define CMD_POINT 2

#define POINT_ON 6
#define POINT_PA 5
#define POINT_PB 3
#define POINT_DELAY 50

typedef struct train_packet {
  uint16_t cmd;
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


RF24 radio(4, 7);
const uint64_t pipes[2] = {0xE7E7E7E7E7, 0xF0F0F0F0D2LL};

DCCCommandStation dcc;
int point_state = 0;

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
  pinMode(POINT_ON, OUTPUT);
  pinMode(POINT_PA, OUTPUT);
  pinMode(POINT_PB, OUTPUT);
  delay(1000);
  point_trigger();
  delay(1000);
  point_trigger();

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
        Serial.print(tPkt.addr);
        Serial.print("-");
        Serial.println(aPkt->new_addr);
        break;
      case (CMD_SPEED):
        sPkt = (Speed_Packet *) &(tPkt.data);
        dcc.setSpeed128(tPkt.addr, DCC_ADDR_SHORT, sPkt->speed, (DCCDirection) sPkt->dir);
        Serial.println("Set speed");
        Serial.print(tPkt.addr);
        Serial.print("-");
        Serial.println(sPkt->speed);
        break;
      case (CMD_POINT):
        if (tPkt.addr == 1){
          point_trigger();
          Serial.println("Switching point");
        }
        break;
      default: Serial.println("Not recognised");
    }   
  }
}

void point_trigger() {
  if (point_state) digitalWrite(POINT_PA, HIGH);
  else digitalWrite(POINT_PB, HIGH);

  digitalWrite(POINT_ON, HIGH);
  delay(POINT_DELAY);
  digitalWrite(POINT_ON, LOW);
  digitalWrite(POINT_PA, LOW);
  digitalWrite(POINT_PB, LOW);
  point_state = !point_state;
}