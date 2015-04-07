/* Creates an RF24 Command station for DCC control */
#include <DCCPacket.h>
#include <DCCCommandStation.h>
#include <DCCHardware.h>
#include <SPI.h>
#include "RF24.h"

#define FORWARD 1
#define REVERSE 2
#define SPEEDINCREMENT 2
#define STOPLIMIT 20

RF24 radio(4, 7);
const uint64_t pipes[2] = {0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL};

DCCCommandStation dcc;
unsigned int analog_value;
char speed_byte, old_speed = 0;
long count = 0;
byte prev_state = 1;
byte F0 = 0;

void setup() {
  Serial.begin(57600);
  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  Serial.print("Radio working: ");
  Serial.println(radio.getPALevel() ? "True" : "False");
  radio.setChannel(0x4c);
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
  dcc.setSpeed128(3, DCC_ADDR_SHORT, 10);
}

int i = 10;
int go = 1;
int command = 0;
int mappedSpeed = 0;
int trackSpeed = 0;
int trackDir = 1;

void loop() {
  if (radio.available()){
    // Fetch the payload.
    radio.read(&command, sizeof(uint8_t));
    Serial.println(command);

    if (command == FORWARD) {
        trackSpeed += SPEEDINCREMENT;
        if (trackDir == FORWARD && trackSpeed > 126)
          trackSpeed = 126; 
        else if (trackDir == REVERSE && trackSpeed > 0)
          trackDir = FORWARD;
      } else if (command == REVERSE) {
        trackSpeed -= SPEEDINCREMENT;
        if (trackDir == FORWARD && trackSpeed < 0)
          trackDir = REVERSE;
        else if (trackDir == REVERSE && trackSpeed < -126) 
          trackSpeed = -126;
      }
      Serial.print(trackSpeed);
      Serial.print("-");
      dcc.setSpeed128(3, DCC_ADDR_SHORT, trackSpeed);
  }
}
