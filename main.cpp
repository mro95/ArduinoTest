#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <avr/sfr_defs.h>
#define F_CPU 16E6
#include <util/delay.h>
// output on USB = PD1 = board pin 1
// datasheet p.190; F_OSC = 16 MHz & baud rate = 19.200
#define UBBRVAL 51

//thresholds
uint8_t maxLight = 0xAA;
uint8_t minTemp = 0x95;

// false = uitgerold/closed; true = ingerold/open.
bool status_blinds = false;
bool new_status_blinds = false;

void uart_init()
{
    // set the baud rate
    UBRR0H = 0;
    UBRR0L = UBBRVAL;
    // disable U2X mode
    UCSR0A = 0;
    // enable transmitter
    UCSR0B = _BV(TXEN0);
    // set frame format : asynchronous, 8 data bits, 1 stop bit, no parity
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
}

void transmit(uint8_t data)
{
    // wait for an empty transmit buffer
    // UDRE is set when the transmit buffer is empty
    loop_until_bit_is_set(UCSR0A, UDRE0);
    // send the data
    UDR0 = data;
}

uint8_t readAnalog(uint8_t ch)
{
    // select the corresponding channel 0~7
    // ANDing with ’7′ will always keep the value
    // of ‘ch’ between 0 and 7
    ch &= 0b00000111;  // AND operation with 7
    ADMUX = (ADMUX & 0xF8)|ch; // clears the bottom 3 bits before ORing
 
    // start single convertion
    // write ’1′ to ADSC
    ADCSRA |= (1<<ADSC);
 
    // wait for conversion to complete
    // ADSC becomes ’0′ again
    // till then, run loop continuously
    loop_until_bit_is_clear(ADCSRA, ADSC);
 
    return (ADC);
}


inline void ledOn(uint8_t pin)
{
    PORTD |= (1<<pin);
}

inline void ledOff(uint8_t pin)
{
    PORTD &= ~(1<<pin);
}

inline void ledToggle(uint8_t pin)
{
    PORTD ^= (1<<pin);
}

inline void initLedsOn()
{
    ledOn(2);
    ledOn(3);
    ledOn(4);
}

inline void initLedsOff()
{
    ledOff(2);
    ledOff(3);
    ledOff(4);
}

void setup()
{
    // Set PORTD as output
    DDRD = 0xFF;

    // Turn on all leds to communicate it is starting up
    initLedsOn();

    // Setup uart
    uart_init();

    // Wait 1 second for setup uart
    _delay_ms(1000);

    // AREF = AVcc and left ajust ADC
    //ADMUX = (1<<REFS0) | (1<<ADLAR);
    ADMUX = (1<<REFS0);

    // ADC Enable and prescaler of 128
    // 16000000/128 = 125000
    ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);


    // Enables timer 0 and 2
    TCCR0B |= (1 << CS00) | (1 << CS02);
    TCCR2B |= (1 << CS22) | (1 << CS21) | (1 << CS20);

    TCNT0 = 0;
    TCNT2 = 0;

    TIMSK0 |= (1<<TOIE0);
    TIMSK2 |= (1<<TOIE2);

    sei();
    
    // Turn off all leds
    initLedsOff();
}


uint8_t light_cnt = 50;
uint8_t temp_cnt = 0;
ISR(TIMER0_OVF_vect) 
{
    if (light_cnt > 125) {
        uint8_t light = readAnalog(1);
        transmit(0x02); 
        transmit(light);
        light_cnt = 0;
    } else {
        light_cnt++;
    }
    if (temp_cnt > 125) {
        uint8_t temp = readAnalog(0);
        transmit(0x01); 
        transmit(temp); 
        temp_cnt = 0;
    } else {
        temp_cnt++;
    }
}

uint8_t ledy_cnt = 0;
bool ledy_blink = false;
bool busy = false;
uint16_t check_timer1 = 0;
uint8_t motor_timer = 0;
const uint16_t MOTOR_TIM = 480;

// sensors
uint8_t temp = 0;
uint8_t light = 0;

ISR(TIMER2_OVF_vect) 
{
    // Blink the yellow led
    if (ledy_blink == true) {
        if (ledy_cnt > 15) {
            ledToggle(3);
            ledy_cnt = 1;
        }
        ledy_cnt++;
    }

    // Decrement motor variable if not 0
    if (motor_timer > 0) {
        motor_timer--;
    } else {
        if (busy == true) {
            // if motor variable is finished
            // set variables back to normal
            ledOff(3);
            ledy_blink = false;
            busy = false;
            status_blinds = new_status_blinds;
        }
    }

    // if not busy
    if (busy == false) {
        // check sensors every 8 seconds
        if (check_timer1 >= 20) {
            temp = readAnalog(0);
            light = readAnalog(1);

            //transmit(light);
            // Check if blinds have to close
            if (light > maxLight) {
                // do nothing if blinds are already closed
                // else close the blinds
                if (status_blinds == true) {
                    ledOff(2);
                    ledOn(4);
                    motor_timer = MOTOR_TIM;
                    ledy_blink = true;
                    busy = true;
                    new_status_blinds = false;
                }
            } else {
                if (status_blinds == false) {
                    ledOff(4);
                    ledOn(2);
                    motor_timer = MOTOR_TIM;
                    ledy_blink = true;
                    busy = true;
                    new_status_blinds = true;
                }
            }
            check_timer1 = 0;
        } else {
            check_timer1++;
        }
    }
}

int main(void) 
{
    setup();
    while (1) {
    }
}

