

/// Just a place to temporarally hold some code snippets. Not included in build.

//note that it is ok to skip setting up the zero reg (init2) and loading variables (init4) on warm boot becuase all
//registers and SRAM are saved intact though a reset



// We are not including startup files to save space (linker option -nostartfiles)
// This means we need to provide our own VECTOR table. Since we don't do *any* interrupts
// we actually don't need vectors at all. Our linker script will put our .init section
// right at the very beginning of the code space (address 0) so we will just start executing from there.


// Ok, if we get here then it is a cold start so we need to get everything initialized


void init2 (void) __attribute__ ((naked)) __attribute__ ((section (".init2")));

void init2(void) {
	
	asm("clr __zero_reg__");			// Clear the designated zero register to zero becuace C code depends on this zero always being there
	
}


// This is our main(). It is run on normal powerup and by the time we get here, all the golbal variables are initialized and we are ready to rock and Roll and set up the watch dog

void init9(void) __attribute__ ((naked)) __attribute__ ((section (".init9")));

void init9(void) {
	
	setup();					// do some hardware setup
	
	wdt_enable( WDTO_15MS );		// Turn on the watch dog time, set it to wake us up in 15ms (about 62hrz)
	
	#ifdef TIMECHECK
	PORTA |= _BV(0);
	#endif
	for(byte x=0;x<254;x++) {NOP;NOP;NOP;}
	#ifdef TIMECHECK
	PORTA &= ~_BV(0);
	#endif
	
	asm("sleep");				// Good night!
	
}






Do any of these power savings efforts do anything? Are the worth the extra memory and time to do on each wake?

static inline void setup() {
	

	// On boot, clock prescaler will be 8. Lets set it to 1 to get full speed
	// Note that no interrupts shouldnt be on yet so we'll be sure to hit the second step in time (you only get 4 cycles).
	
	// TODO: Commenting this out for now, need testing to see if it actually is better to go faster..
	
	//CLKPR = (1 << CLKPCE); // enable a change to CLKPR
	//CLKPR = 0; // set the CLKDIV to 0 - was 0011b = div by 8
	// TODO: we could do this in a fuse bit and save a couple of bytes
	
	ACSR |= ACD;		// Turn off analog compare unit. We don't use it, so save power. Saves about 0.1ma   3.6mA drops to  3.5 mA
	
	PRR = PRTIM0 | PRUSI | PRUSART;		// Turn off Timer/Counter0, USI, USART since we don't need them. Saves about 0.1mA.
	
	set_sleep_mode( SLEEP_MODE_PWR_DOWN );	// Sleep as deep as we can - will still wake on WDT reset
	
	sleep_enable();			// Allow sleep instruction to work
	
	// TODO: These two sllep register lines could be condensed to much shorter ASM

	#ifdef TIMECHECK
	DDRA = 0xff;		// make PORTA bits output so that we can see timeing patterns
	#endif
	
}


