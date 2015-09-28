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

#define POINT_1_ON 9
#define POINT_1 8
#define POINT_2_ON 7
#define POINT_2 6
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


RF24 radio(4, 3);
const uint64_t pipes[2] = {0xE7E7E7E7E7, 0xF0F0F0F0D2LL};

DCCCommandStation dcc;
int p1_state = LOW;
int p2_state = LOW;

void setup() {
  Serial.begin(57600);
  radio.begin();
  Serial.print("Radio working: ");
  radio.setPALevel(RF24_PA_LOW);
  Serial.println(radio.getPALevel() == RF24_PA_LOW ? "True" : "False");
  radio.setPALevel(RF24_PA_HIGH);
  Serial.println(radio.getPALevel() == RF24_PA_HIGH ? "True" : "False");
  radio.setDataRate(RF24_2MBPS);
  Serial.println(radio.getDataRate() == RF24_2MBPS ? "True" : "False");
  radio.setChannel(2);
  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1, pipes[0]);
  radio.enableDynamicPayloads();
  radio.setAutoAck(true);
  radio.powerUp();
  radio.startListening();
  pinMode(POINT_1_ON, OUTPUT);
  pinMode(POINT_2_ON, OUTPUT);
  pinMode(POINT_1, OUTPUT);
  pinMode(POINT_2, OUTPUT);
  delay(1000);
  point_trigger(1);
  delay(1000);
  point_trigger(1);
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
        Serial.println("Not interested: cmd_addr");
        break;
      case (CMD_SPEED):
        Serial.println("Not interested: cmd_addr");
        break;
      case (CMD_POINT):
        point_trigger(tPkt.addr);
        Serial.println("Switching point");
        break;
      default: Serial.println("Not recognised");
    }   
  }
}

void point_trigger(int point) {

  switch (point) {
    case (2): {
      p1_state = !p1_state;
      digitalWrite(POINT_1, p1_state);
      digitalWrite(POINT_1_ON, HIGH);
      delay(POINT_DELAY);
      digitalWrite(POINT_1_ON, LOW);
      break;
      } 
    case (3): {
      p2_state = !p2_state;
      digitalWrite(POINT_2, p2_state);
      digitalWrite(POINT_2_ON, HIGH);
      delay(POINT_DELAY);
      digitalWrite(POINT_2_ON, LOW);
      break;
      } 
  }

}