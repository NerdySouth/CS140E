#include "rpi.h"
#include "i2c.h"
#include "bme280.h"

// i2c addr for bme280 sensor
#define BME_I2C 0x77
// chip id for bme280 sensor
#define BME_CHIPID 0x60

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

	return (reg_val[0] >> 8) | (reg_val[1] << 8);
}

/* Function to read a 3 byte value from BME280 Sensor
 * NOTE: THE SENSOR IS  IN  LITTLE ENDIAN
 * so we correct that in our read24() 
* THIS FUNCTION READS/RETURNS AN UNSIGNED VALUE
 */
uint16_t bme280_read24(uint8_t reg) {
	i2c_write(BME_I2C, &reg, 1);
	
	uint8_t reg_val[3];
    i2c_read(BME_I2C, &reg_val[0], 3);

	return ((uint32_t)(reg_val[0]) << 16) | ((uint32_t)(reg_val[1]) << 8) |
         ((uint32_t)(reg_val[2]));
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
	printk("T1 = %d\nT2  = %d\nT3 = %d\n", calib_data.dig_T1, calib_data.dig_T2, calib_data.dig_T3);

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
	calib_data.dig_H4 = ((int8_t)bme280_read8(DIG_REG_H4) << 4) |
							(bme280_read8(DIG_REG_H4 + 1) & 0xF);
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
	uint32_t temp_data = bme280_read24(TEMPDATA_REG);
	printk("temp  val = %d\n", temp_data);
	// temp_data >>= 4;
	int32_t var1, var2, real_temp;
	var1 = ((((temp_data >> 3)  - ((int32_t)calib_data.dig_T1 << 1))) * ((int32_t)calib_data.dig_T2)) >> 11;
	var2 = (((((temp_data >> 4) - ((int32_t)calib_data.dig_T1)) * ((temp_data >> 4) - ((int32_t)calib_data.dig_T1))) >> 12) * ((int32_t)calib_data.dig_T3)) >> 14;
	int32_t t_fine = var1 + var2;
	real_temp = ((t_fine * 5) + 128) >> 8;
	return real_temp;
}


// int32_t bme280_read_hum(void) {
// 	uint32_t hum_data = bme280_read24
// }

/* bme280_init()
* Initialize the BME280  Sensor
*/
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
	bme280_config(MODE_SLEEP, SAMPLING_X1, SAMPLING_X1, SAMPLING_X1, FILTER_OFF, STANDBY_MS_0_5);			// weather monitoring config

	//let sensor settle
	delay_ms(100);

	dev_barrier();

	return;
	
}



/**********************************************************************
 * trivial driver.
 */
void notmain(void) {
    uart_init();

    delay_ms(100);   // allow time for device to boot up.
    i2c_init();
    delay_ms(100);   // allow time to settle after init.


	bme280_init();

	// dance so that we discard the first (unreliable) measurement
	take_measurement();
	int32_t temp = bme280_read_temp();
	//now take real new measurements
	take_measurement();

	while (1) {
		temp = bme280_read_temp();
		printk("Temp = %d\n", temp);
		delay_ms(5000);
	}
    clean_reboot();
}