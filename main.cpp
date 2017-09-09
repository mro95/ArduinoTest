#include <avr/io.h>
#include <avr/interrupt.h>

bool isOn = false;
uint8_t cnt = 0;

void ledOn() {
    PINB |= (1 << 5);
}

void ledOff() {
    PINB &= ~(1 << 5);
}

ISR(TIMER0_OVF_vect){
    cnt++;
    if (cnt >= 61/2) {
        if (isOn) {
            ledOff();
            isOn = false;
        } else {
            ledOn();
            isOn = true;
        }
        cnt = 0;
    }
}

int main() {
    DDRB |= (1 << 5);

    TCCR0B |= (1 << CS00) | (1 << CS02);

    TCNT0 = 0;

    TIMSK0 |= (1 << TOIE0);

    sei();



    while(true) {};
}

