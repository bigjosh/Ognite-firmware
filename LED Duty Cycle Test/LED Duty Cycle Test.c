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

		case 3: {
			
			__asm__ __volatile__ (
			"OUT %0,%1 \n\t"				// DO OUTPORT first because in current config it will never actually have both pins so LED can't turn on (not turn of DDRB)
			"RJMP L_%=\n\t"		// waste 2 cycles - use RJMP rather than 2 NOPS because it uses 1/2 the space
			"L_%=:OUT %0,__zero_reg__ \n\t"
			
			: :  "I" (_SFR_IO_ADDR(LED_DUTY_CYCLE_PORT)) , "r" (ledonbits)
			
			);
		}
		break;
		
		default: {
			
				// note that we could make a loop that only uses three cycles, but use 4 makes the math so much easier
			
				// for any delay greater than 3, we have enough cycles to go though a general loop construct
		
				// Loop delay for loop counter c:
								
				// --When c==1---
				//	loop:   DEC c		// 1 cycle  (c=0)
				//      NOP             // 1 cycle 
				//	BRNE loop			// 1 cycle 
				//                      // =============
				//                      // 3 cycles total


				// --When c==2---
				//	loop:   DEC c		// 1 cycle	(c=1)
				//	BRNE loop			// 2 cycles (branch taken)
				//  NOP					// 1 cycle
				//	loop:   DEC c		// 1 cycle	(c=0)
				//	BRNE loop			// 1 cycles (branch not taken)
				//  NOP					// 1 cycle
				//                      // =============
				//                      // 7 cycles total

				
				
				// if c==1 then delay=3 cycles (branch not taken at all)
				// if c==2 then delay=7 (2+4) cycles
				// if c==3 then delay=10 (2+4+4)
								
				// ...so loop overhead is always 3+(c-1)*4 cycles
				
				// Include the single cycle overhead for the trailing OUT after the loop and we get...
				
				// delay = 4+(c-1)*4 
				// delay = 4+(4*c)-4 
				// delay = (4*c) 						
				// delay/4 = c
								
				byte loopcounter = cycles/ (byte) 4;		// Compiler should turn this into a shift operation
			
				byte remainder = cycles & 0x03 ;			// AND with 0x03 will efficiently return the remainder of divide by 4
					
				switch (remainder) {
						
					case 0:		{// No remainder, so just loop and we will get right delay
						
						__asm__ __volatile__ (
						"OUT %[port],%[bits] \n\t"			// DO OUTPORT first because in current config it will never actually have both pins so LED can't turn on (not turn of DDRB)
						"L_%=:dec %[loop]\n\t"			// 1 cycle
						"NOP \n\r"						// 1 cycle in loop
						"BRNE L_%= \n\t"			// 1 on false, 2 on true cycles
						"OUT %[port],__zero_reg__ \n\t"
						
						: [loop] "=r" (loopcounter) : [port] "I" (_SFR_IO_ADDR(LED_DUTY_CYCLE_PORT)) , [bits] "r" (ledonbits) , "0" (loopcounter)
						
						);
					}
					break;
						
					case 1:  {// We need 1 extra cycle to come out with the right delay
						
						__asm__ __volatile__ (
						"OUT %[port],%[bits] \n\t"			// DO OUTPORT first because in current config it will never actually have both pins so LED can't turn on (not turn of DDRB)
						"L_%=:dec %[loop] \n\t"			// 1 cycle
						"NOP \n\r"						// 1 cycle in loop
						"BRNE L_%= \n\t"			// 1 on false, 2 on true cycles
						"NOP \n\r"						// 1 cycle after loop
						"OUT %[port],__zero_reg__ \n\t"
						
						: [loop] "=r" (loopcounter) : [port] "I" (_SFR_IO_ADDR(LED_DUTY_CYCLE_PORT)) , [bits] "r" (ledonbits) , "0" (loopcounter)
						
						);
					}
					break;
						
						
					case 2:  { // We need 2 extra cycles to come out with the right delay
						
						__asm__ __volatile__ (
						
						"OUT %[port],%[bits] \n\t"			// DO OUTPORT first because in current config it will never actually have both pins so LED can't turn on (not turn of DDRB)
						"L_%=:dec %[loop] \n\t"			// 1 cycle
						"NOP \n\r"						// 1 cycle in loop
						"BRNE L_%= \n\t"			// 1 on false, 2 on true cycles
						"RJMP .+0 \r\n"					// 2 cycles after loop (RJMP better than 2 NOPS becuase same time 1/2 the space)
						"OUT %[port],__zero_reg__ \n\t"
						
						: [loop] "=r" (loopcounter) : [port] "I" (_SFR_IO_ADDR(LED_DUTY_CYCLE_PORT)) , [bits] "r" (ledonbits) , "0" (loopcounter)
						
						);
					}
					
					break; 

					case 3:  { // We need 2 extra cycles to come out with the right delay
						
						__asm__ __volatile__ (
						
						"OUT %[port],%[bits] \n\t"			// DO OUTPORT first because in current config it will never actually have both pins so LED can't turn on (not turn of DDRB)
						"L_%=:dec %[loop] \n\t"			// 1 cycle
						"NOP \n\r"						// 1 cycle in loop
						"BRNE L_%= \n\t"			// 1 on false, 2 on true cycles
						"RJMP .+0 \r\n"					// 2 cycles after loop (RJMP better than 2 NOPS becuase same time 1/2 the space)
						"NOP \n\r"						// 1 cycle after loop						
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