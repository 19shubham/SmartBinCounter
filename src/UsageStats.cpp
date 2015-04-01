#include <Arduino.h> 
#include "EEPROM.h"
#include "Time.h"

#include "UsageStats.h"

// Record the usage stats in EEPROM

UsageStats::UsageStats()
{
	memset(&m_data, 0, sizeof(UsageStatsData));
	m_isChanged = false;
}

// start the usage stats
bool UsageStats::begin(tmElements_t tm)
{
	bool result = false;
	if ( EEPROM.read(0) == USAGE_DATA_EEPROM_SIG ) {
		readData(m_data, tm.Day);
		if ( m_data.month != tm.Month) {
			clearData(m_data, tm.Month, tm.Day);		
		}
		result = true;
	}
	else {
		EEPROM.write(0, USAGE_DATA_EEPROM_SIG);
		for ( uint8_t day = 1; day <= USAGE_DATA_MAX_COUNT; day ++ ) {
			clearData(m_data, tm.Month, day);
			writeData(m_data, day);
		}
	}
	return result;
}

// update the current usage stats
void UsageStats::update(tmElements_t tm)
{
	if ( tm.Day != m_data.day || tm.Month != m_data.month ) {
		writeData(m_data, m_data.day);
		clearData(m_data, tm.Month, tm.Day);
		m_isChanged = true;		
	}
	if ( m_isChanged ) {
		writeData(m_data, m_data.day);
		m_isChanged = false;
	}
}

// set counter A value
void UsageStats::setCounterA(unsigned int value)
{
	m_data.countA = value;
	m_isChanged = true;
}

// set counter B value
void UsageStats::setCounterB(unsigned int value)
{
	m_data.countB  = value;
	m_isChanged = true;
}
void UsageStats::setCounterLid(unsigned int value)
{
	m_data.countLid = value;
	m_isChanged = true;
}

// print history to the buffer
void UsageStats::printHistory(char *buffer, uint8_t day)
{
	UsageStatsData data;
	readData(data, day );
	sprintf(buffer, "%02d/%02d: %d %d %d", data.day, data.month, data.countA, data.countB, data.countLid);
}


// get raw usage stats data for today

UsageStatsData& UsageStats::getData()
{
	return m_data;
}

// usage read data from EEPROM
void UsageStats::readData(UsageStatsData &data, uint8_t day)
{
	uint8_t index = day;
	if ( index < USAGE_DATA_MAX_COUNT ) {
		int position = USAGE_DATA_EEPROM_DATA_OFFSET + ( sizeof(UsageStatsData) * index );
		readDataAtOffset(data, position);
		data.day = day;
	}
}

// write usage data to EEPROM
void UsageStats::writeData(UsageStatsData &data, uint8_t day)
{
	uint8_t index = day;
	if ( index < USAGE_DATA_MAX_COUNT ) {
		data.day = day;
		int position = USAGE_DATA_EEPROM_DATA_OFFSET + ( sizeof(UsageStatsData) * index );
		writeDataAtOffset(data, position);
	}
}

// clear the data struct with the day/month
void UsageStats::clearData(UsageStatsData &data, uint8_t month, uint8_t day)
{
	memset(&data, 0, sizeof(UsageStatsData));
	data.day = day;
	data.month = month;
}

// write the usage data at offset to EEPROM
void UsageStats::writeDataAtOffset(UsageStatsData &data, int offset)
{
	uint8_t *ptr = (uint8_t *) &data;
	for ( int i = 0; i < sizeof(UsageStatsData); i++ ) {
		EEPROM.write(i + offset, ptr[i]);
	}
}

// read the data at offset from EEPROM
void UsageStats::readDataAtOffset(UsageStatsData &data, int offset)
{
	uint8_t *ptr = (uint8_t *) &data;
	for ( int i = 0; i < sizeof(UsageStatsData); i++ ) {
		ptr[i] = EEPROM.read(i + offset);
	}
}

