/*
 * Implement the following routines to set GPIO pins to input or output,
 * and to read (input) and write (output) them.
 *
 * DO NOT USE loads and stores directly: only use GET32 and PUT32 
 * to read and write memory.  Use the minimal number of such calls.
 *
 * See rpi.h in this directory for the definitions.
 */
#include "rpi.h"

// see broadcomm documents for magic addresses.
#define GPIO_BASE 0x20200000
static const unsigned gpio_set0  = (GPIO_BASE + 0x1C);
static const unsigned gpio_clr0  = (GPIO_BASE + 0x28);
static const unsigned gpio_lev0  = (GPIO_BASE + 0x34);

// set <pin> to be an output pin.  
//
// note: fsel0, fsel1, fsel2 are contiguous in memory, so you 
// can (and should) use array calculations!
void gpio_set_output(unsigned pin) {
	if (pin > 32) {return;}
	/* find which GPFSELn word to write to for pin n
	 * each  word controls 10 pins
	 * so GPFSEL0 controls pins 0-9, etc 
	 */
	const unsigned GPFSELN = pin / 10;
	uint32_t addr = (GPIO_BASE + (4 * GPFSELN));
	/* call GET32 to avoid the C compiler optimizing out our loads and stores
	 * to device memory 
	 */
	uint32_t addr_val = GET32(addr);
	/* set proper bit to be output
	 * done by setting a sepcific bit within the addr word to 1 
	 */
	const unsigned FSELN = pin % 10;
	/* want the pins 3 bits to be 001
	 * since we may be setting a pin in the middle of
	 * our word, we should shift by multiples of 3
	 * depending on where our pin lives in the word
	 */
	addr_val &= ~(0b111 << (3 * FSELN)); //clear bits we want to set
	addr_val |= (1 << (3 * FSELN));
	PUT32(addr, addr_val);
}

// set GPIO <pin> on.
void gpio_set_on(unsigned pin) {
	//set bit 'pin' to 1 within gpio_set0
	PUT32(gpio_set0, 1 << pin);
}

// set GPIO <pin> off
void gpio_set_off(unsigned pin) {
	//set bit 'pin; to 1 withinn gpio_clr0
	PUT32(gpio_clr0, 1 << pin);
}

// set <pin> to <v> (v \in {0,1})
void gpio_write(unsigned pin, unsigned v) {
    if(v)
       gpio_set_on(pin);
    else
       gpio_set_off(pin);
}


// set <pin> to input.
void gpio_set_input(unsigned pin) {
	if (pin > 32) {return;}
	/* find which GPFSELn word to write to for pin 'n'
	 * each  word controls 10 pins
	 * so GPFSEL0 controls pins 0-9, 
	 * GPFSEL1 controls pins 10-19, etc 
	 */
	const unsigned GPFSELN = pin / 10;
	uint32_t addr = (GPIO_BASE + (4 * GPFSELN));
	/* call GET32 to avoid the C compiler optimizing out our loads and stores
	 * to device memory 
	 */
	uint32_t addr_val = GET32(addr);
	/* set proper bit to be output
	 * done by setting a sepcific bit within the addr word to 1 
	 */
	const unsigned FSELN = pin % 10;
	/* want the pins 3 bits to be 000
	 * since we may be setting a pin in the middle of
	 * our word, we should shift by multiples of 3
	 * depending on where our pin lives in the word
	 */
	addr_val &= ~(0b111 << (3 * FSELN)); //clear bits we want to set
	addr_val |= (0 << (3 * FSELN));
	PUT32(addr, addr_val);
}

// return the value of <pin>
int gpio_read(unsigned pin) {
	/* read in the value from gpio_lev0
	 * Then, only return the value of the bit
	 * that corresponds to 'pin' within the word.
	 * ex: pin 9 is the 9th bit of the word read
	 * by calling GET32() on gpio_lev0
	 */
    unsigned v = GET32(gpio_lev0);;
	v >>= pin;
	v &= 1;
    return v;
}
