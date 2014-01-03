/*
 * Jeenode remote Temperature sensor for Temperature Data Logger.
 * Remote sensors use dallas DS18B20 or DS18S20.
 *
 * Each Jeenode sends data over RF12 to central datalogger module.
 * See code & comment of Temperature Data Logger project.
 *
 * Gérard Chevalier, Jan 2011
 */

// RF12 frame format: Byte 0 = channel, Byte 1 = temp (bit 7 == sign), Byte 2 = decitemp

// Channel number. In a next release, should be read from config jumpers.
#include <OneWire.h>
#include <RF12.h>
#include <Ports.h>
#include <avr/sleep.h>

#define LED_PIN     9   // activity LED, comment out to disable
//#define DEBUG

static void activityLed (byte on) {
#ifdef LED_PIN
	pinMode(LED_PIN, OUTPUT);
	digitalWrite(LED_PIN, !on);
#endif
}

OneWire  ds(4);

byte HighByte, LowByte, TReading, SignBit, Whole, Fract;
byte TempRead, CountRemain;
word Tc_100;
char StrBuff[50];
char TempAsString[10];
byte TempAsStringLen;
byte TempBinFormat[15];
byte addr[8];
byte count;
byte *temp;

void setup() {
	Serial.begin(57600);

	rf12_initialize(7, RF12_868MHZ, 33);
	Serial.println("Temp transmiter startup");
	activityLed(0);
}

bool Next1820(byte *addr) {
	byte i;
	if ( !ds.search(addr)) {
		Serial.println("No more addresses.");
		ds.reset_search();
		return false;
	}
	Serial.print("R=");
	for( i = 0; i < 8; i++) {
		Serial.print(addr[i], HEX);
		Serial.print(" ");
	}

	if ( OneWire::crc8( addr, 7) != addr[7]) {
		Serial.println("CRC is not valid!");
		return false;
	}
	return true;
}

byte *Get1820Tmp(byte *addr) {
	byte i;
	byte present = 0;
	byte OneWData[12];

	ds.reset();
	ds.select(addr);
	ds.write(0x44,1);         // start conversion, with parasite power on at the end

	deep_sleep(1000);     // maybe 750ms is enough, maybe not

	present = ds.reset();
	ds.select(addr);
	ds.write(0xBE);         // Read Scratchpad

	for ( i = 0; i < 9; i++) {           // we need 9 bytes
		OneWData[i] = ds.read();
	}

#ifdef DEBUG
	// 18B20 default to 12 bits resolution at power up
	LowByte = OneWData[0];
	HighByte = OneWData[1];
	Whole = (HighByte << 4) | (LowByte >> 4);
	Fract = ((LowByte & 0b1111) * 10) / 16;

	// A little bit to heavy to go through a bollean for sign in that case...
	// but keeped as is for compatibility with 18S20 code.
	SignBit = (Whole & 0b10000000) != 0;
	Whole &= 0b01111111;
	sprintf(StrBuff, "1Wire: %c%d.%d",SignBit != 0 ? '-' : '+', Whole, Fract);
	Serial.println(StrBuff);

	if (ds.crc8(OneWData, 8) != OneWData[8]) Serial.println("BAD CRC !");
	TempAsStringLen = sprintf(TempAsString, "%c%d.%d",SignBit != 0 ? '-' : '+', Whole, Fract);
#endif
	return OneWData;
}

void deep_sleep(int milli) {
	long limit = millis() + milli;
	while(millis() <= limit) {
		set_sleep_mode(SLEEP_MODE_IDLE);
		sleep_mode();
	}
}

void rf12_send(byte header, const void* data, byte length) {
#ifdef LED_PIN
	activityLed(1);
#endif
	rf12_recvDone();
	if (rf12_canSend()) {
		rf12_sendStart(header, data, length);
		rf12_sendWait(2);
		deep_sleep(10);
	}
#ifdef LED_PIN
	activityLed(0);
#endif
}
void loop() {
	// Note that even if you only want to send out packets, you still have to call rf12 recvDone periodically, because
	// it keeps the RF12 logic going. If you don't, rf12_canSend() will never return true.
	count = 0;
	while (Next1820(addr)) {
		count += 1;
		temp = Get1820Tmp(addr);
		LowByte = temp[0];
		HighByte = temp[1];
		Whole = (HighByte << 4) | (LowByte >> 4);
		Fract = ((LowByte & 0b1111) * 10) / 16;

		TempBinFormat[0] = count;
		memcpy(&TempBinFormat[1], addr, 8); 

		TempBinFormat[9] = Whole;
		TempBinFormat[10] = Fract;
		rf12_send(0, TempBinFormat, 15);
	}
	TempBinFormat[0] = 0;
	TempBinFormat[1] = count;
	rf12_send(0, TempBinFormat, 2);
	deep_sleep(3000);
}

