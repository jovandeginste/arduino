/*
 HC-SR04 Ping distance sensor]
 VCC to arduino 5v GND to arduino GND
 Echo to Arduino pin 13 Trig to Arduino pin 12
 Red POS to Arduino pin 11
 Green POS to Arduino pin 10
 560 ohm resistor to both LED NEG and GRD power rail
 More info at: http://goo.gl/kJfirstledGl
 Original code improvements to the Ping sketch sourced from Trollmaker.com
 Some code and wiring inspired by http://en.wikiversity.org/wiki/User:Dstaub/robotcar
 */

#define trigPin 13
#define echoPin 12
#define firstled 2

int leds = 0;
int last_beep = 0;

void setup() {
  Serial.begin (9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  for (int i = firstled; i <=11; i++) {
    pinMode(i, OUTPUT);
  }
  leds = 6;
  for (int led = firstled; led < firstled + leds; led++)
    digitalWrite(led, HIGH);
  delay(750);
  for (int led = firstled + leds; led < 12; led++)
    digitalWrite(led,LOW);
}

void loop() {
  long duration, distance;
  digitalWrite(trigPin, LOW);  // Added this line
  delayMicroseconds(2); // Added this line
  digitalWrite(trigPin, HIGH);
  //  delayMicroseconds(1000); - Removed this line
  delayMicroseconds(10); // Added this line
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = (duration/2) / 29.1;
  Serial.print(distance);
  Serial.println(" cm");
  if (distance == 0 || distance > 200) {  // This is where the LED On/Off happens
    leds = 0;
    Serial.println("Out of range");
  } 
  else if (distance < 10) {  // This is where the LED On/Off happens
    leds=6;
  } 
  else if (distance < 25) {  // This is where the LED On/Off happens
    leds=5;
  } 
  else if (distance < 40) {  // This is where the LED On/Off happens
    leds=4;
  }
  else if (distance < 60) {  // This is where the LED On/Off happens
    leds=3;
  }
  else if (distance < 75) {  // This is where the LED On/Off happens
    leds=2;
  }
  else {
    leds=1;
  }

  for (int led = firstled; led < firstled + leds; led++)
    digitalWrite(led, HIGH);
  for (int led = firstled + leds; led < 12; led++)
    digitalWrite(led,LOW);

  delay(100);
}










