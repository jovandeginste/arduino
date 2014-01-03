#include <JeeLib.h>
 
#define myNodeID 28      // RF12 node ID in the range 1-30
#define network 210      // RF12 Network group
#define freq RF12_868MHZ // Frequency of RFM12B module
 
//########################################################################################################################
//Data Structure to be received
//########################################################################################################################
 
 typedef struct {
    int supplyV;    // Supply voltage
    int temp;   // Temperature reading
    int temp2;  // Temperature 2 reading
    int temp3;  // Temperature 3 reading
    int temp4;  // Temperature 4 reading
 } Payload;
 
 Payload temptx;
 
 void setup(){
   Serial.begin(57600);
   Serial.print("Start");
   rf12_initialize(myNodeID,freq,network);
 }
 
 void loop(){
   if (rf12_recvDone() && rf12_crc == 0){
     int numSensors = rf12_len/2 - 1;
     const Payload* p = (const Payload*) rf12_data;
     Serial.println();
     Serial.print("Voltage: ");
     Serial.print(p->supplyV / 100.);
     if (numSensors>0) Serial.print(" Sensor1: ");
     if (numSensors>0) Serial.print(p->temp / 100.);
     if (numSensors>1) Serial.print(" Sensor2: ");
     if (numSensors>1) Serial.print(p->temp2 / 100.);
     if (numSensors>2) Serial.print(" Sensor3: ");
     if (numSensors>2) Serial.print(p->temp3 / 100.);
     if (numSensors>3) Serial.print(" Sensor4: ");
     if (numSensors>3) Serial.print(p->temp4 / 100.);
   }
 }
