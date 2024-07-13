/*
	GPSParser.cpp - simple GPS parser 
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

#include "GPSParser.h"

GPSParser::GPSParser()
:	state(NMEA_WAIT)
,	id(NMEA_ID_OTHER)
,	checksum(0)
,	token_number(0)
,	buf_pos(NMEA_BUFF_SIZE)
,	time_message_ready(false)
,	hour(0)
,	minute(0)
,	second(0)
,	sub_sec(0)
,	day(0)
,	month(0)
,	year(0)
#if defined(USE_GGA)
, 	nsat_message_ready(false)
, 	nsat(0)
#endif
{
	clear_buf();
}

void GPSParser::reset(void)
{
	clear_buf();
	state = NMEA_WAIT;
	id = NMEA_ID_OTHER;
	checksum = 0;
	token_number = 0;

	time_message_ready = false;
	
	hour   = 0;
	minute = 0;
	second = 0;
	sub_sec = 0;
	day = 0;
	month = 0;
	year = 0;
#if defined(USE_GGA)
	nsat_message_ready = false;
	nsat = 0;
#endif
	// zeroize buf
	// buf_pos = NMEA_BUFF_SIZE;
}

inline uint8_t GPSParser::str2decimal(char *buf)
{
	return 10 * (buf[0] - '0') + (buf[1] - '0');
}

//	GGA:
//
//	1. UTC of position		hhmmss.ss
//	2  Latitude				llll.lllllll
//	3. Lat direction		N or S
//	4. Longitude			lllll.lllllll
//	5. Long direction		E or W
//	6. GPS quality			0-5
//	7. Satellites			XX (09)
//	8. HDOP					XX.X	Variable
//	9. Alt. geoid height	(-)X.XX
//	10. Unit of 9.			M
//	11. Geoidal separation	(-)X.XX	Variable
//	12. Unit of 11.			M
//	13. Age of differential empty
//	14. Diff. ref. station	empty

#ifdef USE_GGA
void GPSParser::data_gga_reader()
{
	if (buf[0] == '\0')
		return;
	switch (token_number) {
		case 6:
			nsat = str2decimal(&buf[0]);
		break;
	}
}
#endif

// RMC:
//
//	1. UTC time				hhmmss.ss
//	2. Status				A or V for invalid
//	3. Latitude				llll.lllll11
//	4. Lat direction		N or S
//	5. Longitude			lllll.lllllll
//	6. Long direction		E or W
//	7. Speed over ground	X+.XX
//	8. Course over ground	X+.XX
//	9. Date					ddmmyy
//	10. Magnetic variation	X+.XX
//	11. Magnetic variation	E/W	E if variation positive
//	12.	Mode indicator		A: Autonomous D: Differential

void GPSParser::data_rmc_reader()
{
	if (buf[0] == '\0')
		return;
	switch (token_number) {
		case 0:
			hour = str2decimal(&(buf[0]));
			minute = str2decimal(&(buf[2]));
			second = str2decimal(&(buf[4]));
			sub_sec = str2decimal(&(buf[7]));
			break;
		case 1:
			valid = buf[0];
			break;
		case 8:
			day = str2decimal(&(buf[0]));
			month = str2decimal(&(buf[2]));
			year = str2decimal(&(buf[4]));
			break;
	}
}


// Address field
// The address field starts with “$” followed by the talker ID and a sentence identifier. The used talker IDs are:
// GP for GPS only solutions
// GL for GLONASS only solutions
// GA for GALILEO only solutions
// GN for multi GNSS solutions
//The used sentence identifiers are:
// GGA – Global Positioning System Fix Data
// VTG – Course over Ground and Ground Speed
// GSA – GNSS DOP and Active Satellites
// GSV – GNSS Satellites in View
// RMC – Recommended Minimum Specific GNSS Data
// ZDA – Time and Date
// PASHR – Attitude Data

nmea_id GPSParser::check_message_id() {
	if (buf[0] != 'G')
		return NMEA_ID_OTHER;
	
	if (buf[1] != 'A' && buf[1] != 'L' && buf[1] != 'N' && buf[1] != 'P')
		return NMEA_ID_OTHER;
	
	if (buf[2] == 'R' && buf[3] == 'M' && buf[4] == 'C')
		return NMEA_ID_RMC;
	
#ifdef USE_GGA
	if (buf[2] == 'G' && buf[3] == 'G' && buf[4] == 'A')
		return NMEA_ID_GGA;
#endif
	
	return NMEA_ID_OTHER;
}

void GPSParser::clear_buf() {
	while(buf_pos)
		buf[--buf_pos] = '\0';
}

inline uint8_t hexchar(char a)
{
	if (a >= 'a' && a <= 'f')
		return a - 'a' + 10;
	if (a >= 'A' && a <= 'F')
		return a - 'A' + 10;
	return a - '0';
}

void GPSParser::nmea_parse(char c)
{
	static uint8_t r_checksum;
	
	// start parsing if char is $ (i.e. beginning of NMEA message)
	if (c == '$') {
		reset();
		state = NMEA_ID;
		return;
	}
	
	// do nothing in wait state
	if (state == NMEA_WAIT) {
		return;
	}
	
	// end of message data. start checksum verification
	if (c == '*') {
		state = NMEA_CHECKSUM;
		clear_buf();
		return;
	}
	

	if (state == NMEA_CHECKSUM) {
		// read checksum until EOL
		if ((c != '\r') && (c != '\n')) {
			r_checksum <<= 4;
			r_checksum |= hexchar(c);
			return;
		}

		// at EOL
		if (checksum == r_checksum) {
			if (id == NMEA_ID_RMC)
				time_message_ready = true;
#ifdef USE_GGA
			if (id == NMEA_ID_GGA)
				nsat_message_ready = true;
#endif
		}
		r_checksum = 0;
		state = NMEA_WAIT;

		return;
	}
	
	// calculate checksum from all message data
	checksum ^= (uint8_t)c;
	
	// read NMEA ID
	if (state == NMEA_ID) {
		if (c != ',') {
			buf[buf_pos++] = c;
			return;
		}

		id = check_message_id();
		if (id != NMEA_ID_OTHER)
			state = NMEA_DATA;
		clear_buf();
		return;
	}
	
	// read NMEA data
	if (state == NMEA_DATA) {
		if (c != ',') {
			buf[buf_pos++] = c;
			return;
		}
#ifdef USE_GGA
		switch (id) {
			case NMEA_ID_RMC:
				data_rmc_reader();
				break;
			case NMEA_ID_GGA:
				data_gga_reader();
				break;
			default:
				break;
		}
#else
		data_rmc_reader();
#endif
		token_number++;
		clear_buf();
		if (c == '*')
			state = NMEA_CHECKSUM;
		return;
	}
}
