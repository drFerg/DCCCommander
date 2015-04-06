/********************
* Creates a minimum DCC command station from a potentiometer connected to analog pin 0,
* and a button connected to ground on one end and digital pin 4 on the other end. See this link
* http://www.arduino.cc/en/Tutorial/AnalogInput
* The DCC waveform is output on Pin 9, and is suitable for connection to an LMD18200-based booster directly,
* or to a single-ended-to-differential driver, to connect with most other kinds of boosters.
* The Differential DCC waveform is output on Pins 9 and 10.
********************/

#include <DCCPacket.h>
#include <DCCCommandStation.h>
#include <DCCHardware.h>

DCCCommandStation dcc;
unsigned int analog_value;
char speed_byte, old_speed = 0;
byte count = 0;
byte prev_state = 1;
byte F0 = 0;

void setup() {
  Serial.begin(115200);
  Serial.print("HELLO\n"); Serial.flush();
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

int i = 0;
int go = 1;

void loop() {
  //handle reading button, controls F0
  // byte button_state = digitalRead(4); //high == not pushed; low == pushed
  // if(button_state && (button_state != prev_state))
  // {
  //   //toggle!
  //   F0 ^= 1;
  //   Serial.println(F0,BIN);
  //   speed_byte = (F0 ? 30 : 0);
  // }

  //prev_state = button_state;
  if (go) {
    if (i < 127) i++;
    else go = 0;
  }
  else {
    if (i > 0) i--;
    else go = 1;
  }
  ++count;
  getNextPacket();
}
