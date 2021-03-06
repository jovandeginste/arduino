/*
   Loosely based on work from Gérard Chevalier, Jan 2011

   Description
   This code runs on both JeeNodes and JeeNode Micros. Connect one
   or multiple DS18B20 sensors (or any other 1-Wire sensor) to it,
   and it should send it's data over RF to a listening device (eg.
   a JeeLink running the w1receiver)

   JeeNodes can be programmed over USB, JeeNode micro's over ISP.

References:
Arduino IDE libs:
 * http://www.pjrc.com/teensy/arduino_libraries/OneWire.zip
 * https://github.com/jcw/jeelib

 Arduino als ISP (for JNµ):
Connect the wires as such:
   Arduino	JNµ
 * A0	->	SCK
 * A3	->	MOSI
 * D4	->	MISO
 * D7	->	RST
 * 3.3v	->	3.3v
 * GND	->	G

 */

#include <OneWire.h>
#include <JeeLib.h>

#define LED_PIN     9   // activity LED, comment out to disable

static void activityLed (byte on) {
#ifdef LED_PIN
	pinMode(LED_PIN, OUTPUT);
	digitalWrite(LED_PIN, !on);
#endif
}

#if defined(__AVR_ATtiny84__)

/*
   JeeNode micro and other ATtiny's

   Connect the sensor to the JNµ as follows:

   DS18B20	JNµ
 * GND	->	GND
 * DQ	->	DIO/PA0 (ATtiny pin 13)
 * Vdd	->	AIO/PA1 (ATtiny pin 12)

 */
#define ONE_WIRE_BUS 10
#define ONE_WIRE_POWER 9
#else

/*
   JeeNode and other ATmega's

   Connect the sensor to the JN as follows:

   DS18B20	JN Port 1
 * GND	->	G
 * DQ	->	D (ATmega pin 4)
 * Vdd	->	+

 */
#define ONE_WIRE_BUS 4
#endif


OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance

byte TempRead, CountRemain;
int stamp;
byte TempBinFormat[15];
byte addr[8];
byte *temp;

void setup() {
	cli();
	CLKPR = bit(CLKPCE);
#if defined(__AVR_ATtiny84__)
	CLKPR = 0; // div 1, i.e. speed up to 8 MHz
#else
	CLKPR = 1; // div 2, i.e. slow down to 8 MHz
#endif
	sei();

#if defined(__AVR_ATtiny84__)
	// power up the radio on JMv3
	bitSet(DDRB, 0);
	bitClear(PORTB, 0);
#endif

	rf12_initialize(7, RF12_868MHZ, 33);
	activityLed(0);
	// see http://tools.jeelabs.org/rfm12b
	rf12_control(0xC040); // set low-battery level to 2.2V i.s.o. 3.1V
	rf12_sleep(0);                          // Put the RFM12 to sleep

#ifdef ONE_WIRE_POWER
	pinMode(ONE_WIRE_POWER, OUTPUT); // set power pin for DS18B20 to output
#endif
}

bool Next1820(byte *addr) {
	byte i;
	if ( !oneWire.search(addr)) {
		oneWire.reset_search();
		return false;
	}

	if ( OneWire::crc8( addr, 7) != addr[7]) {
		return false;
	}
	return true;
}

byte *Get1820Tmp(byte *addr) {
	byte i;
	byte present = 0;
	byte OneWData[12];

	oneWire.reset();
	oneWire.select(addr);
	oneWire.write(0x44,1);         // start conversion, with parasite power on at the end

	delay(1000);     // maybe 750ms is enough, maybe not

	present = oneWire.reset();
	oneWire.select(addr);
	oneWire.write(0xBE);         // Read Scratchpad

	for ( i = 0; i < 9; i++) {           // we need 9 bytes
		OneWData[i] = oneWire.read();
	}

	return OneWData;
}

void rf12_send(byte header, const void* data, byte length) {
	rf12_recvDone();
	if (rf12_canSend()) {
		activityLed(1);
		rf12_sendStart(header, data, length);
		rf12_sendWait(1);
		delay(50);
		activityLed(0);
		delay(50);
	}
}

ISR(WDT_vect) { 
	Sleepy::watchdogEvent(); 
}

void checkAllTemps() {
	// Note that even if you only want to send out packets, you still have to call rf12 recvDone periodically, because
	// it keeps the RF12 logic going. If you don't, rf12_canSend() will never return true.
	while (Next1820(addr)) {
		temp = Get1820Tmp(addr);

		memcpy(&TempBinFormat[0], addr, 8); 
		memcpy(&TempBinFormat[8], temp, 2); 

		rf12_send(1, TempBinFormat, 15);
	}
}

void loop() {
	stamp = (int)millis();
	rf12_sleep(RF12_WAKEUP);
	checkAllTemps();

	rf12_sendWait(2);
	rf12_sleep(RF12_SLEEP);
	Sleepy::loseSomeTime(5000 + stamp - (int)millis());
	delay(5);
}

