// engler, cs140 put your gpio implementations in here.
#include "rpi.h"

//set rising edge registers
#define RPI_GPREN0_REG 0x2020004C
#define RPI_GPREN1_REG 0x20200050
//set falling edge register
#define RPI_GPFEN0_REG 0x20200058

#define RPI_GPFEN1_REG 0x2020005C
//detect GPIO Event register
#define RPI_GPEDS0_REG 0x20200040
#define RPI_GPEDS1_REG 0x20200044
//enable gpio general interrupts rregister
#define RPI_ENABLE_IRQ2_REG 0x2000B214
#define RPI_IRQ2_PENDING 0x2000B208
#define GPIO_ENABLE_INTERRUPTS 49

static unsigned GPIO_ENABLED = 0;

// gpio_int_rising_edge and gpio_int_falling_edge (and any other) should
// call this routine (you must implement) to setup the right GPIO event.
// as with setting up functions, you should bitwise-or in the value for the 
// pin you are setting with the existing pin values.  (otherwise you will
// lose their configuration).  you also need to enable the right IRQ.   make
// sure to use device barriers!!
int is_gpio_int(unsigned gpio_int) {
	if (gpio_int != GPIO_INT0)
		panic("passed an invalid gpio_int to is_gpio_int.\n");

	dev_barrier();
	unsigned reg_val = GET32(RPI_IRQ2_PENDING);
	unsigned status = reg_val & (1  << (gpio_int - 32));
	dev_barrier();
	return status  ?  1 :  0;// engler, cs140 put your gpio implementations in here.
#include "rpi.h"

//set rising edge registers
#define RPI_GPREN0_REG 0x2020004C
#define RPI_GPREN1_REG 0x20200050
//set falling edge register
#define RPI_GPFEN0_REG 0x20200058

#define RPI_GPFEN1_REG 0x2020005C
//detect GPIO Event register
#define RPI_GPEDS0_REG 0x20200040
#define RPI_GPEDS1_REG 0x20200044
//enable gpio general interrupts rregister
#define RPI_ENABLE_IRQ2_REG 0x2000B214
#define RPI_IRQ2_PENDING 0x2000B208

//keep track of whether general gpio interrupts have been enabled yet.
static unsigned GPIO_INT_GENERAL_ENABLED = 0;

// gpio_int_rising_edge and gpio_int_falling_edge (and any other) should
// call this routine (you must implement) to setup the right GPIO event.
// as with setting up functions, you should bitwise-or in the value for the 
// pin you are setting with the existing pin values.  (otherwise you will
// lose their configuration).  you also need to enable the right IRQ.   make
// sure to use device barriers!!
int is_gpio_int(unsigned gpio_int) {
	if (gpio_int != GPIO_INT0)
		panic("passed an invalid gpio_int to is_gpio_int.\n");

	dev_barrier();
	unsigned reg_val = GET32(RPI_IRQ2_PENDING);
	unsigned status = reg_val & (1  << (gpio_int - 32));
	dev_barrier();
	return status  ?  1 :  0;
}


void gpio_int_general() {
	dev_barrier();
	unsigned reg_val = GET32(RPI_ENABLE_IRQ2_REG);
	reg_val |= (1 << (GPIO_INT0 - 32));
	PUT32(RPI_ENABLE_IRQ2_REG, reg_val);
	GPIO_INT_GENERAL_ENABLED = 1;
} 

// p97 set to detect rising edge (0->1) on <pin>.
// as the broadcom doc states, it  detects by sampling based on the clock.
// it looks for "011" (low, hi, hi) to suppress noise.  i.e., its triggered only
// *after* a 1 reading has been sampled twice, so there will be delay.
// if you want lower latency, you should us async rising edge (p99)
void gpio_int_rising_edge(unsigned pin) {
	if (pin > 31 && pin != 47)
		return;

	dev_barrier(); //-> is this needed?
	//determine which  reg to use
	unsigned reg_addr = (pin < 32) ? RPI_GPREN0_REG : RPI_GPREN1_REG;
	//get value  of rising set reg
	unsigned reg_val = GET32(reg_addr);

	//bitwise or in pin n to the n'th  bit
	reg_val |= (1 << (pin % 32));

	//put reg_val back to set enable it for pin n
	PUT32(reg_addr, reg_val);

	if (!GPIO_INT_GENERAL_ENABLED) 
		gpio_int_general();

	dev_barrier(); //-> is this needed?
}

// p98: detect falling edge (1->0).  sampled using the system clock.  
// similarly to rising edge detection, it suppresses noise by looking for
// "100" --- i.e., is triggered after two readings of "0" and so the 
// interrupt is delayed two clock cycles.   if you want  lower latency,
// you should use async falling edge. (p99)
void gpio_int_falling_edge(unsigned pin) {
	if (pin > 31 && pin != 47) 
		return;

	dev_barrier(); //-> is this needed?

	//determine which  reg to use
	unsigned reg_addr = (pin < 32) ? RPI_GPFEN0_REG : RPI_GPFEN1_REG;

	//get value  of rising set reg
	unsigned reg_val = GET32(reg_addr);

	//bitwise or in pin n to the n'th  bit
	reg_val |= (1 << (pin % 32));

	//put reg_val back to set enable it for pin n
	PUT32(reg_addr, reg_val);

	if (!GPIO_ENABLED) 
		gpio_int_general();

	dev_barrier(); //-> is this needed?
}

// p96: a 1<<pin is set in EVENT_DETECT if <pin> triggered an interrupt.
// if you configure multiple events to lead to interrupts, you will have to 
// read the pin to determine which caused it.
int gpio_event_detected(unsigned pin) {
	if (pin > 31 && pin != 47) 
		return 0;

	dev_barrier(); //-> needed?
	//determine which reg to use
	unsigned reg_addr = (pin < 32) ? RPI_GPEDS0_REG : RPI_GPEDS1_REG;
	//get value of the detection register
	unsigned reg_val = GET32(reg_addr);

	//check if pin n has event by checking bit n
	unsigned status = reg_val & (1 << (pin % 32));

	dev_barrier();
	return status ? 1 : 0;
}

// p96: have to write a 1 to the pin to clear the event.
void gpio_event_clear(unsigned pin) {
	if (pin > 31 && pin != 47) 
		return;

	dev_barrier(); //-> needed?
	//determine addr to use
	unsigned reg_addr = (pin < 32) ? RPI_GPEDS0_REG : RPI_GPEDS1_REG;
	// unsigned reg_val = GET32(reg_addr);
	//set a mask so we can write to the pin
	unsigned clear_mask = (1 << pin);
	//write the bit to clear
	PUT32(reg_addr, clear_mask);
	dev_barrier();
}

}


void gpio_int_general() {
	dev_barrier();
	unsigned reg_val = GET32(RPI_ENABLE_IRQ2_REG);
	reg_val |= (1 << (GPIO_INT0 - 32));
	PUT32(RPI_ENABLE_IRQ2_REG, reg_val);
	GPIO_ENABLED = 1;
} 

// p97 set to detect rising edge (0->1) on <pin>.
// as the broadcom doc states, it  detects by sampling based on the clock.
// it looks for "011" (low, hi, hi) to suppress noise.  i.e., its triggered only
// *after* a 1 reading has been sampled twice, so there will be delay.
// if you want lower latency, you should us async rising edge (p99)
void gpio_int_rising_edge(unsigned pin) {
	if (pin > 31 && pin != 47)
		return;

	dev_barrier(); //-> is this needed?
	//determine which  reg to use
	unsigned reg_addr = (pin < 32) ? RPI_GPREN0_REG : RPI_GPREN1_REG;
	//get value  of rising set reg
	unsigned reg_val = GET32(reg_addr);

	//bitwise or in pin n to the n'th  bit
	reg_val |= (1 << (pin % 32));

	//put reg_val back to set enable it for pin n
	PUT32(reg_addr, reg_val);

	// if (!GPIO_ENABLED) 
	gpio_int_general();

	dev_barrier(); //-> is this needed?
}

// p98: detect falling edge (1->0).  sampled using the system clock.  
// similarly to rising edge detection, it suppresses noise by looking for
// "100" --- i.e., is triggered after two readings of "0" and so the 
// interrupt is delayed two clock cycles.   if you want  lower latency,
// you should use async falling edge. (p99)
void gpio_int_falling_edge(unsigned pin) {
	if (pin > 31 && pin != 47) 
		return;

	dev_barrier(); //-> is this needed?

	//determine which  reg to use
	unsigned reg_addr = (pin < 32) ? RPI_GPFEN0_REG : RPI_GPFEN1_REG;

	//get value  of rising set reg
	unsigned reg_val = GET32(reg_addr);

	//bitwise or in pin n to the n'th  bit
	reg_val |= (1 << (pin % 32));

	//put reg_val back to set enable it for pin n
	PUT32(reg_addr, reg_val);

	// if (!GPIO_ENABLED) 
	gpio_int_general();

	dev_barrier(); //-> is this needed?
}

// p96: a 1<<pin is set in EVENT_DETECT if <pin> triggered an interrupt.
// if you configure multiple events to lead to interrupts, you will have to 
// read the pin to determine which caused it.
int gpio_event_detected(unsigned pin) {
	if (pin > 31 && pin != 47) 
		return 0;

	dev_barrier(); //-> needed?
	//determine which reg to use
	unsigned reg_addr = (pin < 32) ? RPI_GPEDS0_REG : RPI_GPEDS1_REG;
	//get value of the detection register
	unsigned reg_val = GET32(reg_addr);

	//check if pin n has event by checking bit n
	unsigned status = reg_val & (1 << (pin % 32));

	dev_barrier();
	return status ? 1 : 0;
}

// p96: have to write a 1 to the pin to clear the event.
void gpio_event_clear(unsigned pin) {
	if (pin > 31 && pin != 47) 
		return;

	dev_barrier(); //-> needed?
	//determine addr to use
	unsigned reg_addr = (pin < 32) ? RPI_GPEDS0_REG : RPI_GPEDS1_REG;
	// unsigned reg_val = GET32(reg_addr);
	//set a mask so we can write to the pin
	unsigned clear_mask = (1 << pin);
	//write the bit to clear
	PUT32(reg_addr, clear_mask);
	dev_barrier();
}
