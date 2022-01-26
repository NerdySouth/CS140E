// example client that uses our tracing system: can cross check these values
// against the ones on your laptop!
#include "rpi.h"
#include "trace.h"

static void my_putk(char *s) {
    for(; *s; s++)
        uart_putc(*s);
}
void notmain(void) {
    uart_init();

    my_putk("about to trace gpio_read(20):\n");
    // trace_start(0);
    int v = gpio_read(20);
    // trace_stop();
    my_putk("done: gpio_read(20)=%d\n");

    clean_reboot();
}
