/*

	Bin Game with Arduino UNO
Connected To:

	LCD Display		
		Show score for side A and side A, plus any other info.
		
	2 X Ultrasound proximity detectors 
		One sensor for each side, to detect bin usage.
		
	RTC Clock
		Clock to record time of bin usage, and standby time to turn off LCD.
		
	Tilt Switch
		Turn off detectors if the lid is opened, or if the bin falls over.
		
*/

#include <Arduino.h> 

#include <UTFT.h>
#include <Time.h>
#include <DS1307RTC.h>
#include <EEPROM.h>
#include <LowPower.h>
#include <BinSensor.h>

// Modules

// #define MODULE_WRITE_TIME 						// ** WRITE_TIME
// #define MODULE_SAVE_TIME 						// ** MODULE_SAVE_TIME
// #define MODULE_SERIAL_DEBUG						// ** MODULE_SERIAL_DEBUG
#define MODULE_USAGE_STATS						// ** MODULE_USAGE_STATS
#define MODULE_TILT_SWITCH						// ** MODULE_TILT_SWITCH

// Display Modes
#define MODE_CLOCK			1					// Show time and date
#define MODE_COUNTERS		2					// Show the counters
#define MODE_UASGE_STATS	3					// Show the usage stats
#define MODE_PAUSE			4					// Show a pause display with time/date


/* Display Pins

	Display			Arduino
 	1: 5V+		-	5V+
	2: GND		-	GND
	3: RST		-	8
	4: RS		-	7
	5: SDA		-	6
	6: SCL		-	5
	7: CS		-	4
	8: 3.3v
*/	



// LCD Pins
#define PIN_CS		4
#define PIN_SCL		5
#define PIN_SDA		6
#define PIN_RS		7
#define PIN_RST		8


UTFT LCD(ST7735, PIN_SDA, PIN_SCL, PIN_CS, PIN_RST, PIN_RS);  

// Declare which fonts we will be using
extern uint8_t SmallFont[];
extern uint8_t BigFont[];
extern uint8_t SevenSegNumFont[];

// Ultrasonic sensor pins
#define PIN_SIDE_A_TRIGGER		9
#define PIN_SIDE_A_ECHO			10

#define PIN_SIDE_B_TRIGGER		11
#define PIN_SIDE_B_ECHO			12

// Bin Sensor with ultrasonic distance detection 

BinSensor binSensorA(PIN_SIDE_A_TRIGGER, PIN_SIDE_A_ECHO);
BinSensor binSensorB(PIN_SIDE_B_TRIGGER, PIN_SIDE_B_ECHO);




// General options

#define COUNTER_YPOS				30
#define DISTANCE_CHANGE_AMOUNT		20	 			// minimum cm change in range to recognise a hit
#define SENSOR_HIT_TIMEOUT			4 				// number of ticks before looking for another hit
#define MAX_COUNTER_VALUE			999				// max value for the counter
#define MODE_COUNTERS_DELAY			SLEEP_60MS		// delay time between each ultrasonic test
#define MODE_CLOCK_SECOND_DELAY		SLEEP_60MS		// delay between each clock second display
#define MODE_CLOCK_MINUTE_DELAY		SLEEP_8S		// delay between each clock minute display
#define MAX_DISTANCE_ALLOWED		300				// max distance allowed from the ultrasonic sensors
#define MODE_PAUSE_DELAY			SLEEP_8S		// pause sleep time
// only used if module usage stats is enabled
#define MODE_USAGE_STATS_DELAY		SLEEP_4S		// time to sleep while showing stats

// to save power switch to clock mode between theses times below
#define SLEEP_START_HOUR			22				// enter MODE_CLOCK starting from this hour  - comment out to disable 
#define SLEEP_STOP_HOUR				07				// exit MODE_CLOCK after this hour - comment out to disable



// display options
typedef struct {
	uint8_t isSeconds:1;
	uint8_t isSleepZZZ:1;
	uint8_t isDemoModeSwitch:1;
	uint8_t isCounterColors:1;
	uint8_t isUsageModeViewOnStartup:1;
	uint8_t isSensorDistanceVisible:1;
} DisplayOptions;

DisplayOptions displayOptions = { 
	displayOptions.isSeconds = 0, 
	displayOptions.isSleepZZZ = 1, 
	displayOptions.isDemoModeSwitch = 0,
	displayOptions.isCounterColors = 1,
	displayOptions.isUsageModeViewOnStartup = 0,
	displayOptions.isSensorDistanceVisible = 0,
};

// display time cache values
typedef struct {
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
} DisplayTime;

// Variables
char buffer[120];					// the main text buffer
tmElements_t tm;					// current time from RTC
DisplayTime displayTime;			// what time is being shown
bool isCounterDisplayInvalid;		// redraw the counters?
uint8_t displayMode;				// current display mode



#ifdef MODULE_TILT_SWITCH					// ** MODULE_TILT_SWITCH

// Tilt switch
#define PIN_TILT_SWITCH			3

#define TILT_ACTIVE_TIMEOUT		5

uint8_t tiltActiveCounter;
boolean isTiltSwitch;

// #define DEBUG_TILT_SWITCH						// ** DEBUG_TILT_SWITCH

void setTiltSwitch(boolean value) 
{
	isTiltSwitch = value;
	if ( value ) {
		if ( displayMode != MODE_PAUSE ) {
			setMode(MODE_PAUSE);
		}
	}
	else {
		if ( displayMode == MODE_PAUSE ) {
			setMode(MODE_COUNTERS);
		}
	}
}

void checkTiltSwitch()
{
	int value = digitalRead(PIN_TILT_SWITCH);

#ifdef DEBUG_TILT_SWITCH					// ** DEBUG_TILT_SWITCH
	sprintf(buffer,"T%d", value);
	LCD.setFont(SmallFont);	
	LCD.print(buffer, 140, 0);
#endif					
										// ** DEBUG_TILT_SWITCH
						
	if ( value == 1 ) {
		if ( !isTiltSwitch ) {
			tiltActiveCounter ++;
			if ( tiltActiveCounter >= TILT_ACTIVE_TIMEOUT ) {
				tiltActiveCounter = 0;
				setTiltSwitch(true);
			}
		}
	}
	else {	
		tiltActiveCounter = 0;
		if ( isTiltSwitch ) {
			setTiltSwitch(false);
		}
	}
}

#endif										// ** MODULE_TILT_SWITCH



#ifdef MODULE_USAGE_STATS					// ** MODULE_USAGE_STATS

#include "UsageStats.h"
UsageStats usageStats;

#define USAGE_STATS_SHOW_TIME		3;		// 1 view per page (3 pages) 
// number of cycles = delay 4s * 3 = 18s

typedef struct {
	uint8_t timeout;				// number of ticks to exit mode
	uint8_t pageOffset;				// start day number for page
	uint8_t pageNumber;				// current page number
} UsageStatsInfo;

UsageStatsInfo usageStatsInfo;


void showUsageStats()
{
	bool isPageBreak = false;
	if ( usageStatsInfo.pageOffset == 0 ) {
		usageStatsInfo.pageNumber = 1;
	}
	else {
		usageStatsInfo.pageNumber ++;
	}
	uint8_t x = 0;
	uint8_t y = 0;
	LCD.setFont(SmallFont);	
	LCD.setColor(255, 255, 255);
	sprintf(buffer, "%d", usageStatsInfo.pageNumber);
	LCD.print(buffer, LCD.getDisplayXSize() - 10, y);
	for ( uint8_t i = usageStatsInfo.pageOffset; i < 31; i ++  ) {
		usageStats.printHistory(buffer, i + 1);
		LCD.print(buffer, x, y );
		y += 10;
		if ( y + 10 > LCD.getDisplayYSize() ) {
			usageStatsInfo.pageOffset = i;
			isPageBreak = true;
			break;
		}
	}
	if ( ! isPageBreak ) {
		usageStatsInfo.pageOffset = 0;
	}
}


#endif 										// ** MODULE_USAGE_STATS


#ifdef MODULE_SAVE_TIME						// ** MODULE_SAVE_TIME

#include "TimeLog.h"
#define TIME_LOG_START_TIMEOUT				90
TimeLog timeLog;
uint8_t timeLogDelay;

void showTimeLog()
{
	unsigned int minutes = timeLog.durationMinutes();
	int hours;
	
	LCD.setFont(SmallFont);	
	LCD.setColor(255, 255, 255);

	timeLog.printTime(buffer, timeLog.getStartTime());
	LCD.print(buffer, 0, 0);

	timeLog.printTime(buffer, timeLog.getStopTime());
	LCD.print(buffer, 0, 12);
	
	hours = round(minutes / 60);
	minutes = minutes - ( hours * 60 );
	sprintf(buffer, "%d:%02d", hours, minutes); 
	LCD.print(buffer, 100, 0);
}

#endif										// ** MODULE_SAVE_TIME


void showClock(tmElements_t &tm, bool showSeconds, bool showSleep)
{
	bool isDateRefresh = false;
	LCD.setFont(SevenSegNumFont);
	LCD.setColor(255, 255, 255);
	
	if ( displayTime.hour != tm.Hour ) {
		sprintf(buffer, "%02d", tm.Hour);
		LCD.print(buffer, 0, COUNTER_YPOS);
		displayTime.hour = tm.Hour;
		isDateRefresh = true;
	}

	if ( displayTime.minute != tm.Minute ) {
		sprintf(buffer, "%02d", tm.Minute);
		LCD.print(buffer, 100, COUNTER_YPOS);
		displayTime.minute = tm.Minute;
	}
	
	if ( isDateRefresh ) {
		char dayBuffer[4];
		strcpy(dayBuffer, dayShortStr(tm.Wday));
		sprintf(buffer,"%s %2d %s", dayBuffer, tm.Day, monthShortStr(tm.Month));
		LCD.setFont(BigFont);
		LCD.setColor(0, 220, 220);
		LCD.print(buffer, 0, 100);
		
		if ( showSleep ) {
			LCD.setFont(BigFont);
			LCD.setColor(255, 0, 0);
			LCD.print("Z", 70, 20 );
			LCD.print("Z", 90, 10 );
			LCD.print("Z", 110, 0 );
		}		
	}
	if ( showSeconds ) {
		if ( displayTime.second != tm.Second) {
			if ( (tm.Second % 2) == 0 ) {
				LCD.setColor(0, 255, 0);
			}
			else {
				LCD.setColor(0, 0, 0);
			}
			LCD.fillCircle(80, 50, 4);
			LCD.fillCircle(80, 70, 4);
			displayTime.second = tm.Second;
		}
	}
}

void showCounters(unsigned int a, unsigned int b)
{
	LCD.setFont(SevenSegNumFont);
	if ( displayOptions.isCounterColors ) {
		if ( a < b ) {
			LCD.setColor(0, 0, 255);
		}
		else  { 
			if ( a > b ) {
				LCD.setColor(0, 255, 0);
			}
			else {
				LCD.setColor(0, 255, 0);
			}
		}
	}
	else {
		LCD.setColor(255, 255, 255);
	}
	sprintf(buffer, "%03d", a);
	LCD.print(buffer, 0, 0);

	if ( displayOptions.isCounterColors ) {
		if ( b < a ) {
			LCD.setColor(0, 0, 255);
		}
		else  { 
			if ( b > a ) {
				LCD.setColor(0, 255, 0);
			}
			else {
				LCD.setColor(0, 255, 0);
			}
		}
	}
	else {
		LCD.setColor(255, 255, 255);
	}
	sprintf(buffer, "%03d", b);
	LCD.print(buffer, 62, 80);

	if ( displayOptions.isCounterColors ) {
		LCD.setColor(255, 255, 255);
	}
	else {
		LCD.setColor(0, 255, 255);
	}
	for ( int i = -2; i < 2; i ++ ) {
		LCD.drawLine(20 + i, LCD.getDisplayYSize() - 40 + i, LCD.getDisplayXSize() - 20 + i, 40 + i); 
	}
}

void showDistance(int a, int b) 
{
	LCD.setFont(SmallFont);	
	LCD.setColor(0, 255, 255);
	if ( a > 10) {
		sprintf(buffer, "%4d", a);
	}
	else {
		strcpy(buffer, "    ");
	}
	LCD.print(buffer, 0, 60);

	if ( b > 10 ) {
		sprintf(buffer, "%4d", b);
	}
	else {
		strcpy(buffer, "    ");
	}
	LCD.print(buffer,  LCD.getDisplayXSize() - 40, 60);
}

void setMode(uint8_t value)
{
	displayMode = value;
	LCD.clrScr();
	memset(&displayTime, 0xFF, sizeof(DisplayTime));
	isCounterDisplayInvalid = true;
}

void processMode()
{
	switch(displayMode) {
		case MODE_UASGE_STATS:
#ifdef MODULE_USAGE_STATS					// ** MODULE_USAGE_STATS
			showUsageStats();
			LowPower.powerDown(MODE_USAGE_STATS_DELAY, ADC_OFF, BOD_OFF);
			if ( usageStatsInfo.timeout > 0 ) {
				usageStatsInfo.timeout --;
				LCD.clrScr();
			}
			else {
				setMode(MODE_COUNTERS);
			}
#endif										// ** MODULE_USAGE_STATS			
			break;
		case MODE_CLOCK:
			showClock(tm, displayOptions.isSeconds, displayOptions.isSleepZZZ);
			if ( displayOptions.isSeconds ) {
				LowPower.powerDown(MODE_CLOCK_SECOND_DELAY, ADC_OFF, BOD_OFF);
			}
			else {
				LowPower.powerDown(MODE_CLOCK_MINUTE_DELAY, ADC_OFF, BOD_OFF);
			}
		break;
		case MODE_COUNTERS:
			if ( isCounterDisplayInvalid) {
				showCounters(binSensorA.getCounter(), binSensorB.getCounter());
				isCounterDisplayInvalid = false;
			}
			if  ( binSensorA.isHit() ) {
				binSensorA.incCounter();
#ifdef MODULE_USAGE_STATS					// ** MODULE_USAGE_STATS
				usageStats.setCounterA(binSensorA.getCounter());				
#endif										// ** MODULE_USAGE_STATS
				isCounterDisplayInvalid = true;
			}
			LowPower.powerDown(MODE_COUNTERS_DELAY, ADC_OFF, BOD_OFF);

			if  ( binSensorB.isHit() ) {
				binSensorB.incCounter();
#ifdef MODULE_USAGE_STATS					// ** MODULE_USAGE_STATS
				usageStats.setCounterB(binSensorB.getCounter());				
#endif										// ** MODULE_USAGE_STATS
				isCounterDisplayInvalid = true;
			}
			LowPower.powerDown(MODE_COUNTERS_DELAY, ADC_OFF, BOD_OFF);
			if ( displayOptions.isSensorDistanceVisible) {
				showDistance(binSensorA.getHitDistance(), binSensorB.getHitDistance());
			}
			
#ifdef MODULE_USAGE_STATS					// ** MODULE_USAGE_STATS
			usageStats.update(tm);
#endif										// ** MODULE_USAGE_STATS

		break;
		case MODE_PAUSE:
			showClock(tm, false, false);
			LowPower.powerDown(MODE_PAUSE_DELAY, ADC_OFF, BOD_OFF);
		
		break;
	}
}
/************************************************************************************
*
*	setup
*
************************************************************************************/

void setup()
{
#ifdef MODULE_SERIAL_DEBUG					// ** MODULE_SERIAL_DEBUG
	Serial.begin(115200);
	delay(100);
	Serial.println("Ready");
#endif										// ** MODULE_SERIAL_DEBUG

#ifdef MODULE_TILT_SWITCH					// ** MODULE_TILT_SWITCH
	pinMode(PIN_TILT_SWITCH, INPUT_PULLUP);
#endif										// ** MODULE_TILT_SWITCH


	LCD.InitLCD(LANDSCAPE);
	LCD.clrScr();
	LCD.setContrast(84);

#ifdef MODULE_WRITE_TIME					// ** MODULE_WRITE_TIME	
	delay(500);
	tm.Year = 2015;
	tm.Month = 3;
	tm.Day = 30;
	tm.Hour = 15;
	tm.Minute = 35;
	tm.Second = 0;
	tm.Wday = 2;
	RTC.write(tm);
	delay(1000);
#endif										// ** MODULE_WRITE_TIME

	RTC.read(tm);



	binSensorA.begin(MAX_DISTANCE_ALLOWED, DISTANCE_CHANGE_AMOUNT, SENSOR_HIT_TIMEOUT, MAX_COUNTER_VALUE);
	binSensorB.begin(MAX_DISTANCE_ALLOWED, DISTANCE_CHANGE_AMOUNT, SENSOR_HIT_TIMEOUT, MAX_COUNTER_VALUE);


	displayMode = MODE_COUNTERS;
	isCounterDisplayInvalid = true;

#ifdef MODULE_USAGE_STATS					// ** MODULE_USAGE_STATS
	if ( usageStats.begin(tm) ) {
#ifdef MODULE_SERIAL_DEBUG					// ** MODULE_SERIAL_DEBUG
		for ( int day = 1; day <= 31 ; day ++ ) {
			usageStats.printHistory(buffer, day);
			Serial.println(buffer);
		}
#endif										// ** MODULE_SERIAL_DEBUG	
	}
	binSensorA.setCounter(usageStats.getData().countA);
	binSensorB.setCounter(usageStats.getData().countB);
	
	memset(&usageStatsInfo, 0, sizeof(UsageStatsInfo));
	if ( displayOptions.isUsageModeViewOnStartup == 1 ) {
		usageStatsInfo.timeout = USAGE_STATS_SHOW_TIME;
		displayMode = MODE_UASGE_STATS;
	}
	
#endif										// ** MODULE_USAGE_STATS

	

#ifdef MODULE_SAVE_TIME						// ** MODULE_SAVE_TIME
	if ( timeLog.begin() ) {
#ifdef MODULE_SERIAL_DEBUG					// ** MODULE_SERIAL_DEBUG

		timeLog.printTime(buffer, timeLog.getStartTime());
		Serial.print("Start :");
		Serial.println(buffer);

		timeLog.printTime(buffer, timeLog.getStopTime());
		Serial.print("Stop :");
		Serial.println(buffer);

#endif										// ** MODULE_SERIAL_DEBUG
		showTimeLog();
	}
	timeLogDelay = TIME_LOG_START_TIMEOUT;
#endif										// ** MODULE_SAVE_TIME

#ifdef MODULE_TILT_SWITCH					// ** MODULE_TILT_SWITCH
	isTiltSwitch = false;
	tiltActiveCounter = 0;
#endif MODULE_TILT_SWITCH					// ** MODULE_TILT_SWITCH

}

/************************************************************************************
*
*	loop
*
************************************************************************************/
void loop()
{
	RTC.read(tm);
	if ( displayOptions.isDemoModeSwitch && displayMode != MODE_UASGE_STATS) {	
		if ( tm.Second == 30 ) {
			setMode(MODE_CLOCK);
		}
		if ( tm.Second < 10 && displayMode == MODE_CLOCK ) {
			setMode(MODE_COUNTERS);
		}
	}	
	
#ifdef SLEEP_START_HOUR
	if ( tm.Hour >= SLEEP_START_HOUR && displayMode == MODE_COUNTERS ) {
		setMode(MODE_CLOCK);
	}
#endif

#ifdef SLEEP_STOP_HOUR
	if ( tm.Hour <= SLEEP_STOP_HOUR && displayMode == MODE_COUNTERS ) {
		setMode(MODE_CLOCK);
	}
#endif


#ifdef MODULE_TILT_SWITCH					// ** MODULE_TILT_SWITCH
	checkTiltSwitch();
#endif										// ** MODULE_TILT_SWITCH

	processMode();
	
#ifdef MODULE_SAVE_TIME						// ** MODULE_SAVE_TIME
	if ( timeLogDelay == 0 ) {
		if ( timeLog.update(tm) ) {
#ifdef MODULE_SERIAL_DEBUG					// ** MODULE_SERIAL_DEBUG		
			Serial.println("save log time");
#endif										// ** MODULE_SERIAL_DEBUG
			showTimeLog();
		}
	}
	else {
		timeLogDelay --;
	}
#endif										// ** MODULE_SAVE_TIME

}
