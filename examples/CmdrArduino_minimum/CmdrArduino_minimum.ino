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

#define BUTTON 4

DCCCommandStation dcc;
unsigned int analog_value;
char speed_byte, old_speed = 0;
byte count = 0;
byte prev_state = 1;
byte F0 = 0;

void setup() {
  Serial.begin(115200);
  dcc.setup();
  pinMode(BUTTON, INPUT);
  digitalWrite(BUTTON, HIGH); //activate built-in pull-up resistor  
}

void loop() {
  /* handle reading button, controls F0 */
  byte button_state = digitalRead(BUTTON); /* high == not pushed; low == pushed */
  if (button_state && (button_state != prev_state)){
    F0 ^= 1; /* toggle */
    Serial.println(F0, BIN);
    dcc.setFunctions0to4(3, DCC_ADDR_SHORT, F0);
  }
  prev_state = button_state;

  /* handle reading throttle */
  analog_value = analogRead(0);
  speed_byte = (analog_value >> 2) - 127 ; //divide by four to take a 0-1023 range number and make it 1-126 range.
  if(speed_byte != old_speed) {
    if(speed_byte == 0) { /* would be treated as e-stop */
      speed_byte = 1;
    }
    Serial.print("analog = ");
    Serial.println(analog_value, DEC);
    Serial.print("digital = ");
    Serial.println(speed_byte, DEC);
    dcc.setSpeed128(3, DCC_ADDR_SHORT, speed_byte, DCC_FORWARD);
    old_speed = speed_byte;
  }
  ++count;
}
