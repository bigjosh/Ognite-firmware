#define FRAME_RATE 15				// Frames per second

#define	WIDTH 	5
#define	HEIGHT	8

#define BRIGHTNESSBITS 5

typedef byte framecounttype;		// Define this type in case we ever go over 255 frames we can switch to an unsigned int

#define	FRAMECOUNT	((framecounttype) 195)

extern byte PROGMEM const videobitstream[];
