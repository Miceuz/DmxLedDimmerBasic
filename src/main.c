#include <inttypes.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include "dmx.h"

#define TEST_BUTTON PD6
#define TEST_LED PB1

#define SW_1 PD4
#define SW_2 PD3
#define SW_3 PD2
#define SW_4 PC5
#define SW_5 PC4
#define SW_6 PC3
#define SW_7 PC2
#define SW_8 PC1
#define SW_9 PC0

volatile dmx_t dmx = {IDLE, 1, 0, 0, 0, 0, 0, 255};


void inline initPwm() {
  TCCR1A |= _BV(COM1A1) | _BV(COM1A0);  //Toggle OC1A on Compare Match 
  TCCR1A |= _BV(WGM10); //PWM, Phase Corrected, 8bit
  TCCR1B |= _BV(CS11);    //start timer
}

void inline initUSART() {
  UBRRH = 0;
  UBRRL = 3;
  UCSRB |= _BV(RXEN) | _BV(RXCIE);
  UCSRC |= _BV(URSEL) | _BV(USBS) | _BV(UCSZ1) | _BV(UCSZ0);
}


ISR (USART_RXC_vect) {
  // reading data clears status flags, so read status first
  dmx.status = UCSRA;
  dmx.data = UDR;

//  PORTB &= ~_BV(TEST_LED);
//  _delay_ms(100);

  // data overrun or frame error (break condition)
  if ( dmx.status & _BV(FE)) {
    dmx.state = BREAK;
  } else {
    switch (dmx.state) { // previous slot's state
      case BREAK:
        if (dmx.data != 0) {
          dmx.state = IDLE; // invalid start code
        } else {
          dmx.slot = dmx.address - 1; // skip this many slots
          if (dmx.slot == 0) {
            dmx.state = DATA; // dmx.address == 1
          } else {
            dmx.state = SKIP;
          } 
        }
        break;
      case SKIP:
        if (--dmx.slot == 0) dmx.state = DATA;
        break;
      case DATA:
        if (dmx.chanval[dmx.slot] != dmx.data) {
          dmx.chanval[dmx.slot] = dmx.data;
          dmx.dataisnew |= ((uint16_t)1 << dmx.slot);
        }
        if (++dmx.slot == DMX_CHANNELS) dmx.state = IDLE;
        break;
      case IDLE:
        break;
    }
  }
  PORTB |= _BV(TEST_LED);
}

void inline setupGPIO() {
  DDRB |= _BV(TEST_LED);
  PORTB |= _BV(TEST_LED);
  
  DDRD &= ~_BV(TEST_BUTTON);
  PORTD |= _BV(TEST_BUTTON);
  
  DDRD &= ~_BV(SW_1);
  PORTD |= _BV(SW_1);
  DDRD &= ~_BV(SW_2);
  PORTD |= _BV(SW_2);
  DDRD &= ~_BV(SW_3);
  PORTD |= _BV(SW_3);
  DDRC &= ~_BV(SW_4);
  PORTC |= _BV(SW_4);
  DDRC &= ~_BV(SW_5);
  PORTC |= _BV(SW_5);
  DDRC &= ~_BV(SW_6);
  PORTC |= _BV(SW_6);
  DDRC &= ~_BV(SW_7);
  PORTC |= _BV(SW_7);
  DDRC &= ~_BV(SW_8);
  PORTC |= _BV(SW_8);
  DDRC &= ~_BV(SW_9);
  PORTC |= _BV(SW_9);
}

uint16_t getAddress() {
  uint16_t address = 0;
  
  if(!(PIND & _BV(SW_1))) {
    address |= _BV(0);
  }
  if(!(PIND & _BV(SW_2))) {
    address |= _BV(1);
  }
  if(!(PIND & _BV(SW_3))) {
    address |= _BV(2);
  }
  if(!(PINC & _BV(SW_4))) {
    address |= _BV(3);
  }
  if(!(PINC & _BV(SW_5))) {
    address |= _BV(4);
  }
  if(!(PINC & _BV(SW_6))) {
    address |= _BV(5);
  }
  if(!(PINC & _BV(SW_7))) {
    address |= _BV(6);
  }
  if(!(PINC & _BV(SW_8))) {
    address |= _BV(7);
  }
  if(!(PINC & _BV(SW_9))) {
    address |= _BV(8);
  }
  return address;
}

int main (void) {
  setupGPIO();
  initPwm();
  initUSART();
  dmx.address = getAddress();
  sei();

  OCR1A = 0;
  while(1){
    dmx.address = getAddress();
    if(!(PIND & _BV(TEST_BUTTON))) {
      OCR1A+=10;
      _delay_ms(10);
    } else {
      OCR1A = dmx.chanval[0];
    }
  }
}
