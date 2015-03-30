

#ifndef _TIME_LOG_H
#define _TIME_LOG_H


#include "Time.h"


/*
EEPROM spec

SIG,	timeLog
1,	 	4
Size: 1 + 4 = 5 bytes

*/


#define TIME_LOG_EEPROM_SIG					0x42

typedef struct {
	uint8_t day;			// 1
	uint8_t month;			// 1
	uint8_t hour;			// 1
	uint8_t minute;			// 1
} TimeLogTime;				// Total = 4 bytes

typedef struct {
	TimeLogTime	start;
	TimeLogTime	stop;
} TimeLogData;				// Total = 8 bytes


class TimeLog {
	public:
		TimeLog();
		bool begin();
		void printTime(char *buffer, TimeLogTime &time);
		bool update(tmElements_t &tm);
		TimeLogTime& getStartTime();
		TimeLogTime& getStopTime();
		unsigned int durationMinutes();
		

	protected:
		void setTime(TimeLogTime &time, tmElements_t &tm);
		bool IsTimeEqual(TimeLogTime &time, tmElements_t &tm);
		time_t getTimeAsSeconds(TimeLogTime &time);
		void write(TimeLogData &data, int offset);
		void read(TimeLogData &data, int offset);


	private:
		TimeLogData m_data;
		bool m_isStart;
		uint8_t m_year;


};



#endif				// _TIME_LOG_H