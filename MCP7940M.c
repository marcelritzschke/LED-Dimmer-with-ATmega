#include <stdio.h>
#include <avr/io.h>
#include <util/twi.h>
#include "MCP7940M.h"

volatile static int master_mode_active = 0;		/* Needs to be set to 1, if the MCU is in master mode */


void twi_init() 
{
	TWBR = 12; 									/* Division factor for SCL frequency */		
}

uint8_t twi_tx_start()
{
	uint8_t ERR_CODE = TWI_SUCCESS;
	
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);		/* Clear TWINT flag, enable TWI and send start */
	while(!(TWCR & (1<<TWINT)));				/* Wait for flag */
	
	if(TW_STATUS != TW_START) {					/* Check for errors */
		ERR_CODE = TW_STATUS;
	}
	
	return ERR_CODE;
}

uint8_t twi_tx_rep_start()
{
	uint8_t ERR_CODE = TWI_SUCCESS;
	
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);		/* Clear TWINT flag, enable TWI and send start */	
	while(!(TWCR & (1<<TWINT)));				/* Wait for flag */	
	
	if(TW_STATUS != TW_REP_START) {				/* Check for errors */
		ERR_CODE = TW_STATUS;
	}
	
	return ERR_CODE;
}

uint8_t twi_tx_stop()
{
	uint8_t ERR_CODE = TWI_SUCCESS;
	
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);		/* Clear TWINT flag, enable TWI and send stop */
	
	return ERR_CODE;
}

uint8_t twi_tx_sla_r(uint8_t sla_addr)
{
	uint8_t ERR_CODE = TWI_SUCCESS;
	
	TWDR = (sla_addr << 1)|TW_READ;				/* Load data into data register of the TWI */								
	TWCR = (1<<TWINT)|(1<<TWEN);				/* Clear TWINT flag, enable TWI */		
	while(!(TWCR & (1<<TWINT)));				/* Wait for flag */	
	
	if(TW_STATUS != TW_MR_SLA_ACK) {			/* Check for errors */
		ERR_CODE = TW_STATUS;
	}
	
	return ERR_CODE;
}

uint8_t twi_tx_sla_w(uint8_t sla_addr)
{
	uint8_t ERR_CODE = TWI_SUCCESS;
	
	TWDR = (sla_addr << 1)|TW_WRITE;			/* Load data into data register of the TWI */								
	TWCR = (1<<TWINT)|(1<<TWEN);				/* Clear TWINT flag, enable TWI */		
	while(!(TWCR & (1<<TWINT)));				/* Wait for flag */	
	
	if(TW_STATUS != TW_MT_SLA_ACK) {			/* Check for errors */
		ERR_CODE = TW_STATUS;
	}
	
	return ERR_CODE;
}

uint8_t twi_tx_data(uint8_t* data)
{
	uint8_t ERR_CODE = TWI_SUCCESS;
	
	TWDR = *data;								/* Load data into data register of the TWI */									
	TWCR = (1<<TWINT)|(1<<TWEN);				/* Clear TWINT flag, enable TWI */	
	while(!(TWCR & (1<<TWINT)));				/* Wait for flag */			
	
	if(TW_STATUS != TW_MT_DATA_ACK) {			/* Check for errors */
		ERR_CODE = TW_STATUS;
	}
	
	return ERR_CODE;
}

uint8_t twi_rx_data(uint8_t* data)
{
	uint8_t ERR_CODE = TWI_SUCCESS;
	
	TWCR = (1<<TWINT)|(1<<TWEN);				/* Clear TWINT flag, enable TWI */	
	while(!(TWCR & (1<<TWINT)));				/* Wait for flag */
	
	if(TW_STATUS != TW_MR_DATA_NACK) {			/* Check for errors */
		ERR_CODE = TW_STATUS;
	}
	else {
		*data = TWDR;							/* On success get data from the data register of the TWI */
	}
	
	return ERR_CODE;
}

uint8_t rtcc_byte_read(uint8_t mem_address, uint8_t* data)
{
	uint8_t ERR_CODE;
	
	//if(!master_mode_active) {					/* Send start condition or repeated start condition */
	//	ERR_CODE = twi_tx_start();
	//	master_mode_active = 1;					/* MCU is now in master mode */
	//}
	//else 
	ERR_CODE = twi_tx_start();
	
	if(ERR_CODE != TWI_SUCCESS) {				/* Check for errors */
		PORTD |= (1<<PD6);
		return ERR_CODE;
	}
	
	ERR_CODE = twi_tx_sla_w(SLA_ADDRESS);		/* Send SLA+W */
	if(ERR_CODE != TWI_SUCCESS) {				/* Check for errors */
		PORTD |= (1<<PD6);
		return ERR_CODE;
	}
	
	ERR_CODE = twi_tx_data(&mem_address);		/* Send memory address */
	if(ERR_CODE != TWI_SUCCESS) {				/* Check for errors */
		PORTD |= (1<<PD6);
		return ERR_CODE;
	}
	
	ERR_CODE = twi_tx_rep_start();				/* Send repeated start */
	if(ERR_CODE != TWI_SUCCESS) {				/* Check for errors */
		PORTD |= (1<<PD6);
		return ERR_CODE;
	}
	
	ERR_CODE = twi_tx_sla_r(SLA_ADDRESS);		/* Send SLA+R */
	if(ERR_CODE != TWI_SUCCESS) {				/* Check for errors */
		PORTD |= (1<<PD6);
		return ERR_CODE;
	}
	
	ERR_CODE = twi_rx_data(data);				/* Receive data byte */
	if(ERR_CODE != TWI_SUCCESS) {				/* Check for errors */
		PORTD |= (1<<PD6);
		return ERR_CODE;
	}
	
	ERR_CODE = twi_tx_stop();					/* Send stop condition */
		if(ERR_CODE != TWI_SUCCESS) {			/* Check for errors */
		PORTD |= (1<<PD6);
		return ERR_CODE;
	}
	
	return ERR_CODE;
}

uint8_t rtcc_byte_write(uint8_t mem_address, uint8_t* data)
{
	uint8_t ERR_CODE;
	
	//if(!master_mode_active) {					/* Send start condition or repeated start condition */
	//	ERR_CODE = twi_tx_start();
	//	master_mode_active = 1;					/* MCU is now in master mode */
	//}
	//else 
	ERR_CODE = twi_tx_start();
	
	if(ERR_CODE != TWI_SUCCESS) {				/* Check for errors */
		PORTD |= (1<<PD6);
		return ERR_CODE;
	}
	
	ERR_CODE = twi_tx_sla_w(SLA_ADDRESS);		/* Send SLA+W */
	if(ERR_CODE != TWI_SUCCESS) {				/* Check for errors */
		PORTD |= (1<<PD6);
		return ERR_CODE;
	}
	
	ERR_CODE = twi_tx_data(&mem_address);		/* Send memory address */
	if(ERR_CODE != TWI_SUCCESS) {				/* Check for errors */
		PORTD |= (1<<PD6);
		return ERR_CODE;
	}
	
	ERR_CODE = twi_tx_data(data);				/* Send data byte */
	if(ERR_CODE != TWI_SUCCESS) {				/* Check for errors */
		PORTD |= (1<<PD6);
		return ERR_CODE;
	}
	
	ERR_CODE = twi_tx_stop();					/* Send stop condition */
	if(ERR_CODE != TWI_SUCCESS) {				/* Check for errors */
		PORTD |= (1<<PD6);
		return ERR_CODE;
	}
	
	return ERR_CODE;
}

void rtcc_start_osc()
{
	uint8_t data;
	
	rtcc_byte_read(OSCON_REG, &data);			/* Read OSCON */
	
	if(!((data & OSCON_MASK) >> OSCON)) {		/* Check if oscillator is on */
		data = ST_OSC_MASK;
		rtcc_byte_write(ST_OSC_REG, &data);		/* Start oscillator */
	}
	rtcc_oscon_flag = 1;						/* Set flag */
}

void rtcc_get_time(rtcc_time_t* data)
{
	data->seconds 	= rtcc_get_seconds();
	data->minutes 	= rtcc_get_minutes();
	data->hours 	= rtcc_get_hours();
	data->day		= rtcc_get_day();
	data->date		= rtcc_get_date();
	data->month		= rtcc_get_month();
	data->year		= rtcc_get_year();
}

unsigned int rtcc_get_seconds()
{
	 return get_decimal(SEC_REG, SEC_MASK, SEC10_MASK);
}

unsigned int rtcc_get_minutes()
{
	return get_decimal(MIN_REG, MIN_MASK, MIN10_MASK);
}

unsigned int rtcc_get_hours()
{
	return get_decimal(HOUR_REG, HOUR_MASK, HOUR10_MASK);
}

unsigned int rtcc_get_day()
{
	return get_decimal(DAY_REG, DAY_MASK, DAY10_MASK);
}

unsigned int rtcc_get_date()
{
	return get_decimal(DATE_REG, DATE_MASK, DATE10_MASK);
}

unsigned int rtcc_get_month()
{
	return get_decimal(MONTH_REG, MONTH_MASK, MONTH10_MASK);
}

unsigned int rtcc_get_year()
{
	return get_decimal(YEAR_REG, YEAR_MASK, YEAR10_MASK);
}

unsigned int get_decimal(uint8_t reg, uint8_t _mask, uint8_t _10_mask)
{
	uint8_t data;
	unsigned value;
	
	rtcc_byte_read(reg, &data);
	value = data & _mask;
	value += ((data & _10_mask)>>4) * 10;
	
	return value;
}

void rtcc_set_time(rtcc_time_t* time)
{
	rtcc_set_seconds(&time->seconds);
	rtcc_set_minutes(&time->minutes);
	rtcc_set_hours(&time->hours);
	rtcc_set_day(&time->day);
	rtcc_set_date(&time->date);
	rtcc_set_month(&time->month);
	rtcc_set_year(&time->year);
}

void rtcc_set_seconds(unsigned int* seconds)
{
	uint8_t data;
	
	if(rtcc_oscon_flag)
		data = get_binary(seconds, SEC_MASK, SEC10_MASK)|(1<<ST_OSC);
	else
		data = get_binary(seconds, SEC_MASK, SEC10_MASK);
		
	rtcc_byte_write(SEC_REG, &data);
}

void rtcc_set_minutes(unsigned int* minutes)
{
	uint8_t data;
	
	data = get_binary(minutes, MIN_MASK, MIN10_MASK);
	rtcc_byte_write(MIN_REG, &data);
}

void rtcc_set_hours(unsigned int* hours)
{
	uint8_t data;
	
	data = get_binary(hours, HOUR_MASK, HOUR10_MASK);
	rtcc_byte_write(HOUR_REG, &data);
}

void rtcc_set_day(unsigned int* day)
{
	uint8_t data;
	
	data = get_binary(day,DAY_MASK, DAY10_MASK);
	rtcc_byte_write(DAY_REG, &data);
}

void rtcc_set_date(unsigned int* date)
{
	uint8_t data;
	
	data = get_binary(date, DATE_MASK, DATE10_MASK);
	rtcc_byte_write(DATE_REG, &data);
}

void rtcc_set_month(unsigned int* month)
{
	uint8_t data;
	
	data = get_binary(month, MONTH_MASK, MONTH10_MASK);
	rtcc_byte_write(MONTH_REG, &data);
}

void rtcc_set_year(unsigned int* year)
{
	uint8_t data;
	
	data = get_binary(year, YEAR_MASK, YEAR10_MASK);
	rtcc_byte_write(YEAR_REG, &data);
}

uint8_t get_binary(unsigned int* data, uint8_t _mask, uint8_t _10_mask)
{
	uint8_t value;
	
	value = ((*data%10) & _mask);
	value |= ((*data/10)<<4) & _10_mask;
	
	return value;
}

void disable_master_mode()
{
	master_mode_active = 0;
}