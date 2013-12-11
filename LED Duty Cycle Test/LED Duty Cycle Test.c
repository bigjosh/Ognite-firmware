/*
 * LED_Duty_Cycle_Test.c
 *
 * Created: 11/22/2013 7:29:00 PM
 *  Author: josh
 */ 


// This code shows how to do precise timing down to the single cycle on an AVR
// In this case, it is used to bit-banged PWM on an LED that has no current limiting resistor. 


#include <avr/io.h>

#define F_CPU 8000000UL  // 8 MHz - used by _delay_ms()
 
#include <util/delay.h>


typedef unsigned char byte;		


// Set (ledonbits) to (LED_DUTY_CYCLE_PORT) for (cycles) CPU cycles, then send a zero to the port

#define LED_DUTY_CYCLE_PORT (PORTA)

static inline void ledDutyCycle(unsigned char cycles , byte ledonbits )
{
	
	switch (cycles) {
		
		
		// First, special case out the 0,1, and 2 cycle cases becuase these are so short that we can't do it any other way then hard code
		
		case 0:
		
			break; 
		
		
		case 1:	{	
					
				__asm__ __volatile__ (
				"OUT %0,%1 \n\t"				// DO OUTPORT first because in current config it will never actually have both pins so LED can't turn on (not turn of DDRB)
				"OUT %0,__zero_reg__ \n\t"
					
				: :  "I" (_SFR_IO_ADDR(LED_DUTY_CYCLE_PORT)) , "r" (ledonbits) 
					
				);
			}
			break;
			
		case 2: {
		
				__asm__ __volatile__ (
				"OUT %0,%1 \n\t"				// DO OUTPORT first because in current config it will never actually have both pins so LED can't turn on (not turn of DDRB)
				"NOP \n\t"		// waste one cycle that will (hopefully) not get optimized out
				"OUT %0,__zero_reg__ \n\t"
			
				: :  "I" (_SFR_IO_ADDR(LED_DUTY_CYCLE_PORT)) , "r" (ledonbits) 
			
				);
			}
			break;
		
		default: {
			
				// for any delay greater than 2, we have enough cycles to go though a general loop construct
		
				// Loop delay for loop counter c:
								
				// --When c==1---
				//	loop:   DEC c		// 1 cycle  (c=0)
				//	BRNE loop			// 1 cycle 
				//                      // =============
				//                      // 2 cycles total


				// --When c==2---
				//	loop:   DEC c		// 1 cycle	(c=1)
				//	BRNE loop			// 2 cycles (branch taken)
				//	loop:   DEC c		// 1 cycle	(c=0)
				//	BRNE loop			// 1 cycles (branch not taken)
				//                      // =============
				//                      // 5 cycles total

				
				
				// if c==1 then delay=2 cycles (branch not taken at all)
				// if c==2 then delay=5 (2+3) cycles
				// if c==3 then delay=8 (2+3+3)
								
				// ...so loop overhead is always 2+(c-1)*3 cycles
				
				// Include the single cycle overhead for the trailing OUT after the loop and we get...
				
				// delay = 3+(c-1)*3 
				// delay = 3+(3*c)-3 
				// delay = (3*c) 						
				// delay/3 = c
								
				byte loopcounter = cycles/ (byte) 3;		// TODO: either do faster bit compute here, or store dividends and remainder in lookup 
			
				byte remainder = cycles - (loopcounter*3);			// THis is how many cycles we need to pick up the slack to make up for the granularity of the loop 
					
				switch (remainder) {
						
					case 0:		{// No remainder, so just loop and we will get right delay
						
						__asm__ __volatile__ (
						"OUT %[port],%[bits] \n\t"			// DO OUTPORT first because in current config it will never actually have both pins so LED can't turn on (not turn of DDRB)
						"L_%=:dec %[loop]\n\t"			// 1 cycle
						"BRNE L_%= \n\t"			// 1 on false, 2 on true cycles
						"OUT %[port],__zero_reg__ \n\t"
						
						: [loop] "=r" (loopcounter) : [port] "I" (_SFR_IO_ADDR(LED_DUTY_CYCLE_PORT)) , [bits] "r" (ledonbits) , "0" (loopcounter)
						
						);
					}
					break;
						
					case 1:  {// We need 1 extra cycle to come out with the right delay
						
						__asm__ __volatile__ (
						"OUT %[port],%[bits] \n\t"			// DO OUTPORT first because in current config it will never actually have both pins so LED can't turn on (not turn of DDRB)
						"NOP \n\t"		// waste one cycle that will (hopefully) not get optimized out
						"L_%=:dec %[loop] \n\t"			// 1 cycle
						"BRNE L_%= \n\t"			// 1 on false, 2 on true cycles
						"OUT %[port],__zero_reg__ \n\t"
						
						: [loop] "=r" (loopcounter) : [port] "I" (_SFR_IO_ADDR(LED_DUTY_CYCLE_PORT)) , [bits] "r" (ledonbits) , "0" (loopcounter)
						
						);
					}
					break;
						
						
					case 2:  { // We need 2 extra cycles to come out with the right delay
						
						__asm__ __volatile__ (
						
						"OUT %[port],%[bits] \n\t"			// DO OUTPORT first because in current config it will never actually have both pins so LED can't turn on (not turn of DDRB)
						"RJMP L_%=\n\t"					// Waste 2 cycles using half as much space as 2 NOPs 
						"L_%=:dec %[loop] \n\t"			// 1 cycle
						"BRNE L_%= \n\t"			// 1 on false, 2 on true cycles
						"OUT %[port],__zero_reg__ \n\t"
						
						: [loop] "=r" (loopcounter) : [port] "I" (_SFR_IO_ADDR(LED_DUTY_CYCLE_PORT)) , [bits] "r" (ledonbits) , "0" (loopcounter)
						
						);
					}
					
					break; 
						
				} // switch (remainder)
		
		}	// default case where b>2
		
		break;
		
	}	// switch (b)
				
}
					


void ledBrightnessLoop(void) {
	
	PORTD = 0x01;		// Trigger Scope
			
	for(byte x=0;x<255;x++) {
		ledDutyCycle(x,0x01);		//1-255						
	}
	
	PORTD = 0x00;		// Trigger Scope

	for(byte x=0;x<255;x++) {
		ledDutyCycle(x,0x01);		//1-255
	}


}


int main(void) {
		
	CLKPR = _BV(CLKPCE);				// Enable changes to the clock prescaler
	CLKPR = 0;							// Set prescaler to 1, we will run full speed

	// Enable output on PORTA0
	DDRD = 0x01; 
	DDRA = 0x03;

	while(1) ledBrightnessLoop();

}