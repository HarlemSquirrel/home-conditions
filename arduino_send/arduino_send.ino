/*
 HarlemSquirrel.github.io
 */

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

//
// Hardware configuration
//

// Set up nRF24L01 radio on SPI bus plus pins 7 & 8
RF24 radio(7,8);

// Set up temperature sensor
const int sensorPin = A0;
const float baselineTemp = 24.0;

//
// Topology
//

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

//
// Payload
//

//char receive_payload[max_payload_size+1]; // +1 to allow room for a terminating NULL char
char receive_payload[25];

void setup(void)
{
  Serial.begin(9600);

  // Temperature sensor setup
  for(int pinNumber = 2; pinNumber < 5; pinNumber++){
    pinMode(pinNumber, OUTPUT);
    digitalWrite(pinNumber, LOW);
  }

  //
  // Setup and configure rf radio
  //

  radio.begin();

  // enable dynamic payloads
  radio.enableDynamicPayloads();

  // optionally, increase the delay between retries & # of retries
  radio.setRetries(5,15);

  //
  // Open pipes to other nodes for communication
  //

  // This simple sketch opens two pipes for these two nodes to communicate
  // back and forth.
  // Open 'our' pipe for writing
  // Open the 'other' pipe for reading, in position #1 (we can have up to 5 pipes open for reading)

  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1,pipes[1]);

  //
  // Start listening
  //

  radio.startListening();

  //
  // Dump the configuration of the rf unit for debugging
  //

  radio.printDetails();
}

void loop(void)
{
  //
  // Read temperature and send it out.
  //

  int sensorVal = analogRead(sensorPin);

  Serial.print("Sensor Value: ");
  Serial.print(sensorVal);
  
  // convert the ADC reading to voltage
  float voltage = (sensorVal/1024.0) * 5.0;
  
  Serial.print(", Volts: ");
  Serial.print(voltage);

  // convert the voltage to temperature in degrees
  float temp_c = (voltage - .5) * 100;

  Serial.print(", degrees C: ");
  Serial.print(temp_c);

  float temp_f = (temp_c * 1.8) + 32.0;

  Serial.print(", degrees F: ");
  Serial.println(temp_f);

  // Build json as char array
  char json[32];
  char temp_c_char[6];
  char temp_f_char[6];
  dtostrf(temp_c,4,1,temp_c_char);
  dtostrf(temp_f,4,1,temp_f_char);
  sprintf(json, "{\"tempc\":\"%s\",\"tempf\":\"%s\"}", temp_c_char, temp_f_char);

  // First, stop listening so we can talk.
  radio.stopListening();

  // Take the time, and send it.  This will block until complete
  Serial.println("Now sending temperature JSON...");
  Serial.println(json);
  radio.write( json, sizeof(json));

  // Now, continue listening
  radio.startListening();

  // Wait here until we get a response, or timeout
  unsigned long started_waiting_at = millis();
  bool timeout = false;
  while ( ! radio.available() && ! timeout )
    if (millis() - started_waiting_at > 1000 )
      timeout = true;

  // Describe the results
  if ( timeout )
  {
    Serial.println(F("Failed, response timed out."));
  }
  else
  {
    // Grab the response, compare, and send to debugging spew
    uint8_t len = radio.getDynamicPayloadSize();
    
    // If a corrupt dynamic payload is received, it will be flushed
    if(!len){
      return;
    }
    
    radio.read( receive_payload, len );

    // Put a zero at the end for easy printing
    receive_payload[len] = 0;

    // Spew it
    Serial.print(F("Got response size="));
    Serial.print(len);
    Serial.print(F(" value="));
    Serial.println(receive_payload);
  }

  // Try again 10s later
  delay(10000);
}
