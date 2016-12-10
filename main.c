#define F_CPU 16000000UL

#include <stdio.h>
#include <avr/io.h>	
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/twi.h>
#include <util/delay.h>
#include <stdint.h>
#include <math.h>
#include "softuart.h"
#include "MCP7940M.h"	

#define CALIBRATE										/* 	Uncomment for Calibration of the RTCC */
														/* 	This should be done before the intial start-up at a new place.
														 * 	!PWM can not be used during initial calibration! 
													 	 * 	Estimated calibration value will be sent over UART (Binary output!).
														 * 	Recomment, set value to RTCC_CALIBRATION_VALUE and reflash!
														 * 	However the frequency of the crystal at room temperature was measured
														 * 	to be 32766.2Hz instead of 32768Hz. This means minus 108 cycles per minute,
														 *	which gives a calibration value of 0b10110110 */
//#define SET_TIME										/* Uncomment to set the current time to the RTCC */
														/* This should be done after a reset of the RTCC. For example when the power supply failed. 
														 * Recomment and reflash! */

#ifdef CALIBRATE

	#define RTCC_CALIBRATION_TIME	10UL				/* Time in minutes over which the RTCC shall be calibrated (24 hours is recommended) */
	
#elif defined SET_TIME

	#define RTCC_CALIBRATION_VALUE 	0b10110110			/* Initial calibration value; !Set after intial calibration is done! */
	#define CURRENT_SECOND			0
	#define CURRENT_MINUTE			26						
	#define CURRENT_HOUR			0
	#define CURRENT_DAY				7
	#define CURRENT_DATE			30
	#define CURRENT_MONTH			11
	#define CURRENT_YEAR			16
	
#else

	#define PWM_RESOLUTION			255
	#define SUNRISE_HOUR			14
	#define SUNRISE_MINUTE			26
	#define SUNSET_HOUR				14
	#define SUNSET_MINUTE			36
	
#endif



#ifdef CALIBRATE
	/**
	 * @brief Starts the RTCC with cleared register values. Initializes the 16-bit timer,
	 * which triggers an interrupt after every millisecond.
	 */
	void rtcc_start_calibration(void);
	
	/**
	 * @brief Calculates the calibration value from estimated times on MCU and RTCC.
	 */
	void rtcc_get_calibration_value(void);
	
	volatile static rtcc_time_t rtcc_cal_time;			/* Stores the elapsed time of the RTCC for calibration */
	volatile static int rtcc_calibration_flag = 0;		/* Flag for calibration of the RTCC */
	volatile static uint8_t rtcc_cal_value;				/* Will hold the estimated value after calibration */
	volatile static int rtcc_cal_error = 0;				/* Will be 1, if oscillator can not be calibrated */
	
#elif defined SET_TIME

	/**
	 * @brief Loads the given time into the registers of the RTCC.
	 */
	void set_current_time(void);
	
#else
	
	/**
	 * @brief Initializes the 16-bit timer for onboard timing. 
	 */
	void timer_init(void);
	
	void timer_start(void);
	
	void timer_stop(void);

	/**
	 * @brief Initializes Timer2 for PWM. 
	 */
	void pwm_init(void);

	/**
	 * @brief Calculates the delay time in seconds for every step. 
	 */
	void pwm_calculate_delay_times(void);

	/**
	 * @brief Starts Timer2.
	 */
	void pwm_start(void);

	/**
	 * @brief Stops Timer2.
	 */
	void pwm_stop(void);

	/**
	 * @brief Simulates sunrise.
	 */
	void sunrise(void);

	/**
	 * @brief Simulates sunset.
	 */
	void sunset(void);


	volatile static uint16_t pwm_delay_times[PWM_RESOLUTION];		/* Holds the delay values in seconds. */
	static rtcc_time_t current_time;						/* Stores the current time */
	volatile static uint8_t sunrise_flag = 0;
	volatile static uint8_t sunset_flag = 0;
	
#endif

/**
 * @brief Initializes the software UART interface.
 */
void uart_init(void);


int main (void)										
{
	uint8_t data;
	
	DDRD |= (1<<DDD7)|(1<<DDD3);						/* Set Output */
	
	uart_init();
	twi_init();
	rtcc_start_osc();
	
	#ifdef CALIBRATE
		
		//data = 0x00;
		data = 0b11101100;
		rtcc_byte_write(CAL_REG, &data);				/* Reset calibration value */
		rtcc_start_calibration();						/* Initiate calibration */
		while(rtcc_calibration_flag);					/* Wait until flag is cleared */
		rtcc_get_calibration_value();					/* Load value into variable */
	
	#elif defined SET_TIME
	
		set_current_time();								/* Send current time to RTCC */
		data = RTCC_CALIBRATION_VALUE;	
		rtcc_byte_write(CAL_REG, &data);				/* Set calibration value */
	
	#else
	
		timer_init();
		pwm_init();
		pwm_calculate_delay_times();
		rtcc_get_time(&current_time);
		//Check if sunrise or sunset should already be in progress, set output
		timer_start();
		
	#endif
	
	while(1)			
	{
		
		#ifdef CALIBRATE
		
			char softuart_out[80];
			uint8_t buffer = rtcc_cal_value;
			
			/* Calibration finished, blink LED and send calibration value over UART */
			_delay_ms(500);
			PIND |= (1<<PIND7);
			
			if(!rtcc_cal_error) {
				softuart_puts("Calculated calibration value: 0b");
				
				for(int i=0; i<8; i++) {
					if(buffer & 0x80)
						softuart_putchar(0x31);
					else
						softuart_putchar(0x30);
						
					buffer <<= 1;
				}
			}
			else {
				softuart_puts("Crystal could not be calibrated! Deviation to much!!!");
			}
			
			softuart_puts("; Elapsed Time was: ");
			sprintf(softuart_out, "%i Stunden, %i Minuten und %i Sekunden!", 
			rtcc_cal_time.hours,
			rtcc_cal_time.minutes,
			rtcc_cal_time.seconds);
			softuart_puts(softuart_out);
			softuart_puts("\r");
		
		#elif defined SET_TIME
			
			/* Time was set -> blink LED */
			_delay_ms(500);
			PIND = (1<<PIND7);
		
		#else
			/*
			if(sunrise_flag) {
				timer_stop();
				sunrise();
				sunrise_flag = 0;
				PORTD |= (1<<PD3);
				rtcc_get_time(&current_time);
				timer_start();
			}
			if(sunset_flag) {
				timer_stop();
				sunset();
				sunset_flag = 0;
				PORTD &= 0xF7;
				rtcc_get_time(&current_time);
				timer_start();
			}
			PORTD = (1<<PD7);*/
		#endif
		/*
		char softuart_out[80];
		rtcc_time_t time;
		
		rtcc_get_time(&time);
		softuart_puts("Time: ");
			
			sprintf(softuart_out, "%i:%i:%iUhr am %i.%i.%i!\r", 
			time.hours,
			time.minutes,
			time.seconds,
			time.date,
			time.month,
			time.year+2000);
			softuart_puts(softuart_out);
		
		//rtcc_byte_read(DATE_REG, &data);
		//softuart_puts("RTCC lauft seit: ")
		//if(ERR_CODE != TWI_SUCCESS) {
		//	sprintf(softuart_out, "::Fehler -> 0x%x::\n", ERR_CODE);
		//	softuart_puts(softuart_out);
		//}
		//else {
			//softuart_putchar(((data & 0x70)>>4) + 0x30);
			//softuart_putchar((data & 0x0F) + 0x30);
			//softuart_puts("Sekunden\n");
		//}*/
	}				
	return 0;			
}	

void uart_init() 
{
	softuart_init();
	sei();
}

#ifdef CALIBRATE

	void rtcc_start_calibration() 
	{
		uint8_t data;
		
		/* Stop oscillator and reset registers */
		data = 0x00;
		rtcc_byte_write(SEC_REG, &data);
		rtcc_byte_write(MIN_REG, &data);
		rtcc_byte_write(HOUR_REG, &data);
		rtcc_oscon_flag = 0;
		
		/* Initialize 16-bit timer */
		cli();											/* Disable global interrupts */
		/* May stop the 8-bit timer for softuart application while calibrating! */
		
		OCR1A = 15999;									/* Interrupt every 1ms (no prescaling) */
		TIMSK1 = (1<<OCIE1A);							/* Enable interrupt on compare match OCR1A */
		TCCR1B = (1<<WGM12);							/* Enable CTC */
		TCCR1B |= (1<<CS10);							/* Start timer (no prescaling) */
		
		rtcc_calibration_flag = 1;						/* Set flag */
		
		sei();											/* Enable global interrupts */
	}
	
	void rtcc_get_calibration_value()
	{
		uint32_t mcu_seconds, rtcc_seconds;					/* Elapsed seconds */
		uint32_t ppm;										/* Deviation in parts per million */	
		uint8_t cal_value;
		
		mcu_seconds = RTCC_CALIBRATION_TIME * 60;
		rtcc_seconds 	= (uint32_t)rtcc_cal_time.seconds + 
						(uint32_t)rtcc_cal_time.minutes*60 +
						(uint32_t)rtcc_cal_time.hours*3600;
		
		if(mcu_seconds>rtcc_seconds)
			ppm = (mcu_seconds-rtcc_seconds)*1000000/ mcu_seconds;
		else
			ppm = (rtcc_seconds-mcu_seconds)*1000000/ mcu_seconds;

		cal_value = (ppm*32768*60)/ 2000000;

		if(cal_value > 127) {
			//Crystal can not be calibrated!
			rtcc_cal_error = 1;
			cal_value = 0;
		}
		else
			cal_value |= mcu_seconds>rtcc_seconds ? 0x80 : 0x00;
		
		rtcc_cal_value = cal_value;
		//rtcc_byte_write(CAL_REG, &cal_value);
	}
	
	/**
	 * @brief Interrupt service for the 16-Bit timer. On first run
	 * the RTCC oscillator will be started. Counts the milliseconds,
	 * seconds and minutes elapsed on the MCU. If RTCC_CALIBRATION_TIME
	 * is reached, it will read the time from the RTCC, clear the flag 
	 * and stop the timer.
	 */
	ISR(TIMER1_COMPA_vect) 
	{
		volatile static int millis = -1;
		volatile static int seconds;
		volatile static int minutes;
		uint8_t data;
		
		if(millis < 0) {								/* Check if calibration is in progress */
			millis++;
			data = (1<<ST_OSC);
			rtcc_byte_write(ST_OSC_REG, &data);			/* Start oscillator of the RTCC */
			rtcc_oscon_flag = 1;
			return;
		}
		else {
			if(++millis == 1000) {						/* Iterate millis, seconds and minutes */
				millis = 0;
				if(++seconds == 60) {
					seconds = 0;
					if(++minutes == RTCC_CALIBRATION_TIME) {
						minutes = 0;
						/* Read elapsed time from the RTCC, reset flag and stop timer */
						rtcc_cal_time.seconds = rtcc_get_seconds();
						rtcc_cal_time.minutes = rtcc_get_minutes();
						rtcc_cal_time.hours = rtcc_get_hours();
						rtcc_calibration_flag = 0;
						TCCR1B = 0;
					}
				}
			}
		}
	}
	
#elif defined SET_TIME

	void set_current_time()
	{
		rtcc_time_t time;
		
		time.seconds 	= CURRENT_SECOND;
		time.minutes 	= CURRENT_MINUTE;
		time.hours 		= CURRENT_HOUR;
		time.day		= CURRENT_DAY;
		time.date		= CURRENT_DATE;
		time.month		= CURRENT_MONTH;
		time.year 		= CURRENT_YEAR;
		
		rtcc_set_time(&time);
	}
	
#else

	void timer_init()
	{
		/* Initialize 16-bit timer */
		cli();											/* Disable global interrupts */
		
		OCR1A = 15624;									/* Interrupt every 1s (prescaling 1024) */
		TCCR1B = (1<<WGM12);							/* Enable CTC */
		TIMSK1 = (1<<OCIE1A);							/* Enable interrupt on compare match OCR1A */
		
		sei();											/* Enable global interrupts */
	}
	
	void timer_start()
	{
		TCCR1B |= (1<<CS12)|(1<<CS10);					/* Start timer (prescaling 1024) */
	}
	
	void timer_stop()
	{
		TCCR1B = (1<<WGM12);
	}
	
	void pwm_init()
	{
		cli();
		
		TCCR2A 	= (1<<WGM21)|(1<<WGM20);					/* Fast PWM -> TOP = 0xFF */
		TCCR2A 	|= (1<<COM2B1)|(1<<COM2B0);					/* Inverting mode -> Set OC2B on Compare Match */

		sei();
	}
	
	void pwm_calculate_delay_times()
	{
		double time_n, time_m;
		double illuminance_n, illuminance_m;
		int pos = 0xFF - 1;

		for(int i=pos; i>0; i--) {
			illuminance_n = (double)(255 - i)/256;
			illuminance_m = (double)(255 - i + 1)/256;
			time_n = 1.52*log10(illuminance_n/0.063)/ log10(1.15);
			time_m = 1.52*log10(illuminance_m/0.063)/ log10(1.15);
			pwm_delay_times[i] = (time_m-time_n)*60;
		}
		pwm_delay_times[0] = 2;	
	}
	
	void pwm_start()
	{
		TCCR2B 	= (1<<CS22)|(1<<CS21);						/* Start timer (prescaling 256 -> 244Hz) */
	}

	void pwm_stop()
	{
		TCCR2B = 0;											/* Stop timer */
	}
	
	void sunrise()
	{
		OCR2B = 0xFF - 1;
		pwm_start();
		
		while(OCR2B > 0) {
			for(int i=0; i<pwm_delay_times[OCR2B]; i++)
				_delay_ms(1000);
			OCR2B--;
		}
		_delay_ms(2000);
			
		pwm_stop();
	}

	void sunset()
	{
		OCR2B = 0;
		pwm_start();
		
		while(OCR2B < PWM_RESOLUTION) {
			for(int i=0; i<pwm_delay_times[OCR2B]; i++)
				_delay_ms(1000);
			OCR2B++;
		}
		
		pwm_stop();
	}

	ISR(TIMER1_COMPA_vect)
	{
		if(++current_time.seconds == 60) {
			current_time.seconds = 0;
			if(++current_time.minutes == 60) {
				current_time.minutes = 0;
				if(++current_time.hours == 24) {
					current_time.hours = 0;
				}
			}
		}
		
		if(current_time.hours == SUNRISE_HOUR && current_time.minutes == SUNRISE_MINUTE)
			sunrise_flag = 1;
		else if(current_time.hours == SUNSET_HOUR && current_time.minutes == SUNSET_MINUTE)
			sunset_flag = 1;
	}

#endif