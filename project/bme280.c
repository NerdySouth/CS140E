#include "rpi.h"
#include "i2c.h"
#include "bme280.h"
#include "nrf.h"
#include "nrf-test.h"
#include "rf-support.h"
// #include "simple-boot.h"z	

// i2c addr for bme280 sensor
#define BME_I2C 0x77
// chip id for bme280 sensor
#define BME_CHIPID 0x60

// for nrf
#define NTRIAL 1000
#define TIMEOUT_USEC 1000
#define NBYTES 32

#define REBOOT 0x42069

const static unsigned server = 1;
// enum {
//     server_addr = 0xd5d5d5,
//     client_addr = 0xe5e5e5,
// };

/* TODO:
 * 1. read_temp
 * 2. read_hum
 * 3. read_pressure
 * 4. helpers that can get and set the compensation values (read from struct and write to proper reg)
 * 5. takeForceMeasurement
 */



/*
 * uint8_t bme280_read8(uint8_t reg_addr);
 * Function to read 1 byte of data from
 * reg_addr over I2C from the bme280 sensor
 */
 uint8_t bme280_read8(uint8_t reg) {
	i2c_write(BME_I2C, &reg, 1);
	
	uint8_t reg_val;
    i2c_read(BME_I2C, &reg_val, 1);

    return reg_val;
 }

/* Function to read a 2 byte value from BME280 Sensor
 * NOTE: THE SENSOR IS  IN  LITTLE ENDIAN
 * so we correct that in our read16() 
* THIS FUNCTION READS/RETURNS AN UNSIGNED VALUE
 */
uint16_t bme280_read16(uint8_t reg) {
	i2c_write(BME_I2C, &reg, 1);

	uint8_t reg_val[2];
    i2c_read(BME_I2C, &reg_val[0], 2);

	return (uint16_t)(reg_val[0]) | (uint16_t)(reg_val[1] << 8);
}

/* Function to read a 2 byte value from BME280 Sensor
 * NOTE: THE SENSOR IS  IN  LITTLE ENDIAN
 * so we correct that in our read16() 
* THIS FUNCTION READS/RETURNS AN UNSIGNED VALUE
 */
uint16_t bme280_read16_meas(uint8_t reg) {
	i2c_write(BME_I2C, &reg, 1);

	uint8_t reg_val[2];
    i2c_read(BME_I2C, &reg_val[0], 2);

	// return (uint16_t)(reg_val[0]) | (uint16_t)(reg_val[1] << 8);
	return (uint16_t)(reg_val[0]) << 8 | (uint16_t)(reg_val[1]);
}

/* Function to read a 3 byte value from BME280 Sensor
 * NOTE: THE SENSOR IS  IN  LITTLE ENDIAN
 * so we correct that in our read24() 
* THIS FUNCTION READS/RETURNS AN UNSIGNED VALUE
 */
int32_t bme280_read24(uint8_t reg) {
	i2c_write(BME_I2C, &reg, 1);
	
	uint8_t reg_val[3];
    i2c_read(BME_I2C, &reg_val[0], 3);

	uint32_t msb = (uint32_t)reg_val[0];
	uint32_t middle = (uint32_t)reg_val[1];
	uint32_t lsb = (uint32_t)reg_val[2];

	msb = (uint32_t)reg_val[0] << 16;
	middle = (uint32_t)reg_val[1] << 8;
	lsb = (uint32_t)reg_val[2];

	return msb | middle | lsb;
}

/* bme280_write8(uint8_t reg_addr, uint8_t data)
 * Function to write 1 byte  of data over I2C to
 * the bme280 sensor
 */
void bme280_write8(uint8_t reg_addr, uint8_t data) {
	uint8_t data_buf[2];
	data_buf[0] = reg_addr;
	data_buf[1] = data;
	i2c_write(BME_I2C, data_buf, 2);
}

/* Function to get the values for our control register as a uint8_t
 * so that it is suitable to write with (cannot write a struct to a register)
 */
uint8_t ctrl_reg_u8(ctrl_reg ctrl_meas) {
	return ((uint8_t)ctrl_meas.osrs_t << 5) | ((uint8_t)ctrl_meas.osrs_p << 2) | ((uint8_t)ctrl_meas.mode);
}

/* Function to get the values for our control hum register as a uint8_t
 * so that it is suitable to write with (cannot write a struct to a register)
 */
uint8_t ctrl_hum_reg_u8(ctrl_hum_reg ctrl_hum) {
	return ((uint8_t)ctrl_hum.padding << 3) | ((uint8_t)ctrl_hum.osrs_h);
}

/* Function to get the values for our config register as a uint8_t
 * so that it is suitable to write with (cannot write a struct to a register)
 */
uint8_t conf_reg_u8(conf_reg config_reg) {
	return (config_reg.t_sb << 5) | (config_reg.filter << 2) | (config_reg.padding << 1) | config_reg.spi3w_en;
}

/* Function  to check if bme28 sensor is still starting
 * up. See page 28 of bme280 doc.
 * Checks bit 0 of status register
 */
uint8_t starting() {
	const uint8_t status =  bme280_read8(STATUS_REG);
	return status & 1;
}

/* Function to read in calibration coefficients from the proper registers
 * to be used when we read and calculate sensor data (temp, pres, hum)
 */
void read_calibration_data(void) {
	calib_data.dig_T1 = bme280_read16(DIG_REG_T1);
	calib_data.dig_T2 = (int16_t)bme280_read16(DIG_REG_T2);
	calib_data.dig_T3 = (int16_t)bme280_read16(DIG_REG_T3);


	calib_data.dig_P1 = bme280_read16(DIG_REG_P1);
	calib_data.dig_P2 = (int16_t)bme280_read16(DIG_REG_P2);
	calib_data.dig_P3 = (int16_t)bme280_read16(DIG_REG_P3);
	calib_data.dig_P4 = (int16_t)bme280_read16(DIG_REG_P4);
	calib_data.dig_P5 = (int16_t)bme280_read16(DIG_REG_P5);
	calib_data.dig_P6 = (int16_t)bme280_read16(DIG_REG_P6);
	calib_data.dig_P7 = (int16_t)bme280_read16(DIG_REG_P7);
	calib_data.dig_P8 = (int16_t)bme280_read16(DIG_REG_P8);
	calib_data.dig_P9 = (int16_t)bme280_read16(DIG_REG_P9);

	calib_data.dig_H1 = bme280_read8(DIG_REG_H1);
	calib_data.dig_H2 = (int16_t)bme280_read16(DIG_REG_H2);

	calib_data.dig_H3 = bme280_read8(DIG_REG_H3);
	printk("reading dig  h4 reg...\n");
	calib_data.dig_H4 = ((int8_t)bme280_read8(DIG_REG_H4) << 4) |
							(bme280_read8(DIG_REG_H4 + 1) & 0xF);
	printk("reading dig h5 reg...\n");
	calib_data.dig_H5 = ((int8_t)bme280_read8(DIG_REG_H5 + 1) << 4) |
							(bme280_read8(DIG_REG_H5) >> 4);
	calib_data.dig_H6 = (int8_t)bme280_read8(DIG_REG_H6);
}

/* Function to set the configuration of the bme280 sensor
 * Uses default recommended values from the bme280 manual
 * uses our defined enums from above
 */
void bme280_config(uint32_t mode, uint32_t tempSampling, uint32_t pressSampling,
					uint32_t humSampling, uint32_t filter, uint32_t duration) {
	ctrl_meas.mode = mode;
	ctrl_meas.osrs_p = pressSampling;
	ctrl_meas.osrs_t = tempSampling;

	// setup hum sampling
	ctrl_hum.padding = 0; // set padding
	ctrl_hum.osrs_h = humSampling;

	// setup config reg
	config_reg.padding = 0; //no spi and padding
	config_reg.filter = filter;
	config_reg.t_sb = duration;

	// set sensor to  sleep mode or else writes to config
	// reg may be ignored. See bme280 manual pg. 27/28
	bme280_write8(CONTROL_REG, MODE_SLEEP);
	bme280_write8(CONFIG_REG, conf_reg_u8(config_reg));

	// must write to ctrl_hum then ctrl_meas. See bme280 manual
	// pg. 26/27
	bme280_write8(CONTROLHUMID_REG, ctrl_hum_reg_u8(ctrl_hum));
	bme280_write8(CONTROL_REG, ctrl_reg_u8(ctrl_meas));
}

/* Function to take a single measurement using forced mode
 * Forces the sensor to take a single measurement
 * must wait for the sensor to complete, otherwise will read out old values
 * see bme280 manual on  forced mode
 */
uint8_t take_measurement(void) {
	ctrl_meas.mode = MODE_FORCED;
	bme280_write8(CONTROL_REG, ctrl_reg_u8(ctrl_meas));
	while (bme280_read8(STATUS_REG) & 0x08) {delay_ms(1);}
	return  1;
		
}

/* Function to take a tempature reading from the bme28p sensor
 * Returns a 32 bit signed int.
 * returns  number like "6054"
 */
int32_t bme280_read_temp(void) {
	take_measurement();
	int32_t temp_data = bme280_read24(TEMPDATA_REG1);

	temp_data >>= 4;
	int32_t var1, var2, real_temp;
	var1 = (int32_t)((temp_data >> 3) - ((int32_t)calib_data.dig_T1 << 1));
	var1 = (var1 * ((int32_t)calib_data.dig_T2)) >> 11;
	var2 = (int32_t)((temp_data >> 4) - ((int32_t)calib_data.dig_T1));
	var2 = (((var2 * var2) >> 12) * ((int32_t)calib_data.dig_T3)) >> 14;

	//update t_fine from .h
	t_fine = var1 + var2;

	real_temp = (t_fine * 5 + 128) / 256;
	//fareinheight conversion
	int32_t far = ((real_temp  * 9) / 5) + 3200;

	return far;
	
}

/* Function to take a pressure reading from the bme280 sensor
 * Returns a 32 bit signed int. uses 64 bit long div, unusable on pi zero
 * returns  number like "60540978" which means 60540978 / 256 = 
 */
	// uint32_t bme280_read_pressure(void) {
	// 	int32_t press_data = bme280_read24(TEMPDATA_REG1);

	// 	int64_t var1, var2, real_press;
	// 	var1 = ((int64_t)t_fine) - 128000;
	// 	var2 = var1 * var1 * (int64_t)calib_data.dig_P6;
	// 	var2 += ((var1  * (int64_t)calib_data.dig_P5) << 17);
	// 	var2 += (((int64_t)calib_data.dig_P4) << 35);
	// 	var1 = ((var1 * var1 * (int64_t)calib_data.dig_P3) >> 8) + ((var1 *  (int64_t)calib_data.dig_P2) << 12);
	// 	var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)calib_data.dig_P1) >> 33;
		
	// 	// div by zero chance
	// 	if (!var1)
	// 		return 0;

	// 	//update t_fine from .h
	// 	real_press = 1048576 - press_data;
	// 	real_press = (((real_press << 31) - var2) * 3125) / var1;
	// 	var1 = (((int64_t)calib_data.dig_P9) * (real_press >> 13) * (real_press  >> 13) >> 25);
	// 	var2 = (((int64_t)calib_data.dig_P8) * real_press) >> 19;
	// 	real_press = ((real_press + var1 + var2) >> 8) + (((int64_t)calib_data.dig_P7) << 4);
	// 	return real_press;
		
	// }

int32_t bme_read_humidity(void) {

	int32_t var1, var2, var3, var4, var5;

	bme280_read_temp(); // must be done first to get t_fine

	int32_t adc_H = bme280_read16_meas(HUMIDDATA_REG);	
	if (adc_H == 0x8000) // value in case humidity measurement was disabled
		return 0; //NaN

	printk("adc_H: %x\n", adc_H);

	printk("dig_H1: %x\n", calib_data.dig_H1);
	printk("dig_H2: %x\n", calib_data.dig_H2);
	printk("dig_H3: %x\n", calib_data.dig_H3);
	printk("dig_H4: %x\n", calib_data.dig_H4);
	printk("dig_H5: %x\n", calib_data.dig_H5);
	printk("dig_H6: %x\n", calib_data.dig_H6);

	var1 = t_fine - ((int32_t)76800);
	var2 = (int32_t)(adc_H * 16384);
	var3 = (int32_t)(((int32_t)calib_data.dig_H4) * 1048576);
	var4 = ((int32_t)calib_data.dig_H5) * var1;
	var5 = (((var2 - var3) - var4) + (int32_t)16384) / 32768;
	var2 = (var1 * ((int32_t)calib_data.dig_H6)) / 1024;
	var3 = (var1 * ((int32_t)calib_data.dig_H3)) / 2048;
	var4 = ((var2 * (var3 + (int32_t)32768)) / 1024) + (int32_t)2097152;
	var2 = ((var4 * ((int32_t)calib_data.dig_H2)) + 8192) / 16384;
	var3 = var5 * var2;
	var4 = ((var3 / 32768) * (var3 / 32768)) / 128;
	var5 = var3 - ((var4 * ((int32_t)calib_data.dig_H1)) / 16);
	var5 = (var5 < 0 ? 0 : var5);
	var5 = (var5 > 419430400 ? 419430400 : var5);
	uint32_t H = (uint32_t)(var5 / 4096);

	return H / 1024;

}


// int32_t bme280_read_hum(void) {
// 	uint32_t hum_data = bme280_read24
// }

// /* bme280_init()
// * Initialize the BME280  Sensor
// */
void bme280_init(void) {
	dev_barrier();

	uint8_t _sensor_id = bme280_read8(CHIPID_REG);
	assert(_sensor_id == BME_CHIPID);

	// soft reset the device to make sure things are off
	// like interrupts. see page 14 of bme280 manual for what reset does
	// and page 27 for how to use reset register
	bme280_write8(SOFTRESET_REG, 0xB6);

	// delay so sensor has time to settle
	delay_ms(10);

	// delay until done writing to image registers from NVM (non volatile mem) is done
	// write to image reg from NVM happens on all Power-on-resets, and before each conversion
	// see page 28 of bme280 doc
	while (starting()) {
		delay_ms(10);
	}

	//setup sensor coefficients for when we calulate sensor readings
	// see chapter 4.2.2 of bme280 manual
	read_calibration_data();

	// bme280_config(MODE_SLEEP, SAMPLING_X16, SAMPLING_X16, SAMPLING_X16, FILTER_OFF, STANDBY_MS_0_5);			// default config
	bme280_config(MODE_SLEEP, SAMPLING_X1, SAMPLING_X1, SAMPLING_X1, FILTER_OFF, STANDBY_MS_10);			// weather monitoring config

	//let sensor settle
	delay_ms(100);

	dev_barrier();

	return;
	
}


// should add a checksum to see if things are getting corrupted.
static inline msg_send_t mk_test_msg(uint32_t i, unsigned nbytes) {
    struct { uint32_t v; uint8_t pad[32-4]; } test_msg = { .v = i };
#if 0
    return msg_mk_pkt(nbytes, &test_msg, nbytes);
#else
    msg_send_t m = msg_send_mk(nbytes);
    msg_put_n(&m, &test_msg, nbytes);
    return m;

#endif
}
// returns number of lost packets
static void
send_reboot(nrf_t *server_nic, int verbose_p) {
	// only server can send reboots
	assert(server);
    unsigned nbytes = NBYTES;

	//build message to send
	msg_send_t reboot_msg = msg_send_mk(nbytes);
	struct { uint32_t v; uint8_t pad[32-4]; } test_msg = { .v = REBOOT };
	msg_put_n(&reboot_msg, &test_msg, nbytes);

	//send msg
	nrf_send_msg_ack(server_nic, client_addr, reboot_msg);
	
}

// returns number of lost packets
static void
client_listen(nrf_t *client_nic, int verbose_p) {
	nrf_pipe_t *client = nrf_get_pipe(client_nic, 0);
	while (1) {
		unsigned timeout = TIMEOUT_USEC;
		unsigned nbytes = NBYTES;
		unsigned ntimeout = 0, npackets = 0;
		// receive from client nic
		msg_recv_t m;
		if(nrf_get_msg_exact_timeout(client, &m, nbytes, timeout) == nbytes) {
			assert(msg_nbytes_get(&m) == nbytes);
			unsigned x = msg_get32(&m);

			if(x != REBOOT)
				debug("client: received %d (expected=%d)\n", x, REBOOT);
			assert(x == REBOOT);
			output("received reboot command.\n");
			clean_reboot();
			npackets++;
		} else {
			if(verbose_p) 
				debug("receive failed for packet=%d\n", npackets);
			ntimeout++;
		}
		output("trial: total successfully sent %d ack'd packets lost %d\n",
			npackets, ntimeout);
		delay_ms(5000);
	}
}

void notmain(void) {
    uart_init();

    // delay_ms(100);   // allow time for device to boot up.
    // i2c_init();
    // delay_ms(100);   // allow time to settle after init.


	// bme280_init();

	// // dance so that we discard the first (unreliable) measurement
	// take_measurement();
	// int32_t temp = bme280_read_temp();
	// //now take real new measurements
	// take_measurement();

	// while (1) {
	// 	temp = bme280_read_temp();
	// 	printk("Temp = %d\n", temp);
	// 	delay_ms(5000);

	// 	take_measurement();
	// 	int32_t hum = bme_read_humidity();
	// 	printk("Humidity = %d\n", hum);
	// 	delay_ms(5000);
	// }

	unsigned nbytes = 32;
    int ack_p = 1;
	
	
	
	if (server) {
		// client
		output("about to configure ack'd server\n");
		nrf_t *server_nic = server_config(nbytes, ack_p);
		send_reboot(server_nic, 1);
		while(1) {;}
	} else {
		// client
		output("about to configure ack'd client\n");
		nrf_t *client_nic = client_config(nbytes, ack_p);
		client_listen(client_nic, 1);
	}
    
    clean_reboot();
}