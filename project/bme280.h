#ifndef __RPI_BME280_H__
#define __RPI_BME280_H__

// register addresses. See page chapter 5.4, page 25 of bme280 doc
enum {
	// tempature compensation values
	DIG_REG_T1 = 0x88,
	DIG_REG_T2 = 0x90,
	DIG_REG_T3 =  0x92,

	// pressure compenation values
	DIG_REG_P1 = 0x8E,
	DIG_REG_P2 = 0x90,
	DIG_REG_P3 = 0x92,
	DIG_REG_P4 = 0x94,
	DIG_REG_P5 = 0x96,
	DIG_REG_P6 = 0x98,
	DIG_REG_P7 = 0x9A,
	DIG_REG_P8 = 0x9C,
	DIG_REG_P9 = 0x9E,

	// humidity compensation values
	DIG_REG_H1 = 0xA1,
	DIG_REG_H2 = 0xE1,
	DIG_REG_H3 = 0xE3,
	DIG_REG_H4 = 0xE4,
	DIG_REG_H5 = 0xE5,
	DIG_REG_H6 = 0xE7,

	// chip registers
	CHIPID_REG = 0xD0,
  	VERSION_REG = 0xD1,
  	SOFTRESET_REG = 0xE0,
	
	// R calibration stored in 0xE1-0xF0
	CAL26_REG = 0xE1,

	// status/config registers
	STATUS_REG = 0XF3,
  	CONTROL_REG = 0xF4,
  	CONFIG_REG = 0xF5,
	CONTROLHUMID_REG = 0xF2,
	
	// data registers
	PRESSUREDATA_REG = 0xF7,
  	TEMPDATA_REG = 0xFA,
  	HUMIDDATA_REG = 0xFD

};

// Oversampling modes. See Chapter 5.4 of bme280. Page 28
// Note temp, press, and hum have the same oversampling mode options
enum sensor_sampling {
    SAMPLING_NONE = 0b000,
    SAMPLING_X1 = 0b001,
    SAMPLING_X2 = 0b010,
    SAMPLING_X4 = 0b011,
    SAMPLING_X8 = 0b100,
    SAMPLING_X16 = 0b101
};

// Filtering settings found in bme280 datasheet Chapter 5.4 page 30
enum sensor_filter {
    FILTER_OFF = 0b000,
    FILTER_X2 = 0b001,
    FILTER_X4 = 0b010,
    FILTER_X8 = 0b011,
    FILTER_X16 = 0b100
};

// Sensor Modes. Found in bme280 Chapter 3 Page 15
enum sensor_mode {
	MODE_SLEEP = 0b00,
    MODE_FORCED = 0b01,
    MODE_NORMAL = 0b11
};

struct typedef {
	/// temperature compensation values
    uint16_t dig_T1; 
	int16_t dig_T2;  
	int16_t dig_T3;  

    ///< pressure compensation value
	uint16_t dig_P1; 
	int16_t dig_P2; 
	int16_t dig_P3;  
	int16_t dig_P4;  
	int16_t dig_P5;  
	int16_t dig_P6;  
	int16_t dig_P7;  
	int16_t dig_P8;  
	int16_t dig_P9;  

    ///< humidity compensation value
	uint8_t dig_H1; 
	int16_t dig_H2; 
	uint8_t dig_H3; 
	int16_t dig_H4; 
	int16_t dig_H5; 
	int8_t dig_H6;  
} bme_calib_data;

//bitfield struct for bme280 ctrl register
struct typedef {
    // temperature oversampling
    unsigned int osrs_t : 3; 

    // pressure oversampling
    unsigned int osrs_p : 3; 

    // device mode
    unsigned int mode : 2; 

    /// @return combined ctrl register
    unsigned int get() { return (osrs_t << 5) | (osrs_p << 2) | mode; }
}ctrl_meas; 

/* bme280_init()
* Initialize the BME280  Sensor
*/
void bme280_init(void);

/*
 * uint8_t bme280_read8(uint8_t reg_addr);
 * Function to read 1 byte of data from
 * reg_addr over I2C from the bme280 sensor
 */
uint8_t bme280_read8(uint8_t reg);

/* bme280_write8(uint8_t reg_addr, uint8_t data)
 * Function to write 1 byte  of data over I2C to
 * the bme280 sensor
 */
void bme280_write8(uint8_t reg_addr, uint8_t data);

/* Function  to check if bme28 sensor is still starting
 * up. See page 28 of bme280 doc.
 * Checks bit 0 of status register
 */
bool starting();

/* Function to read a 2 byte value from BME280 Sensor
 * NOTE: THE SENSOR IS  IN  LITTLE ENDIAN
 * so we correct that in our read16() 
* THIS FUNCTION READS/RETURNS AN UNSIGNED VALUE
 */
uint16_t bme280_read16(uint8_t reg);

/* Function to read in calibration coefficients from the proper registers
 * to be used when we read and calculate sensor data (temp, pres, hum)
 */
void read_calibraion_data(void);

/* Function to set the configuration of the bme280 sensor
 * Uses default recommended values from the bme280 manual
 * uses our defined enums from above
 */
void bme280_config(sensor_mode mode = MODE_NORMAL,
                   sensor_sampling tempSampling = SAMPLING_X16,
                   sensor_sampling pressSampling = SAMPLING_X16,
                   sensor_sampling humSampling = SAMPLING_X16,
                   sensor_filter filter = FILTER_OFF,
                   standby_duration duration = STANDBY_MS_0_5);


#endif