/*
CUBEX v3.0
Thairone S. Loureiro

usando 18.630 bytes (7%) de espaço de armazenamento para programas
Variáveis globais usam 3.985 bytes (48%) de memória dinâmica, deixando 4.207 bytes para variáveis locais. O máximo são 8.192 bytes.
*/


#include <limits.h>
#include <stdio.h>
#include <Wire.h>
#include <NewPing.h>
#include <math.h>


#define TRIGGER_PIN  A1  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     A0  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 200 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.


boolean  novo_obstaculo;


NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.


void setup()
{
  Serial.begin(9600);
  
  Serial.println();        Serial.println();        Serial.println();
  Serial.println("****************************************");

  novo_obstaculo = false;
}



float getSonar() {
 const int numReadings = 10;
 int readings[numReadings];       
 int total = 0;                  
 unsigned int average = 0;       
 float ret;

  //coleta com smoothig
  for (int thisReading = 0; thisReading < numReadings; thisReading++)
    readings[thisReading] = 0;

  for (int readIndex = 0; readIndex < numReadings; readIndex++)
  {
     // subtract the last reading:
    total = total - readings[readIndex];
    // read from the sensor:
    delay(50);
    readings[readIndex] = sonar.ping();    
    total = total + readings[readIndex];       
    average = total / numReadings;    
  }
  
  ret = average / US_ROUNDTRIP_CM; // Convert ping time to distance in cm and print result (0 = outside set distance range)
  return ret;
}


void loop() {
  float dist_obs;
  dist_obs = getSonar();
  Serial.print("distancia: ");
  Serial.println(dist_obs);  
}
