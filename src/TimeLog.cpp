#include <Arduino.h> 
#include <EEPROM.h>
#include "Time.h"


#include "TimeLog.h"

// Log the time taken to be awake
TimeLog::TimeLog()
{
	memset(&m_data, 0, sizeof(TimeLogData));
}


bool TimeLog::begin()
{
	bool result = false;
	if (EEPROM.read(0) == TIME_LOG_EEPROM_SIG) {
		read(m_data, 1);
		result = true;
	}
	else {
		EEPROM.write(0, TIME_LOG_EEPROM_SIG);
		write(m_data, 1);
	}
	m_isStart = true;
	return result;
}

bool TimeLog::update(tmElements_t &tm)
{

	bool result = false;
	if ( ! IsTimeEqual(m_data.stop, tm) ) {
		setTime(m_data.stop, tm);
		if ( m_isStart ) {
			m_year = tm.Year;
			setTime(m_data.start, tm);
			m_isStart = false;
		}
		write(m_data, 1);
		result = true;
	}
	return result;
}

void TimeLog::printTime(char *buffer, TimeLogTime &time)
{
	sprintf(buffer, "%02d:%02d %02d/%02d", time.hour, time.minute, time.day, time.month);
}

TimeLogTime & TimeLog::getStartTime()
{
	return m_data.start;
}
TimeLogTime & TimeLog::getStopTime()
{
	return m_data.stop;
}

void TimeLog::setTime(TimeLogTime &time, tmElements_t &tm)
{
	time.hour = tm.Hour;
	time.minute= tm.Minute;
	time.day = tm.Day;
	time.month = tm.Month;
}

bool TimeLog::IsTimeEqual(TimeLogTime &time, tmElements_t &tm)
{
	return ( time.minute == tm.Minute && 
			time.hour == tm.Hour &&
			time.day == tm.Day && 
			time.month == tm.Month );
}

unsigned int TimeLog::durationMinutes()
{
	time_t startTimeSeconds, stopTimeSeconds;
	
	startTimeSeconds = getTimeAsSeconds(m_data.start);
	stopTimeSeconds = getTimeAsSeconds(m_data.stop);
	return (stopTimeSeconds - startTimeSeconds) / 60 ;
}

time_t TimeLog::getTimeAsSeconds(TimeLogTime &time)
{
	tmElements_t tm;
	tm.Second = 0;
	tm.Minute = time.minute;
	tm.Hour = time.hour;
	tm.Day = time.day;
	tm.Month = time.month;
	tm.Year = m_year;
	return makeTime(tm);
}

void TimeLog::write(TimeLogData &data, int offset)
{
	uint8_t *ptr = (uint8_t *) &data;
	for ( int i = 0; i < sizeof(TimeLogData); i++ ) {
		EEPROM.write(i + offset, ptr[i]);
	}
}

void TimeLog::read(TimeLogData &data, int offset)
{
	uint8_t *ptr = (uint8_t *) &data;
	for ( int i = 0; i < sizeof(TimeLogData); i++ ) {
		ptr[i] = EEPROM.read(i + offset);
	}
}

