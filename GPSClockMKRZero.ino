/*
	GPSClockMKRZero.ino - simple GPS clock 
	Copyright (C) 2023 Maciej Bartosiak

	This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define USE_GGA // for # of satellites
#define PL_LANG

#include "GPSParser.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

GPSParser gps;

uint8_t timezone = 1; // +1 for Central Europe Time

uint8_t sats = 0;
uint8_t prev_hour = 255;
uint8_t prev_min  = 255;

bool timepulse = false;

void setup() 
{
	// initialize display
	if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
		for (;;); // loop forever if library fails to initialize
	}

	display.clearDisplay();

	// draw simple splash screen
	display.setTextColor(SSD1306_WHITE,SSD1306_BLACK); //set text to white
	display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);

	display.setTextSize(1,2);
	display.setCursor(38, 24);
#if !defined(PL_LANG)
	display.print("GPS Clock");
#else
	display.print("Zegar GPS");
#endif
	display.display();
	
	// Serial port for debugging
	//Serial.begin(115200);

	// initialize GPS serial port
	Serial1.begin(9600);

	delay(500);

	// disable unwanted GPS messages. do not disable RMC for time and GGA for # of satellites
#if !defined(USE_GGA)
	Serial1.print("$PUBX,40,GGA,0,0,0,0*5A\n\r"); // # of satellites
#endif
	// Serial1.print("$PUBX,40,RMC,0,0,0,0*47\n\r"); // time and date
	Serial1.print("$PUBX,40,GLL,0,0,0,0*5C\n\r");
	Serial1.print("$PUBX,40,GSA,0,0,0,0*4E\n\r");
	Serial1.print("$PUBX,40,GSV,0,0,0,0*59\n\r");
	Serial1.print("$PUBX,40,VTG,0,0,0,0*5E\n\r");
	
	// Serial1.print("$PUBX,40,TXT,0,0,0,0*43\n\r");

	// optional messages to disable.
	// commented out because the following messages 
	// are disabled by default in the Arduino MKR GPS Shield

	// Serial1.print("$PUBX,40,GBS,0,0,0,0*4D\n\r");
	// Serial1.print("$PUBX,40,GLQ,0,0,0,0*41\n\r");
	// Serial1.print("$PUBX,40,GNQ,0,0,0,0*43\n\r");
	// Serial1.print("$PUBX,40,GNS,0,0,0,0*41\n\r");
	// Serial1.print("$PUBX,40,GPQ,0,0,0,0*5D\n\r");
	// Serial1.print("$PUBX,40,GRS:,0,0,0,0*5D\n\r");
	// Serial1.print("$PUBX,40,GST,0,0,0,0*5B\n\r");
	
	delay(500);
	// display.clearDisplay();
	//display.ssd1306_command(SSD1306_SETCONTRAST);
  	//display.ssd1306_command(0xff);
	// setup pin 6 for timepulse interrupt
	pinMode(6, INPUT);
	attachInterrupt(digitalPinToInterrupt(6), pps, RISING);

	first_loop();
}

// first loop
void first_loop(void)
{
	uint8_t c;
	static bool first_data = true;

	while(1) {
		if (Serial1.available()) {
			c = Serial1.read();
			gps.nmea_parse(c);
			if (gps.time_ready) {
				gps.time_ready = false;
				
				// display the data prepared ealier if there was no interrupt
				if(!timepulse)
					display.display();
				if ((gps.day == 0) && (gps.second == 0)) {
					display_no_signal();
				} else {
					if (first_data == true) {
						display.clearDisplay();
						first_data = false;
					}
					display_date();
					prepare_time();
					if (gps.day != 0) {
						prev_hour = 255;
						prev_min  = 255;
						return;
					}
				}
			}
			if (gps.nsat_ready) {
				gps.nsat_ready = false;
				sats = gps.nsat;
			}
		// Serial.print((char)c);
		}
	}
}

// main loop
void loop(void)
{
	uint8_t c;

	if (Serial1.available()) {
		c = Serial1.read();
		gps.nmea_parse(c);
		if (gps.time_ready) {
			gps.time_ready = false;
			
			// display the data prepared ealier if there was no interrupt
			if(!timepulse)
				display.display();
			display_date();
			prepare_time();
		}
		if (gps.nsat_ready) {
			gps.nsat_ready = false;
			sats = gps.nsat;
		}
		// Serial.print((char)c);
	}
}

// interrupt handler for timepulse
void pps(void)
{
	// display prepared next second
	display.display();
	timepulse = true;
}

void display_no_signal()
{
	display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
	display.setTextSize(1,2);
	display.setCursor(38, 24);
#if !defined(PL_LANG)
	display.print("GPS Clock");
#else
	display.print("Zegar GPS");
#endif
	display.setTextSize(1,1);
#if !defined(PL_LANG)
	display.setCursor(4, 52);
	display.print("waiting for GPS data");
#else
	display.setCursor(10, 52);
	display.print("czekam na dane GPS");
#endif
}

void display0(uint8_t val)
{
	if (val < 10)	
		display.print(0);
	display.print(val);
}

void display_date(void)
{
	static uint8_t last_day = 255;
	static uint8_t last_month = 255;
	static uint8_t last_year = 255;
	static uint8_t last_sats = 255;
	static char last_valid = 'm';

	if (last_day != gps.day) {
		display.setTextSize(1,2);
		display.setCursor(17, 43);

		display0(gps.day);
		display.print('.');
		display0(gps.month);
		display.print('.');
		display0(gps.year);

		last_day = gps.day;
		display.display();
	}
	if (last_sats != sats) {
		display.setTextSize(1,2);
		display.setCursor(88, 43);

		display.print("S:");
		display0(sats);

		last_sats = sats;
		display.display();
	}
	if (last_valid != gps.valid) {
		display.setTextSize(1,2);
		display.setCursor(71, 43);

		display.print((gps.valid=='A')?"  ":"!!");

		last_valid = gps.valid;
		display.display();
	}
}

void prepare_time(void) {
	static uint8_t next_sec = 0;
	static uint8_t next_min = 0;
	static uint8_t next_hour = 0;

	uint8_t hour;


	// prepare next time but do not display it, wait for pps interrupt
	next_sec = gps.second + 1;
	if (next_sec >= 60) next_sec = 0;
	
	next_min = gps.minute;
	next_hour = gps.hour;
	
	display.setTextSize(2,4);
	display.setCursor(89, 7);
	display0(next_sec);

	if (next_sec == 0)
		if (++next_min >= 60) {
			next_min = 0;
			next_hour++;
			if (next_hour >= 24 )
				next_hour = 0;
		}

	if (next_min != prev_min) {
		prev_min = next_min;

		display.setCursor(53, 7);
		display0(next_min);
		display.print(':');

		if (next_hour != prev_hour) {
			prev_hour = next_hour;

			hour = next_hour + timezone + dst(next_hour + timezone, gps.day, gps.month, gps.year);
			if(hour >= 24) hour -= 24;
			if(hour < 0) hour += 24;
			display.setCursor(17, 7);
			display0(hour);
			display.print(':');
		}
	}
	timepulse = false;
}


// adjust DST

// dst for Europe
// returns integer instead of boolean
// for easier hour incementing
int dst(int hour, int day, int month, int year)
{
	//printf("h@dst=%d\n",hour);
	int dayofchange;
	// before March: no DST
	if (month < 3)
		return 0;
	
	// for March and October: check for last sunday od month
	if (month == 3) {
		if (day < 25)
			return 0;
		dayofchange = 31 - zeller(31, month, year);
		if (((day == dayofchange) && (hour >= 2)) || (day > dayofchange))
			// after 2 A.M. of last Sunday of March
			return 1;
		return 0;
	}
	
	// after March to September: DST is active
	if ((month < 10))
		return 1;
	
	// for March and October: check for last sunday od month
	if (month == 10) {
		if (day < 25)
			return 1;
		dayofchange = 31 - zeller(31, month, year);
		if (((day == dayofchange) && (hour >= 2)) || (day > dayofchange))
			// before 3 A.M. of last  Sunday of October
		return 0;
		return 1;
	}
	// after September
	return 0;
}

// Zeller's algorithm. returns day of week from given date.
// from 0 = Sunday to 6 = Saturday
int zeller(int day, int month, int year)
{
	int h;

	if (month < 3) {
		month += 12;
		year--;
	}
	
	h = ( day
		+ 13 * (month + 1) / 5
		+ year
		+ year / 4
		- year / 100
		+ year / 400
		- 1 // adjustment for Sunday = 0
		) % 7;
	return h;
}
