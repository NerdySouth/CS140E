// part2: test GPIO input.
//   - if we get a signal on the input pin 21, turn the LED on.
#include "rpi.h"

void notmain(void) {
    const int pin = 20;
    const int input = 26;
	const int led =  21;

    // gpio_set_pulldown(pin);
	gpio_set_pullup(pin);
	gpio_set_output(led);
    gpio_set_input(input);
    while(1) { 
        // could also do: 
        //  gpio_write(input, gpio_read(led));
        if(gpio_read(input))  {
			gpio_set_on(led);
		} else { 
			// gpio_clear_pulldown(pin);
			gpio_clear_pulldown(pin);
			gpio_set_off(led);
            reboot();
		}
		
    }
	
}