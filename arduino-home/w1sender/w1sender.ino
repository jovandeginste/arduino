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


#define ONE_WIRE_BUS 10   // DS18B20 Temperature sensor is connected on D10/ATtiny pin 13
#define ONE_WIRE_POWER 9  // DS18B20 Power pin is connected on D9/ATtiny pin 12

OneWire ds(ONE_WIRE_BUS); // Setup a oneWire instance

byte HighByte, LowByte, TReading, SignBit, Whole, Fract;
byte TempRead, CountRemain;
int stamp;
char StrBuff[50];
char TempAsString[10];
byte TempAsStringLen;
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

  pinMode(ONE_WIRE_POWER, OUTPUT); // set power pin for DS18B20 to output

  PRR = bit(PRTIM1); // only keep timer 0 going

  ADCSRA &= ~ bit(ADEN); 
  bitSet(PRR, PRADC); // Disable the ADC to save power

}

bool Next1820(byte *addr) {
  byte i;
  if ( !ds.search(addr)) {
    ds.reset_search();
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

  ds.reset();
  ds.select(addr);
  ds.write(0x44,1);         // start conversion, with parasite power on at the end

  delay(1000);     // maybe 750ms is enough, maybe not

  present = ds.reset();
  ds.select(addr);
  ds.write(0xBE);         // Read Scratchpad

  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    OneWData[i] = ds.read();
  }
  return OneWData;
}

void rf12_send(byte header, const void* data, byte length) {
  rf12_recvDone();
  if (rf12_canSend()) {
#ifdef LED_PIN
    activityLed(1);
#endif
    rf12_sendStart(header, data, length);
    rf12_sendWait(1);
    delay(50);
#ifdef LED_PIN
    activityLed(0);
#endif
    delay(50);
  }
}

ISR(WDT_vect) { 
  Sleepy::watchdogEvent(); 
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


