#ifndef __MBOX_H__
#define __MBOX_H__

//
// rpi mailbox interface.
//  https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
//
//  a more reasonable but unofficial writeup:
//  http://magicsmoke.co.za/?p=284
//
// General format [in 32-bit words]:
//   word 0: total size of message [in bytes] 
//   word 1: 0 --- always when sending
//   word 2: tag value
//   word 3: nbytes in reply
//   word 4: nbytes of request 
//        [... nbytes of space.  size=max(nbytes reply, nbytes request)]
//   last word: 0   end tag
//
//   so: value of word 0 = 6*4 + max(response size, request size)

#include "rpi.h"

enum {
	RESERVED_CLOCK = 0x000000000,
	EMMC_CLOCK = 0x000000001,
	UART_CLOCK = 0x000000002,
	ARM_CLOCK = 0x000000003,
	CORE_CLOCK = 0x000000004,
	V3D_CLOCK = 0x000000005,
	H264_CLOCK = 0x000000006,
	ISP_CLOCK = 0x000000007,
	SDRAM_CLOCK = 0x000000008,
	PIXEL_CLOCK = 0x000000009,
	PWM_CLOCK = 0x00000000a,
	HEVC_CLOCK = 0x00000000b,
	EMMC2_CLOCK = 0x00000000c,
	M2MC_CLOCK = 0x00000000d,
	PIXEL_BVB_CLOCK = 0x00000000e
};
 
enum { OneMB = 1024 * 1024 };

/***********************************************************************
 * mailbox interface
 */

#define MAILBOX_FULL   (1<<31)
#define MAILBOX_EMPTY  (1<<30)
#define MAILBOX_START  0x2000B880
#define GPU_MEM_OFFSET    0x40000000

// document states: only using 8 right now.
#define MBOX_CH  8

/*
    REGISTER	ADDRESS
    Read	        0x2000B880
    Poll	        0x2000B890
    Sender	        0x2000B894
    Status	        0x2000B898
    Configuration	0x2000B89C
    Write	        0x2000B8A0
 */
#define MBOX_READ   0x2000B880
#define MBOX_STATUS 0x2000B898
#define MBOX_WRITE  0x2000B8A0

// need to pass in the pointer as a GPU address?
static inline uint32_t uncached(volatile void *cp) { 
    // not sure this is needed: we have no data caching
    // since no VM
	return (unsigned)cp | GPU_MEM_OFFSET; 	
}

static inline void 
mbox_write(unsigned channel, volatile void *data) {
    assert(channel == MBOX_CH);
    // check that is 16-byte aligned
	assert((unsigned)data%16 == 0);

    // 1. we don't know what else we were doing before: sync up 
    //    memory.
    dev_barrier();

    // 2. if mbox status is full, wait.
	unsigned wait = 0;
	do {
		unsigned mbox_status = GET32(MBOX_STATUS);
		wait = (mbox_status & MAILBOX_FULL) ? 1 : 0;
	} while (wait);
	
	//left shift the data so it is 28 bits, so when we OR in channel it fits
	*(unsigned*)data <<= 4;

    // 3. write out the data along with the channel
	PUT32(MBOX_WRITE, uncached(data) | channel);

    // 4. make sure everything is flushed.
    dev_barrier();
}

static inline unsigned 
mbox_read(unsigned channel) {
    assert(channel == MBOX_CH);

    // 1. probably don't need this since we call after mbox_write.
    dev_barrier();

    // 2. while mailbox is empty, wait.
	unsigned wait = 0;
	 do {
		unsigned mbox_status = GET32(MBOX_STATUS);
		wait = (mbox_status & MAILBOX_EMPTY) ? 1 : 0;
	} while (wait);

    // 3. read from mailbox and check that the channel is set.
    uint32_t v = 0;
	v = GET32(MBOX_READ);
	// unsigned read_channel  = v & channel;
	// if (read_channel != channel)
	// 	goto start;


    // 4. verify that the reply is for <channel>
    if((v & 0xf) != channel)
        panic("impossible(?): mailbox read for a different channel\n");

    // return it.
    return v;
}

static inline uint32_t 
mbox_send(unsigned channel, volatile void *data) {
    mbox_write(MBOX_CH, data);
    mbox_read(MBOX_CH);

    volatile uint32_t *u = data;
    if(u[1] != 0x80000000)
		panic("invalid response: got %x\n", u[1]);
    return 0;
}

uint32_t rpi_get_revision(void);
uint32_t rpi_get_model(void);
uint32_t rpi_clock_curhz_get(uint32_t clock);
uint32_t rpi_clock_hz_set(uint32_t clock, uint32_t hz);
uint32_t rpi_clock_maxhz_get(uint32_t clock);
uint32_t rpi_get_memsize(void);
uint64_t rpi_get_serialnum(void);

uint32_t rpi_get_mac_addr(void);

#endif
