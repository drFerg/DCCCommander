#include "Arduino.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include "DCCHardware.h"

/// An enumerated type for keeping track of the state machine used in the timer1 ISR
/** Given the structure of a DCC packet, the ISR can be in one of 5 states.
      *DCC_IDLE: there is nothing to put on the rails. In this case, the only legal thing
                 to do is to put a '1' on the rails.  The ISR should almost never be in this state.
      *DCC_PREAMBLE: A packet has been made available, and so we should broadcast the preamble: 14 '1's in a row
      *DCC_BYTE_START: Each data uint8_t is preceded by a '0'
      *DCC_BYTE_SEND: Sending the current data uint8_t
      *DCC_POSTAMBLE: After the final uint8_t is sent, send a '0'.
*/

//On Arduino UNO, etc, OC1A is digital pin 9, or Port B/Pin 1
//On Arduino MEGA, etc, OC1A is digital pin 11, or Port B/Pin 5
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_AT90CAN128__) || defined(__AVR_AT90CAN64__) || defined(__AVR_AT90CAN32__)
  #define PINBN PINB6 
#else
  #define PINBN PINB1
#endif

#define PREAMBLE_BIT_CNT 14
#define BYTE_BIT_CNT 8
#define current_bit() ((pkt[pkt_size - byte_counter]) >> (bit_counter - 1))

/* Whenever we set OCR1A, we must also set OCR1B, or else OC1B will get out of phase with OC1A! */
#define send_bit(bit) OCR1A = OCR1B = bit 

typedef enum {
  ONE_BIT = 115, /* 58us */
  ZERO_BIT = 199 /* 100us */
} BITS;

typedef enum  {
  DCC_IDLE,
  DCC_PREAMBLE,
  DCC_BYTE_START,
  DCC_BYTE_SEND,
  DCC_POSTAMBLE
} DCC_output_state_t;

DCC_output_state_t DCC_state = DCC_IDLE; //just to start out

/// The currently queued packet to be put on the rails. Default is a reset packet.
uint8_t pkt[6] = {0, 0, 0, 0, 0, 0};
/// How many data uint8_ts in the queued packet?
volatile uint8_t pkt_size = 0;
/// How many uint8_ts remain to be put on the rails?
volatile uint8_t byte_counter = 0;
/// How many bits remain in the current data uint8_t/preamble before changing states?
volatile uint8_t bit_counter = PREAMBLE_BIT_CNT; //init to 14 1's for the preamble
/// A fixed-content packet to send when idle
//uint8_t DCC_Idle_Packet[3] = {255,0,255};
/// A fixed-content packet to send to reset all decoders on layout
//uint8_t DCC_Reset_Packet[3] = {0,0,0};


/// Timer1 TOP values for one and zero
/** S 9.1 A specifies that '1's are represented by a square wave with a half-period of 58us (valid range: 55-61us)
    and '0's with a half-period of >100us (valid range: 95-9900us)
    Because '0's are stretched to provide DC power to non-DCC locos, we need two zero counters,
     one for the top half, and one for the bottom half.

   Here is how to calculate the timer1 counter values (from ATMega168 datasheet, 15.9.2):
 f_{OC1A} = \frac{f_{clk_I/O}}{2*N*(1+OCR1A)})
 where N = prescalar, and OCR1A is the TOP we need to calculate.
 We know the desired half period for each case, 58us and >100us.
 So:
 for ones:
 58us = (8*(1+OCR1A)) / (16MHz)
 58us * 16MHz = 8*(1+OCR1A)
 58us * 2MHz = 1+OCR1A
 OCR1A = 115

 for zeros:
 100us * 2MHz = 1+OCR1A
 OCR1A = 199
 
 Thus, we also know that the valid range for stretched-zero operation is something like this:
 9900us = (8*(1+OCR1A)) / (16MHz)
 9900us * 2MHz = 1+OCR1A
 OCR1A = 19799
*/



uint16_t one_count = 115; //58us
uint16_t zero_high_count = 199; //100us
uint16_t zero_low_count = 199; //100us

/// Setup phase: configure and enable timer1 CTC interrupt, set OC1A and OC1B to toggle on CTC
void setup_DCC_waveform_generator() {
  
 //Set the OC1A and OC1B pins (Timer1 output pins A and B) to output mode
 //On Arduino UNO, etc, OC1A is Port B/Pin 1 and OC1B Port B/Pin 2
 //On Arduino MEGA, etc, OC1A is or Port B/Pin 5 and OC1B Port B/Pin 6
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_AT90CAN128__) || defined(__AVR_AT90CAN64__) || defined(__AVR_AT90CAN32__)
  DDRB |= (1 << DDB5) | (1 << DDB6);
#else
  DDRB |= (1 << DDB1) | (1 << DDB2);
#endif

  // Configure timer1 in CTC mode, for waveform generation, set to toggle OC1A, OC1B, at /8 prescalar, interupt at CTC
  TCCR1A = (0 << COM1A1) | (1 << COM1A0) | (0 << COM1B1) | (1 << COM1B0) | (0 << WGM11) | (0 << WGM10);
  TCCR1B = (0 << ICNC1)  | (0 << ICES1)  | (0 << WGM13)  | (1 << WGM12)  | (0 << CS12)  | (1 << CS11) | (0 << CS10);

  // start by outputting a '1'
  send_bit(ONE_BIT);
  TCNT1 = 0; //get the timer rolling (not really necessary? defaults to 0. Just in case.)
    
  //finally, force a toggle on OC1B so that pin OC1B will always complement pin OC1A
  TCCR1C |= (1 << FOC1B);

}

void DCC_waveform_generation_hasshin() {
  //enable the compare match interrupt
  TIMSK1 |= (1 << OCIE1A);
}

/// This is the Interrupt Service Routine (ISR) for Timer1 compare match.
ISR(TIMER1_COMPA_vect) {
  //in CTC mode, timer TCINT1 automatically resets to 0 when it matches OCR1A. Depending on the next bit to output,
  //we may have to alter the value in OCR1A, maybe.
  //to switch between "one" waveform and "zero" waveform, we assign a value to OCR1A.
  
  //remember, anything we set for OCR1A takes effect IMMEDIATELY, so we are working within the cycle we are setting.
  //first, check to see if we're in the second half of a byte; only act on the first half of a uint8_t

  if(PINB & (1 << PINBN)) { /* if the pin is low, we need to use a different zero counter to enable stretched-zero DC operation */
    if(OCR1A == ZERO_BIT) { //if the pin is low and outputting a zero, we need to be using zero_low_count
      send_bit(ZERO_BIT);
    }
  }
  else //the pin is high. New cycle is begining. Here's where the real work goes.
  {
    switch(DCC_state) {
      case DCC_IDLE: /* Check if a new packet is ready, then send preamble, else send 1 bit. */
        if(byte_counter == 0) { /*if no new packet */
          send_bit(ONE_BIT); /* Send ones if we don't know what else to do. */
          break;
        }
        DCC_state = DCC_PREAMBLE; //and fall through to DCC_PREAMBLE
      case DCC_PREAMBLE: /* Sending preamble bits(14) before switching to DCC_BYTE_START */
        send_bit(ONE_BIT);
        if(--bit_counter == 0) DCC_state = DCC_BYTE_START;
        break;
      case DCC_BYTE_START: /* About to send a byte, proceed the data with a 0, then move to DCC_BYTE_SEND */
        send_bit(ZERO_BIT);
        DCC_state = DCC_BYTE_SEND;
        bit_counter = BYTE_BIT_CNT;
        break;
      case DCC_BYTE_SEND: /* Sending a byte; current bit is tracked with bit_counter, and current byte with byte_counter */
        if(current_bit() & 1) /* Is current bit a 1? */
          send_bit(ONE_BIT);
        else /* It's a 0 */
          send_bit(ZERO_BIT);

        if(--bit_counter == 0) { /* If out of bits, send the next byte */
          if(--byte_counter == 0) /* If out of bytes, packet is sent, move to DCC_POSTAMBLE */
            DCC_state = DCC_POSTAMBLE;
          else /* More bytes left to send, back to DCC_BYTE_START */
            DCC_state = DCC_BYTE_START;
        }
        break;
      case DCC_POSTAMBLE: // Done with the packet. Send out a final '1', then head back to DCC_IDLE to check for a new packet.
        send_bit(ONE_BIT);
        DCC_state = DCC_IDLE;
        bit_counter = PREAMBLE_BIT_CNT; //in preparation for a preamble...
        break;
    }
  }
}
