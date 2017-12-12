/*
 HarlemSquirrel.github.io
 */

#include <SPI.h>

// https://github.com/nRF24/RF24
//#include "nRF24L01.h"
#include "RF24.h"

// https://github.com/adafruit/DHT-sensor-library
#include "DHT.h"

//
// Hardware configuration
//

// Set up nRF24L01 radio on SPI bus plus pins 7 & 8
RF24 radio(7,8);

// Set up DHT22 temperature sensor
#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

//
// Topology
//

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

//
// Payload
//

//char receive_payload[max_payload_size+1]; // +1 to allow room for a terminating NULL char
char receive_payload[32];

void setup(void)
{
  Serial.begin(9600);

  // Setup and configure rf radio
  Serial.print('Starting up radio.');
  radio.begin();
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

  radio.startListening();

  radio.printDetails();

  // Temperature sensor setup
  Serial.print('Starting up DHT22.');
  dht.begin();
}

void loop(void)
{
  // Read humidity and temperature
  float h = dht.readHumidity();
  float temp_c = dht.readTemperature();
  float temp_f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(temp_c) || isnan(temp_f)) {
    Serial.println("Failed to read from DHT sensor!");
    delay(2000); // 2 seconds
    return;
  }
  
  Serial.print("Humidity: ");
  Serial.print(h);

  Serial.print("%, degrees C: ");
  Serial.print(temp_c);

  Serial.print(", degrees F: ");
  Serial.println(temp_f);

  // Build json as char array
  char json[32];
  char h_char[6];
  char temp_c_char[6];
  //char temp_f_char[6];
  dtostrf(h,4,1,h_char);
  dtostrf(temp_c,4,1,temp_c_char);
  //dtostrf(temp_f,4,1,temp_f_char);
  sprintf(json, "{\"h\":%s,\"tempc\":%s}", h_char, temp_c_char);

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
