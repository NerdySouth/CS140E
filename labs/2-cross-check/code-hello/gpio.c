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
static const unsigned gppud = (GPIO_BASE + 0x94);
static const unsigned gppud_clk0 = (GPIO_BASE + 0x98);
static const unsigned gppud_clk1 = (GPIO_BASE + 0x9c);


// set GPIO <pin> on.
void gpio_set_on(unsigned pin) {
	if(pin >= 32 || pin == 47) { return; }
	//set bit 'pin' to 1 within gpio_set0
	PUT32(gpio_set0, 1 << pin);
}

// set GPIO <pin> off
void gpio_set_off(unsigned pin) {
	if(pin >= 32 || pin == 47) { return; }
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


void gpio_set_output(unsigned pin) {
	gpio_set_function(pin, GPIO_FUNC_OUTPUT);
}



// set <pin> to input.
void gpio_set_input(unsigned pin) {
	gpio_set_function(pin, GPIO_FUNC_INPUT);
}

// return the value of <pin>
int gpio_read(unsigned pin) {
	if (pin >= 32 && pin != 47) { return DEV_VAL32(-1); }
	/* read in the value from gpio_lev0
	 * Then, only return the value of the bit
	 * that corresponds to 'pin' within the word.
	 * ex: pin 9 is the 9th bit of the word read
	 * by calling GET32() on gpio_lev0
	 */
    unsigned v = GET32(gpio_lev0);
	v >>= pin;
	v &= 1;
    return DEV_VAL32(v);
}

void gpio_set_pullup(unsigned pin) {
	//mem barrier at start
	nop();
	//step 1 in broadcom docs: write 10 to gppud to set pullup
	PUT32(gppud, 1 << 1);
	//step 2: delay for 150usec, or multiple of 150 clock cycles until i figure out how to delay
	//by microseconds
	delay(300);
	//step 3: write to gppud_clk0/1 depending which pin we want to set for
	// _clk0 controls pins 0-31, _clk1 controls pins 32-53
	const unsigned clock_reg = (pin < 32) ? gppud_clk0 : gppud_clk1;
	PUT32(clock_reg, 1 << pin);
	//step 4: wait again
	delay(300);
	//step 5 gets skipped, straight to step 6
	PUT32(clock_reg, 1 << pin);
	//mem barrrier at  end
	nop();
}

void gpio_set_pulldown(unsigned pin) {
	//mem barrier at start
	nop();
	//step 1 in broadcom docs: write 01 to gppud to set pulldown
	PUT32(gppud, 0b1);
	//step 2: delay for 150usec, or multiple of 150 clock cycles until i figure out how to delay
	//by microseconds
	delay(300);
	//step 3: write to gppud_clk0/1 depending which pin we want to set for
	// _clk0 controls pins 0-31, _clk1 controls pins 32-53
	const unsigned clock_reg = (pin < 32) ? gppud_clk0 : gppud_clk1;
	PUT32(clock_reg, 1 << pin);
	//step 4: wait again
	delay(300);
	//step 5 gets skipped, straight to step 6
	PUT32(clock_reg, 1 << pin);
	//mem barrrier at  end
	nop();
}

// set <pin> to be an output pin.  
//
// note: fsel0, fsel1, fsel2 are contiguous in memory, so you 
// can (and should) use array calculations!
void gpio_set_function(unsigned pin, gpio_func_t function) {
	if(pin >= 32 || pin == 47) { return; }
	if(function > 7 || function < 0) { return; }
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
	addr_val |= (function << (3 * FSELN));
	PUT32(addr, addr_val);
}

 // define: dummy to immediately return and PUT32 as above.
 void reboot(void) {
	const int PM_RSTC = 0x2010001c;
	const int PM_WDOG = 0x20100024;
	const int PM_PASSWORD = 0x5a000000;
	const int PM_RSTC_WRCFG_FULL_RESET = 0x00000020;
	int i;
	for(i = 0; i < 100000; i++)
		nop();
	PUT32(PM_WDOG, PM_PASSWORD | 1);
	PUT32(PM_RSTC, PM_PASSWORD | PM_RSTC_WRCFG_FULL_RESET);
	while(1);	
 }
