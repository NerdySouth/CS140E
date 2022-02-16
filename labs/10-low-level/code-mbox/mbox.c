#include "rpi.h"
#include "mbox.h"

/*
  Get board serial
    Tag: 0x00010004
    Request: Length: 0
    Response: Length: 8
    Value: u64: board serial
*/
uint64_t rpi_get_serialnum(void) {
    static volatile unsigned *u = 0;

    // must be 16-byte aligned.
    if(!u)
        u = kmalloc_aligned(8*4,16);
    memset((void*)u, 0, 8*4);

    // should abstract this.
    u[0] = 8*4;   // total size in bytes.
    u[1] = 0;   // always 0 when sending.
    // serial tag
    u[2] = 0x00010004;
    u[3] = 8;   // response size size
    u[4] = 0;   // request size
    u[5] = 0;   // space for first 4 bytes of serial
    u[6] = 0;   // space for second 4 bytes of serial
    u[7] = 0;   // end tag

    mbox_send(MBOX_CH, u);

    if(u[1] != 0x80000000)
		panic("invalid response: got %x\n", u[1]);

    return (uint64_t)u[5] << 32 | u[6];
}

/* Get ARM memory

 * 		Tag: 0x00010005
 *		Request:
 * 		Length: 0
 * 		Response:
 * 		Length: 8
 * 		Value:
 * 		u32: base address in bytes
 * 		u32: size in bytes
 */
uint32_t rpi_get_memsize(void) {
    static volatile unsigned *u = 0;

    if(!u)
        u = kmalloc_aligned(8*4,16);
    memset((void*)u, 0, 8*4);

	u[0] = 8*4;   // total size in bytes.
    u[1] = 0;   // always 0 when sending.
    // serial tag
    u[2] = 0x00010005;
    u[3] = 8;   // response size size
    u[4] = 0;   // request size
    u[5] = 0;   // space for first 4bytes of response
    u[6] = 0;   // space for second  4 bytes
    u[7] = 0;   // end tag


    mbox_send(MBOX_CH, u);
    demand(u[5] == 0, "expected 0 base, have %d\n", u[5]);
    return u[6];
}


 /*
        Get max clock rate
        Tag: 0x00030004
        Request:
        Length: 4
        Value:
        u32: clock id
        Response:
        Length: 8
        Value:
        u32: clock id
        u32: rate (in Hz)
     */
uint32_t rpi_clock_maxhz_get(uint32_t clock) {
   
    static volatile unsigned *u = 0;

    if(!u)
        u = kmalloc_aligned(8*4,16);
    memset((void*)u, 0, 8*4);

    u[0] = 8*4;   // total size in bytes.
    u[1] = 0;   // always 0 when sending.
    // serial tag
    u[2] = 0x00030004;
    u[3] = 8;   // response size size
    u[4] = 4;   // request size
    u[5] = clock;   // clock id
    u[6] = 0;   // clock hz
    u[7] = 0;   // end tag

    mbox_send(MBOX_CH, u);
    return u[6];
}

/*
    Tag: 0x00038002
    Request:
    Length: 12
    Value:
    u32: clock id
    u32: rate (in Hz)
    u32: skip setting turbo
    Response:
    Length: 8
    Value:
    u32: clock id
    u32: rate (in Hz)
*/
uint32_t rpi_clock_hz_set(uint32_t clock, uint32_t hz) {

    static volatile unsigned *u = 0;

    if(!u)
        u = kmalloc_aligned(9*4,16);
    memset((void*)u, 0, 9*4);

    unimplemented();

    mbox_send(MBOX_CH, u);
    return u[6];
}

/*
	Get cpu clock rate
	Tag: 0x00030002
	Request:
		Length: 4
		Value:
			u32: clock id
	Response:
		Length: 8
		Value:
			u32: clock id
			u32: clock rate (in Hz)
*/
uint32_t rpi_clock_curhz_get(uint32_t clock) {
	static volatile unsigned *u = 0;

    if(!u)
        u = kmalloc_aligned(8*4,16);
    memset((void*)u, 0, 8*4);

	u[0] = 8*4;   // total size in bytes.
    u[1] = 0;   // always 0 when sending.
    // serial tag
    u[2] = 0x00030004;
    u[3] = 8;   // response size size
    u[4] = 4;   // request size
    u[5] = clock;   // clock  id from request
    u[6] = 0;   // clock rate in response
    u[7] = 0;   // end tag

    mbox_send(MBOX_CH, u);
    return u[6];
}

/*
Get board model
Tag: 0x00010001
Request:
Length: 0
Response:
Length: 4
Value:
u32: board model
*/
uint32_t rpi_get_model(void) {
    static volatile unsigned *u = 0;

    if(!u)
        u = kmalloc_aligned(7*4,16);
    memset((void*)u, 0, 7*4);

	u[0] = 8*4;   // total size in bytes.
    u[1] = 0;   // always 0 when sending.
    // serial tag
    u[2] = 0x00010001;
    u[3] = 4;   // response size size
    u[4] = 0;   // request size
    u[5] = 0;   // first 4 bytes of  resp
    u[6] = 0;   // bottom 4 bytes of resp 0 out
    u[7] = 0;   // end tag

    mbox_send(MBOX_CH, u);
    return u[5];
}

/*
Get board revision
Tag: 0x00010002
Request:
Length: 0
Response:
Length: 4
Value:
u32: board model
*/
uint32_t rpi_get_revision(void) {
    static volatile unsigned *u = 0;

    if(!u)
        u = kmalloc_aligned(7*4,16);
    memset((void*)u, 0, 7*4);

	u[0] = 8*4;   // total size in bytes.
    u[1] = 0;   // always 0 when sending.
    // serial tag
    u[2] = 0x00010002;
    u[3] = 4;   // response size size
    u[4] = 0;   // request size
    u[5] = 0;   // first 4 bytes of  resp
    u[6] = 0;   // bottom 4 bytes of resp 0 out
    u[7] = 0;   // end tag

    mbox_send(MBOX_CH, u);
    return u[5];
}

/*
Get board mac addr
Tag: 0x00010003
Request:
Length: 0
Response:
Length: 6
Value:
u32: board mac address in network byte order
*/
uint32_t rpi_get_mac_addr(void) {
    static volatile unsigned *u = 0;

    if(!u)
        u = kmalloc_aligned(7*4,16);
    memset((void*)u, 0, 7*4);

	u[0] = 8*4;   // total size in bytes.
    u[1] = 0;   // always 0 when sending.
    // serial tag
    u[2] = 0x00010003;
    u[3] = 6;   // response size size
    u[4] = 0;   // request size
    u[5] = 0;   // first 4 bytes of  resp
    u[6] = 0;   // bottom 4 bytes of resp 0 out
    u[7] = 0;   // end tag

    mbox_send(MBOX_CH, u);
    return (uint32_t)u[5];
}
