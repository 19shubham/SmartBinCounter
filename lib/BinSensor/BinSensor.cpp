// BinSensor Class

#include <arduino.h>
#include "BinSensor.h"

// Using Ultrasonic range module HC-SR04, to detect items deposited in the bin

// Setup with the trigger and echo pin
BinSensor::BinSensor(uint8_t triggerPin, uint8_t echoPin) :
	m_triggerPin(triggerPin),
	m_echoPin(echoPin)
{
   pinMode(m_triggerPin, OUTPUT);
   pinMode(m_echoPin, INPUT);

}

// begin the sensor
// maxDistance: with maxDistance allowed to be received from the sensor
// differenceAmount: delta difference amount to resolve as in item placed into the bin
// maxTimeout: number of calls to the IsHit before detecting for another item
// maxCounter: max counter value
// distanceUnit: units to measure default to Centimetres
void BinSensor::begin(long maxDistance, int differenceAmount, uint8_t maxTimeout, int maxCounter, DistanceUnit distanceUnit)
{
	m_maxDistance = maxDistance;
	m_differenceAmount = differenceAmount;
	m_maxTimeout = maxTimeout;
	m_maxCounter = maxCounter;
	m_distanceUnit = distanceUnit;
	m_timeout = m_maxTimeout * 4;
	m_lastDistance = 0;
	m_counter = 0;
	m_hitDistance = 0;
}

// return the distance of the object
long BinSensor::getDistance()
{
	long duration;
	digitalWrite(m_triggerPin, LOW);
	delayMicroseconds(2);
	digitalWrite(m_triggerPin, HIGH);
	delayMicroseconds(10);
	digitalWrite(m_triggerPin, LOW);
	duration = pulseIn(m_echoPin, HIGH);
	if ( m_distanceUnit == Centimeters ) {
		return duration / 29 / 2 ;
	}
	return duration / 74 / 2;
}

// return true if the object has detected
bool BinSensor::isHit()
{
	bool result = false;
	long distance = getDistance();
	if ( distance > m_maxDistance ) {
		distance = m_maxDistance;
	}

	m_hitDistance = ( m_lastDistance - distance );
	if ( m_timeout == 0 ) {
		if  ( m_hitDistance > m_differenceAmount ) {
			m_timeout = m_maxTimeout;
			result = true;
		}
	}
	else {
		m_timeout --;
	}
	m_lastDistance = distance;
	return result;
}

// get the last distance detected
long BinSensor::getHitDistance()
{
	return m_hitDistance;
}

// return the current hit counter
int BinSensor::getCounter()
{
	return m_counter;
}

// set the hit counter
void BinSensor::setCounter(int value)
{
	m_counter = value;
	if ( m_counter > m_maxCounter) {
		m_counter = 0;
	}
}

// inc the hit counter
void BinSensor::incCounter(int value)
{
	m_counter += value;
	if ( m_counter > m_maxCounter) {
		m_counter = 0;
	}
}
