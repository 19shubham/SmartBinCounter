

#ifndef _BIN_SENSOR

#define _BIN_SENSOR



class BinSensor
{

	typedef enum {
		Centimeters = 1,
		Inches
	} DistanceUnit;
	
	
	public:
		BinSensor(uint8_t triggerPin, uint8_t echoPin);

		void begin(long maxDistance, int differenceAmount, uint8_t maxTimeout, int maxCounter, DistanceUnit distanceUnit = Centimeters);
		long getDistance();
		bool isHit();
		long getHitDistance();
		int getCounter();
		void setCounter(int value);
		void incCounter(int value = 1);
		
	

	private:
		uint8_t m_triggerPin;				// pin to start the trigger event
		uint8_t m_echoPin;					// pin to receive the echo distance
		int m_counter;						// counter value
		int m_maxCounter;					// max counter value
		long m_maxDistance;					// max distance allowed
		long m_lastDistance;				// last recorded distance
		long m_hitDistance;					// min distance required for a hit
		int m_differenceAmount;				// difference recorded on last hit
		uint8_t m_maxTimeout;				// max timeout on number of 'isHit' calls
		uint8_t m_timeout;					// timeout value
		DistanceUnit m_distanceUnit;		// unit of distance measurement
};



#endif				// _BIN_SENSOR