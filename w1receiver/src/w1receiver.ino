/// @dir RF12demo
/// Configure some values in EEPROM for easy config of the RF12 later on.
// 2009-05-06 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

#include <OneWire.h>
#include <RF12.h>
#include <Ports.h>
#include <avr/sleep.h>

#define LED_PIN     9   // activity LED, comment out to disable

static void activityLed (byte on) {
#ifdef LED_PIN
	pinMode(LED_PIN, OUTPUT);
	digitalWrite(LED_PIN, !on);
#endif
}

void setup() {
	Serial.begin(9600);

	rf12_initialize(1, RF12_868MHZ, 33);
	Serial.println("Temp receiver startup");
	activityLed(0);
}

void loop() {
	if (rf12_recvDone()) {
		byte n = rf12_len;
		if (rf12_crc == 0) {
			activityLed(1);
			Serial.print("OK ");
			Serial.print(millis());
			Serial.print((int) rf12_hdr);
			for (byte i = 0; i < n; ++i) {
				Serial.print(' ');
				Serial.print((int) rf12_data[i]);
			}
			Serial.println();

			// if (RF12_WANTS_ACK == 0) {
			// rf12_sendStart(RF12_ACK_REPLY, 0, 0);
			// }

			activityLed(0);
		}
	}
}
