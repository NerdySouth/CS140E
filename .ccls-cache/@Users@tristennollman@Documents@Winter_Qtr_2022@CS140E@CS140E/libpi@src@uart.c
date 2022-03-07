// implement:
//  void uart_init(void)
//
//  int uart_can_getc(void);
//  int uart_getc(void);
//
//  int uart_can_putc(void);
//  void uart_putc(unsigned c);
//
//  int uart_tx_is_empty(void) {
//
// see that hello world works.
//
//
#include "rpi.h"

#define UART_RX_PIN 15
#define UART_TX_PIN 14
#define UART_AUX_ENABLES 0x20215004
#define UART_AUX_MU_IO_REG 0x20215040
#define UART_AUX_MU_IER_REG 0x20215044
#define UART_AUX_MU_IIR_REG 0x20215048
#define UART_AUX_MU_LCR_REG 0x2021504C
#define UART_AUX_MU_LSR_REG 0x20215054
#define UART_AUX_MU_CNTL_REG 0x20215060
#define UART_AUX_MU_STAT_REG 0x20215064
#define UART_AUX_MU_BAUD_REG 0x20215068

#define UART_115200_BAUD 270


// disable the uart.
void uart_disable(void) {
	dev_barrier();
	//clear FIFOs since they may have garbage from earlier in them
	uart_flush_tx();
	dev_barrier();
	//disable FIFOs for tx and  rx so we arent flooded with messages, p. 16 of BCM2835
	//R-M-W
	// unsigned cntl_val = GET32(UART_AUX_MU_CNTL_REG);
	// cntl_val &= ~(0b11); //set last two to 0, leave rest alone
	// PUT32(UART_AUX_MU_CNTL_REG, cntl_val);


	unsigned aux_enb_val = GET32(UART_AUX_ENABLES);
	aux_enb_val &= 0b0;
	PUT32(UART_AUX_ENABLES, aux_enb_val);
	dev_barrier();
}

// called first to setup uart to 8n1 115200  baud,
// no interrupts.
//  - you will need memory barriers, use <dev_barrier()>
//
//  later: should add an init that takes a baud rate.
void uart_init(void) {
	dev_barrier();
	//set gpio
	gpio_set_function(UART_TX_PIN, GPIO_FUNC_ALT5);
	gpio_set_function(UART_RX_PIN, GPIO_FUNC_ALT5);

	//mem barrier since writing to AUX_ENABLES and AUX is a diff device from AUX_MU
	//need this so that we get mem writes/reads in proper order. All mem reads/writes before
	//here will have been observed before code after this barrier
	dev_barrier();

	//turn on miniUART
	//get value at AUX_ENABLES Register, modify lower bits, put back, p. 9 of BCM2835
	unsigned aux_enb_val = GET32(UART_AUX_ENABLES);
	aux_enb_val |= 0b1;
	PUT32(UART_AUX_ENABLES, aux_enb_val);

	//need dev_barrier so that all reads/writes to aux_enb device will
	//be observed before this point, so that we know our reads/writes to 
	//AUX (different device from AUX_ENB)  will happen in order
	dev_barrier();

	//disable FIFOs for tx and  rx so we arent flooded with messages, p. 16 of BCM2835
	//R-M-W
	unsigned cntl_val = GET32(UART_AUX_MU_CNTL_REG);
	cntl_val &= 0b00; //set last two to 0, dont care about the rest
	PUT32(UART_AUX_MU_CNTL_REG, cntl_val);

	//clear FIFOs since they may have garbage from earlier in them p. 13 of BCM2835
	PUT32(UART_AUX_MU_IIR_REG, 0b110);

	//disable interrupts (will use them later, for now we are polling) p. 12 of BCM2835-ARM-Periph
	PUT32(UART_AUX_MU_IER_REG, 0);//-> the lowest bits are marked as read only in docs?

	//set to 8bit mode (can be 7 or 8), p.14 of BCM2835
	PUT32(UART_AUX_MU_LCR_REG, 3); 

	//set baud rate, p.19 of BCM2835
	PUT32(UART_AUX_MU_BAUD_REG, UART_115200_BAUD);

	//turn on tx and rx FIFOs
	//r-m-w
	cntl_val = GET32(UART_AUX_MU_CNTL_REG);
	cntl_val |= 0b11;
	PUT32(UART_AUX_MU_CNTL_REG, cntl_val);

	dev_barrier();
}



// 1 = at least one byte on rx queue, 0 otherwise
static int uart_can_getc(void) {
	dev_barrier();
	unsigned lsr_val = GET32(UART_AUX_MU_LSR_REG);
	lsr_val &= 1;
	dev_barrier();
	return lsr_val; 
}

// returns one byte from the rx queue, if needed
// blocks until there is one.
int uart_getc(void) {
	dev_barrier();
	while (!uart_can_getc()) {
		;
	}
	//now can read from miniUART
	unsigned val = GET32(UART_AUX_MU_IO_REG);
	//only care about bottom 8 bits(0-7)
	val &= 0b11111111;
	dev_barrier();
	return (int)val;
}

// 1 = space to put at least one byte, 0 otherwise.
int uart_can_putc(void) {
	dev_barrier();
	unsigned stat_val = GET32(UART_AUX_MU_STAT_REG);
	//check if 5th bit is set
	stat_val &= 0b10;
	dev_barrier();
	return stat_val ? 1 : 0;
}


// 1 = tx queue empty, 0 = not empty.
int uart_tx_is_empty(void) {
	dev_barrier();
	unsigned lsr_val = GET32(UART_AUX_MU_LSR_REG);
	//check 6th bit for if tx FIFO is empty and idle
	lsr_val &= 1 << 6;
	dev_barrier();
	return lsr_val;
}

// put one byte on the tx qqueue, if needed, blocks
// until TX has space.
void uart_putc(unsigned c) {
	dev_barrier();
	while (!uart_can_putc()) {
		;
	}
	//tx FIFO now not empty, write to it
	PUT32(UART_AUX_MU_IO_REG, c);
	dev_barrier();
}

// simple wrapper routines useful later.

// a maybe-more intuitive name for clients.
int uart_has_data(void) {
    return uart_can_getc();
}


// return -1 if no data, otherwise the byte.
int uart_getc_async(void) { 
	dev_barrier();
    if(!uart_has_data()) {
        return -1;
	}
	dev_barrier();
    return uart_getc();
}



// flush out all bytes in the uart --- we use this when 
// turning it off / on, etc.
void uart_flush_tx(void) {
	dev_barrier();
    while(!uart_tx_is_empty())
        ;
	dev_barrier();
}

// trivial function needed by some drivers.
int uart_hex(unsigned h) {
    printk("%x",h);
    return 0;
}
