/*

Usage Data


EEPROM spec

SIG,	UsageStatData * 31

Size: 1 + (8 * 31) = 249 bytes

*/

#ifndef _USAGE_DATA_H
#define _USAGE_DATA_H


typedef struct {
	uint8_t day;				// 1
	uint8_t month;				// 1
	unsigned int countA;		// 2 
	unsigned int countB;		// 2
	unsigned int countLid;		// 2 
} UsageStatsData;				// Total = 8 bytes

#define USAGE_DATA_EEPROM_SIG					0x42

#define USAGE_DATA_EEPROM_DATA_OFFSET			0x01		

#define USAGE_DATA_MAX_COUNT					31			// max 31 days in a month	

 
class UsageStats {
	public:
		UsageStats();
		bool begin(tmElements_t tm);
		void update(tmElements_t tm);
		void setCounterA(unsigned int value);
		void setCounterB(unsigned int value);
		void setCounterLid(unsigned int value);
		uint8_t historyCount();
		void printHistory(char *buffer, uint8_t day);
		UsageStatsData& getData();

	protected:
		void clearData(UsageStatsData &data, uint8_t month, uint8_t day);
		void readData(UsageStatsData &data, uint8_t day);
		void writeData(UsageStatsData &data, uint8_t day);
		void writeDataAtOffset(UsageStatsData &data, int offset);
		void readDataAtOffset(UsageStatsData &data, int offset);

	private:
		UsageStatsData m_data;
		bool m_isChanged;

};



#endif				// _USAGE_DATA_H