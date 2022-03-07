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
	
	uint16_t reg_val;
    i2c_read(BME_I2C, &reg_val, 2);

	return (reg_val >> 8) | (reg_val << 8);
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

/* Function  to check if bme28 sensor is still starting
 * up. See page 28 of bme280 doc.
 * Checks bit 0 of status register
 */
uint8_t starting() {
	const uint8_t status =  bme280_read(STATUS_REG);

	return status & 1;
}

/* Function to read in calibration coefficients from the proper registers
 * to be used when we read and calculate sensor data (temp, pres, hum)
 */
void read_calibraion_data(void) {
	bme280_calib_data.dig_T1 = bme280_read16(REGISTER_DIG_T1);
	bme280_calib_data.dig_T2 = (int16_t)bme280_read16(BME280_REGISTER_DIG_T2);
	bme280_calib_data.dig_T3 = (int16_t)bme280_read16(BME280_REGISTER_DIG_T3);

	bme280_calib_data.dig_P1 = bme280_read16(BME280_REGISTER_DIG_P1);
	bme280_calib_data.dig_P2 = (int16_t)bme280_read16(BME280_REGISTER_DIG_P2);
	bme280_calib_data.dig_P3 = (int16_t)bme280_read16(BME280_REGISTER_DIG_P3);
	bme280_calib_data.dig_P4 = (int16_t)bme280_read16(BME280_REGISTER_DIG_P4);
	bme280_calib_data.dig_P5 = (int16_t)bme280_read16(BME280_REGISTER_DIG_P5);
	bme280_calib_data.dig_P6 = (int16_t)bme280_read16(BME280_REGISTER_DIG_P6);
	bme280_calib_data.dig_P7 = (int16_t)bme280_read16(BME280_REGISTER_DIG_P7);
	bme280_calib_data.dig_P8 = (int16_t)bme280_read16(BME280_REGISTER_DIG_P8);
	bme280_calib_data.dig_P9 = (int16_t)bme280_read16(BME280_REGISTER_DIG_P9);

	bme280_calib_data.dig_H1 = bme280_read8(BME280_REGISTER_DIG_H1);
	bme280_calib_data.dig_H2 = (int16_t)bme280_read16(BME280_REGISTER_DIG_H2);
	bme280_calib_data.dig_H3 = bme280_read8(BME280_REGISTER_DIG_H3);
	bme280_calib_data.dig_H4 = ((int8_t)bme280_read8(BME280_REGISTER_DIG_H4) << 4) |
							(bme280_read8(BME280_REGISTER_DIG_H4 + 1) & 0xF);
	bme280_calib_data.dig_H5 = ((int8_t)bme280_read8(BME280_REGISTER_DIG_H5 + 1) << 4) |
							(bme280_read8(BME280_REGISTER_DIG_H5) >> 4);
	bme280_calib_data.dig_H6 = (int8_t)bme280_read8(BME280_REGISTER_DIG_H6);
}

/* Function to set the configuration of the bme280 sensor
 * Uses default recommended values from the bme280 manual
 * uses our defined enums from above
 */
void bme280_config(sensor_mode mode, sensor_sampling tempSampling, sensor_sampling pressSampling,
					sensor_sampling humSampling, sensor_filter filter, standby_duration duration) {
	
	//setup ctrl reg
	ctrl_meas.mode = mode;
	ctrl_meas.osrs_p = pressSamling;
	ctrl_meas.osrs_t = tempSampling;

	// setup hum sampling
	ctrl_hum  = 0; // set padding
	ctrl_hum.osrs_h = humSampling;

	// setup config reg
	config_reg = 0; //no spi and padding
	config_reg.filter = filter;
	config_reg.t_sb = duration;
}

/* bme280_init()
* Initialize the BME280  Sensor
*/
void bme280_init(void) {
	dev_barrier();

	uint8_t _sensor_id = bme280_read8(CHIPID_REG);
	assert(_sensor_id == BME_CHIPID)

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

	bme280_config();

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
    clean_reboot();
}