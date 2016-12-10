#ifndef MCP7940M_H
#define MCP7940M_H

#include <stdint.h>

#define SLA_ADDRESS 	0b1101111		/* Slave address */
#define TWI_SUCCESS 	0xD0			/* Success code for a twi operation */

/* Definitions for the RTCC
 * 	Register addresses
 * 	Masks
 * 	Configuration bits
 */
#define ST_OSC_REG		0x00
#define ST_OSC			7		
#define ST_OSC_MASK 	0x80

#define OSCON_REG		0x03
#define OSCON			5
#define OSCON_MASK		0x20

#define SEC_REG			0x00
#define SEC_MASK		0x0F
#define SEC10_MASK		0x70

#define MIN_REG			0x01
#define MIN_MASK		0x0F
#define MIN10_MASK		0x70

#define HOUR_REG		0x02
#define HOUR_MASK		0x0F
#define HOUR10_MASK		0x30
#define HOUR24_EN_MASK	0x00

#define DAY_REG			0x03
#define DAY_MASK		0x05
#define DAY10_MASK		0x00

#define DATE_REG		0x04
#define DATE_MASK		0x0F
#define DATE10_MASK		0x30

#define MONTH_REG		0x05
#define MONTH_MASK		0x0F
#define MONTH10_MASK	0x10
#define LP_MASK			0x20

#define YEAR_REG		0x06
#define YEAR_MASK		0x0F
#define YEAR10_MASK		0xF0

#define CTRL_REG		0x07
#define OUT				7
#define SQWE			6
#define ALM1			5
#define ALM0			4
#define EXTOSC			3
#define RS2				2
#define RS1				1
#define RS0				0

#define CAL_REG			0x08

/*--------------------------------------------------------------------------------*/

volatile static int rtcc_oscon_flag = 0;/* 0 if oscillator is off, 1 if oscillator is on. */

typedef struct{							/* Time structure for the RTCC */
	unsigned seconds;
	unsigned minutes;
	unsigned hours;
	unsigned day;
	unsigned date;
	unsigned month;
	unsigned year;
}rtcc_time_t;

/*--------------------------------------------------------------------------------*/
/* Declarations for the TWI */
 
/**
* @brief Initializes the TWI. SCL frequency must 
* be set to 400kHz to communicate with the RTCC.
*/
void twi_init(void);

/**
 * @brief Sends start condition over the TWI.
 * @return Error code
 */
uint8_t twi_tx_start(void);

/**
 * @brief Sends repeated start condition over the TWI.
 * @return Error code
 */
uint8_t twi_tx_rep_start(void);

/**
 * @brief Sends stop condition over the TWI.
 * @return Error code
 */
uint8_t twi_tx_stop(void);

/**
 * @brief Sends SLA+R over the TWI.
 * @param sla_addr Slave address (7 bits)
 * @return Error code
 */
uint8_t twi_tx_sla_r(uint8_t);

/**
 * @brief Sends SLA+W over the TWI.
 * @param sla_addr Slave address (7 bits)
 * @return Error code
 */
uint8_t twi_tx_sla_w(uint8_t);

/**
 * @brief Sends one data byte over the TWI.
 * @param data Pointer where data is stored
 * @return Error code
 */
uint8_t twi_tx_data(uint8_t*);

/**
 * @brief Receives data byte over the TWI.
 * @param Pointer where data should be stored
 * @return Error code
 */
uint8_t twi_rx_data(uint8_t*);

/*--------------------------------------------------------------------------------*/
/* Declarations for the RTCC */

/**
 * @brief Performs a random read from the internal memory of the MCP7940M.
 * @param 	mem_address	Memory address to read from
 * @param	data		Pointer where received data should be stored
 * @return 	Error code
 */
uint8_t rtcc_byte_read(uint8_t, uint8_t*);

/**
 * @brief Performs a byte write to the internal memory of the MCP7940M.
 * @param mem_address Memory address to write to
 * @param data Pointer where data is stored
 * @return 
 */
uint8_t rtcc_byte_write(uint8_t, uint8_t*);

/**
 * @brief Check if the internal oscillator is on. Starts it as the case may be.
 * @return Error code
 */
void rtcc_start_osc(void);

/*--------------------------------------------------------------------------------*/
/**
 * @brief Reads the clock and calender registers.
 * @param Pointer where data should be stored.
 */
void rtcc_get_time(rtcc_time_t*);

/**
 * @brief Reads register from the RTCC and returns seconds.
 * @return Seconds as decimal value (0-59)
 */
unsigned int rtcc_get_seconds(void);

/**
 * @brief Reads register from the RTCC and returns minutes.
 * @return Minutes as decimal value (0-59)
 */
unsigned int rtcc_get_minutes(void);

/**
 * @brief Reads register from the RTCC and returns hours.
 * @return Hours as decimal value (0-59)
 */
unsigned int rtcc_get_hours(void);

/**
 * @brief Reads register from the RTCC and returns the day.
 * @return Day as decimal value (1-7)
 */
unsigned int rtcc_get_day(void);

/**
 * @brief Reads register from the RTCC and returns the date.
 * @return Date as decimal value (1-31)
 */
unsigned int rtcc_get_date(void);

/**
 * @brief Reads register from the RTCC and returns the month.
 * @return Month as decimal value (1-12)
 */
unsigned int rtcc_get_month(void);

/**
 * @brief Reads register from the RTCC and returns the year.
 * @return Year as decimal value (0-99)
 */
unsigned int rtcc_get_year(void);

/**
 * @brief Reads from register of the RTCC, combines two hex values and returns decimal.
 * @param reg Register to read from
 * @param _mask Mask for the first byte (contains MSB)
 * @param _10_mask Mask for second byte
 * @return Masked register value in decimal
 */
unsigned int get_decimal(uint8_t, uint8_t, uint8_t);

/*--------------------------------------------------------------------------------*/
/**
 * @brief Writes the time to the RTCC registers.
 * @param time Time to be set
 */
void rtcc_set_time(rtcc_time_t*);

/**
 * @brief Writes the seconds to the appropriate register.
 * @param seconds Seconds to be set
 */
void rtcc_set_seconds(unsigned int*);

/**
 * @brief Writes the minutes to the appropriate register.
 * @param minutes Minutes to be set
 */
void rtcc_set_minutes(unsigned int*);

/**
 * @brief Writes the hours to the appropriate register.
 * @param hours Hours to be set
 */
void rtcc_set_hours(unsigned int*);

/**
 * @brief Writes the day to the appropriate register.
 * @param day Day to be set
 */
void rtcc_set_day(unsigned int*);

/**
 * @brief Writes the date to the appropriate register.
 * @param date Date to be set
 */
void rtcc_set_date(unsigned int*);

/**
 * @brief Writes the month to the appropriate register.
 * @param month Month to be set
 */
void rtcc_set_month(unsigned int*);

/**
 * @brief Writes the year to the appropriate register.
 * @param year Year to be set
 */
void rtcc_set_year(unsigned int*);

/**
 * @brief Takes an integer and returns the masked binary value appropriate to the registers of the RTCC.
 * @param data Integer value
 * @param _mask Mask for bits 0-3
 * @param _10_mask Mask for bits 4-7
 * @return Binary value
 */
uint8_t get_binary(unsigned int*, uint8_t, uint8_t);

/**
 * @brief Disables master mode of the MCU
 */
void disable_master_mode(void);
 
#endif