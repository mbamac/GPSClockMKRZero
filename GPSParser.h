/*
	GPSParser.h - simple GPS parser 
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

#include <stdio.h>
#include <stdlib.h>

#define USE_GGA

#define NMEA_BUFF_SIZE 64

typedef enum {
	NMEA_WAIT=0,
	NMEA_ID,
	NMEA_DATA,
	NMEA_CHECKSUM
} nmea_state;

typedef enum {
	NMEA_ID_OTHER,
	NMEA_ID_RMC,
	NMEA_ID_GGA
} nmea_id;


class GPSParser {
private:
	nmea_state state;
	nmea_id id;
	uint8_t checksum;
	uint8_t token_number;
	uint8_t buf_pos;
	char buf[NMEA_BUFF_SIZE];
	
public:
	// parsed stuff
	
	bool time_message_ready;
	// from RMC
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	uint8_t sub_sec;
	uint8_t day;
	uint8_t month;
	uint8_t year;
	char valid;
#ifdef USE_GGA
	bool nsat_message_ready;
	// from GGA
	uint8_t nsat;
#endif
public:
	GPSParser();
	void nmea_parse(char c);
private:
	void reset(void);
	uint8_t str2decimal(char *buf);
	void clear_buf(void);
	nmea_id check_message_id(void);
	void data_rmc_reader(void);
#ifdef USE_GGA
	void data_gga_reader(void);
#endif
	
};
