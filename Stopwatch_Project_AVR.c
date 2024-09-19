#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// Variables to keep track of time and mode
unsigned char seconds = 0, minutes = 0, hours = 0;
unsigned char countup = 0;  // Mode flag (0 for increment, 1 for countdown)

// Initialize Timer1 in CTC mode for generating 1-second intervals
void Timer1_ctc_init() {
    SREG |= (1 << 7);                 // Enable global interrupts
    TCCR1A = (1 << FOC1A);            // Force Output Compare for Timer1 (non-PWM mode)
    TCCR1B = (1 << WGM12) | (1 << CS10) | (1 << CS12);  // CTC mode with prescaler 1024
    TCNT1 = 0;                        // Initialize counter to 0
    OCR1A = 15625;                    // Compare value for 1-second interval
    TIMSK |= (1 << OCIE1A);           // Enable Timer1 compare interrupt
}

// Increment or decrement time based on the current mode
void Timer_inc_and_dec() {
	if (!countup) {
	    // Count Up Mode

	    // Turn off the buzzer
	    PORTD &= ~(1 << PD0);

	    // Turn on the red LED for count-up indication
	    PORTD |= (1 << PD4);

	    // Turn off the yellow LED for count-down indication
	    PORTD &= ~(1 << PD5);

	    // Increment seconds
	    seconds++;

	    // Check if seconds have reached 60 (1 minute)
	    if (seconds == 60) {
	        minutes++;     // Increment minutes
	        seconds = 0;   // Reset seconds to 0
	    }

	    // Check if minutes have reached 60 (1 hour)
	    if (minutes == 60) {
	        hours++;       // Increment hours
	        minutes = 0;   // Reset minutes to 0
	    }

	    // Check if hours have reached 100 (reset condition)
	    if (hours == 100) {
	        hours = 0;     // Reset hours to 0
	        minutes = 0;   // Reset minutes to 0
	        seconds = 0;   // Reset seconds to 0
	    }
	} else {
	    // Count Down Mode

	    // Turn off the red LED for count-up indication
	    PORTD &= ~(1 << PD4);

	    // Turn on the yellow LED for count-down indication
	    PORTD |= (1 << PD5);

	    // Check if seconds are 0
	    if (seconds == 0) {
	        // Check if minutes are also 0
	        if (minutes == 0) {
	            // Check if hours are also 0
	            if (hours == 0) {
	                // If all values are 0, turn on the buzzer
	                PORTD |= (1 << PD0);
	            } else {
	                // Decrement hours and reset minutes and seconds
	                hours--;
	                minutes = 59;
	                seconds = 59;
	            }
	        } else {
	            // Decrement minutes and reset seconds
	            minutes--;
	            seconds = 59;
	        }
	    } else {
	        // Decrement seconds
	        seconds--;
	    }
	}

}

// Timer1 compare interrupt service routine
ISR(TIMER1_COMPA_vect) {
    Timer_inc_and_dec();
}

// Initialize external interrupt 0 for the reset button
void INT0_Init()
{
	MCUCR |= 1<<ISC01 ;  /* interrupt to be triggered with falling edge */
	GICR  |= 1<<INT0  ; /* module interrupt Enable */
}

// External interrupt 0 service routine (resets the stopwatch)
ISR(INT0_vect)
{
    hours = 0;
    minutes = 0;
    seconds = 0;
    PORTD &= ~(1 << PD0);  // Turn off alarm/buzzer
}

// Initialize external interrupt 1 for the pause button
void INT1_Init() {
    MCUCR |= (1 << ISC11) | (1 << ISC10); // Interrupt on rising edge
    GICR |= (1 << INT1);   // Enable external interrupt 1
    DDRD &= ~(1 << PD3);  // PD3 as input (pause button)
}

// External interrupt 1 service routine (pauses Timer1)
ISR(INT1_vect) {
    TCCR1B &= ~(1 << CS12) & ~(1 << CS11) & ~(1 << CS10); // Stop Timer1
}

// Initialize external interrupt 2 for the resume button
void INT2_Init() {
    MCUCSR &= ~(1 << ISC2); // Interrupt on falling edge
    GICR |= (1 << INT2);    // Enable external interrupt 2
    DDRB &= ~(1 << PB2);    // PB2 as input (resume button)
    PORTB |= (1 << PB2);    // Enable pull-up resistor on PB2
}

// External interrupt 2 service routine (resumes Timer1)
ISR(INT2_vect) {
    TCCR1B |= (1 << WGM12) | (1 << CS10) | (1 << CS12); // Start Timer1 with CTC mode and prescaler 1024
}


// Increment hours (used for adjusting time)
void increment_hour(void) {
    if (hours < 99) {  // Adjust the maximum number of hours as needed
        ++hours;
    } else {
        hours = 0;  // Reset hours if they roll over
    }
}

// Decrement hours (only if hours > 0)
void decrement_hour(void) {
    if (hours > 0) {
        --hours;
    }
}

void increment_min(void) {
    if (minutes < 59) {
        ++minutes;
    } else {
        minutes = 0;
        increment_hour();  // Increment hours if minutes roll over
    }
}

// Decrement minutes (only if minutes > 0)
void decrement_min(void) {
    if (minutes > 0) {
        --minutes;
    } else if (hours > 0) {
        minutes = 59;
        decrement_hour();  // Decrement hours if minutes roll over
    }
}

void increment_sec(void) {
    if (seconds < 59) {
        ++seconds;
    } else {
        seconds = 0;
        increment_min();  // Increment minutes if seconds roll over
    }
}

// Decrement seconds (only if seconds > 0)
void decrement_sec(void) {
    if (seconds > 0) {
        --seconds;
    } else if (minutes > 0) {
        seconds = 59;
        decrement_min();  // Decrement minutes if seconds roll over
    }
}


// Display seconds, minutes, and hours on 7-segment displays
void DisplaySevenSegment() {
    // Display seconds
    PORTA = (PORTA & 0xC0) | (1 << 5); // Enable first 7-segment display
    PORTC = (PORTC & 0xF0) | (seconds % 10); // Ones place of seconds
    _delay_ms(2);

    PORTA = (PORTA & 0xC0) | (1 << 4); // Enable second 7-segment display
    PORTC = (PORTC & 0xF0) | (seconds / 10); // Tens place of seconds
    _delay_ms(2);

    // Display minutes
    PORTA = (PORTA & 0xC0) | (1 << 3); // Enable third 7-segment display
    PORTC = (PORTC & 0xF0) | (minutes % 10); // Ones place of minutes
    _delay_ms(2);

    PORTA = (PORTA & 0xC0) | (1 << 2); // Enable fourth 7-segment display
    PORTC = (PORTC & 0xF0) | (minutes / 10); // Tens place of minutes
    _delay_ms(2);

    // Display hours
    PORTA = (PORTA & 0xC0) | (1 << 1); // Enable fifth 7-segment display
    PORTC = (PORTC & 0xF0) | (hours % 10); // Ones place of hours
    _delay_ms(2);

    PORTA = (PORTA & 0xC0) | (1 << 0); // Enable sixth 7-segment display
    PORTC = (PORTC & 0xF0) | (hours / 10); // Tens place of hours
    _delay_ms(2);
}



int main(void) {
    // Initialization of ports and interrupts
    DDRD &= ~(1 << PD2);  // PD2 as input (reset button)
    PORTD |= 1<<PD2 ; /* enable internal pull up */
    DDRD &= ~(1 << PD3);  // PD3 as input (pause button)
    DDRB = 0;             // PORTB as input (buttons)
    PORTB = 255;          // Enable pull-ups on PORTB (buttons)

    DDRD |= (1 << PD0) | (1 << PD4) | (1 << PD5);  // PD0 (alarm/buzzer), PD4 (counting up LED), PD5 (counting down LED) as output
    DDRC |= (1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC3);  // PC0-PC3 as output (7-segment displays)
    PORTC &= ~((1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC3));  // Initialize PORTC
    DDRA |= (1 << PA0) | (1 << PA1) | (1 << PA2) | (1 << PA3) | (1 << PA4) | (1 << PA5);  // PA0-PA5 as output (7-segment display control)


    PORTD |= (1 << PD4);  // Turn on counting up LED
    PORTD &= ~(1 << PD5);  // Ensure counting down LED is off

    Timer1_ctc_init() ; // Initialize Timer1 and interrupts
    INT0_Init();  // Initialize external interrupt 0 (reset button)
    INT1_Init();  // Initialize external interrupt 1 (pause button)
    INT2_Init();  // Initialize external interrupt 2 (resume button)

	unsigned char Toggle_flag = 1;
	unsigned char inc_sec_flag = 1, dec_sec_flag = 1;
	unsigned char inc_min_flag = 1, dec_min_flag = 1;
	unsigned char inc_hour_flag = 1, dec_hour_flag = 1;


    while (1)
    {


        // Handle mode toggle (increment/countdown) when PB7 is pressed
        if (!(PINB & (1 << PB7))) {  // Check if mode toggle button (PB7) is pressed
            _delay_ms(2);  // Debouncing delay to avoid multiple triggers
            if (!(PINB & (1 << PB7)) && Toggle_flag) {  // Check if button is still pressed after debounce
                countup = countup ^ 1;  // Toggle the counting mode between increment (0) and countdown (1)
                Toggle_flag = 0;  // Reset toggle flag to prevent continuous toggling
            }
        } else {
            Toggle_flag = 1;  // Reset toggle flag when button is released
        }

        // Increment seconds when PB6 is pressed
        if (!(PINB & (1 << PB6))) {  // Check if increment seconds button (PB6) is pressed
            _delay_ms(2);  // Debouncing delay
            if (!(PINB & (1 << PB6))) {  // Check if button is still pressed after debounce
                if (inc_sec_flag) {  // Check if button was previously released
                    inc_sec_flag = 0;  // Reset flag after pressing
                    increment_sec();  // Increment seconds
                }
            }
        } else {
            inc_sec_flag = 1;  // Reset flag when button is released
        }

        // Decrement seconds when PB5 is pressed
        if (!(PINB & (1 << PB5))) {  // Check if decrement seconds button (PB5) is pressed
            _delay_ms(2);  // Debouncing delay
            if (!(PINB & (1 << PB5))) {  // Check if button is still pressed after debounce
                if (dec_sec_flag) {  // Check if button was previously released
                    dec_sec_flag = 0;  // Reset flag after pressing
                    decrement_sec();  // Decrement seconds
                }
            }
        } else {
            dec_sec_flag = 1;  // Reset flag when button is released
        }

        // Increment minutes when PB4 is pressed
        if (!(PINB & (1 << PB4))) {  // Check if increment minutes button (PB4) is pressed
            _delay_ms(2);  // Debouncing delay
            if (!(PINB & (1 << PB4))) {  // Check if button is still pressed after debounce
                if (inc_min_flag) {  // Check if button was previously released
                    inc_min_flag = 0;  // Reset flag after pressing
                    increment_min();  // Increment minutes
                }
            }
        } else {
            inc_min_flag = 1;  // Reset flag when button is released
        }

        // Decrement minutes when PB3 is pressed
        if (!(PINB & (1 << PB3))) {  // Check if decrement minutes button (PB3) is pressed
            _delay_ms(2);  // Debouncing delay
            if (!(PINB & (1 << PB3))) {  // Check if button is still pressed after debounce
                if (dec_min_flag) {  // Check if button was previously released
                    dec_min_flag = 0;  // Reset flag after pressing
                    decrement_min();  // Decrement minutes
                }
            }
        } else {
            dec_min_flag = 1;  // Reset flag when button is released
        }

        // Increment hours when PB1 is pressed
        if (!(PINB & (1 << PB1))) {  // Check if increment hours button (PB1) is pressed
            _delay_ms(2);  // Debouncing delay
            if (!(PINB & (1 << PB1))) {  // Check if button is still pressed after debounce
                if (inc_hour_flag) {  // Check if button was previously released
                    inc_hour_flag = 0;  // Reset flag after pressing
                    increment_hour();  // Increment hours
                }
            }
        } else {
            inc_hour_flag = 1;  // Reset flag when button is released
        }

        // Decrement hours when PB0 is pressed
        if (!(PINB & (1 << PB0))) {  // Check if decrement hours button (PB0) is pressed
            _delay_ms(2);  // Debouncing delay
            if (!(PINB & (1 << PB0))) {  // Check if button is still pressed after debounce
                if (dec_hour_flag) {  // Check if button was previously released
                    dec_hour_flag = 0;  // Reset flag after pressing
                    decrement_hour();  // Decrement hours
                }
            }
        } else {
            dec_hour_flag = 1;  // Reset flag when button is released
        }


        DisplaySevenSegment();

    }

}
