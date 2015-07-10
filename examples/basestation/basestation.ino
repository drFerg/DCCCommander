/* Creates an RF24 Command station for DCC control */

#include <DCCPacket.h>
#include <DCCCommandStation.h>
#include <DCCHardware.h>
#include <SPI.h>
#include "RF24.h"

typedef struct train_packet {
  uint16_t addr;
  uint8_t speed;
  uint8_t dir;
}Train_Packet;

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
  pinMode(8, OUTPUT);
  pinMode(13, OUTPUT);
  digitalWrite(8, HIGH);
  digitalWrite(13, HIGH);
  //set up button on pin 4
  pinMode(4, INPUT);
  digitalWrite(4, HIGH); //activate built-in pull-up resistor  
}

int i = 10;
Train_Packet tPkt;

void loop() {
  if (radio.available()){
    // Fetch the payload.
    radio.read(&tPkt, sizeof(Train_Packet));
    Serial.println(tPkt.addr);
    Serial.print(tPkt.speed);
    Serial.print("-");
    Serial.println(tPkt.dir);

    dcc.setSpeed128(tPkt.addr, DCC_ADDR_SHORT, tPkt.speed, (DCCDirection) tPkt.dir);
  }
}
