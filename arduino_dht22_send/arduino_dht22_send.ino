/*
 HarlemSquirrel.github.io
 */

#include <SPI.h>

// https://github.com/nRF24/RF24
#include "RF24.h"

// https://github.com/adafruit/DHT-sensor-library
#include "DHT.h"

// https://github.com/n0m1/Sleep_n0m1
#include <Sleep_n0m1.h>

// Sleep_n0m1 configuration
Sleep sleep;
unsigned long sleepTime = 3600000; // 60 minutes

// Serial debugging
bool serialOn = true;

//
// Hardware configuration
//

// Set up nRF24L01 radio on SPI bus plus pins 7 & 8
RF24 radio(7,8);
unsigned long timeoutPeriod = 3000;

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
  if (serialOn) {
    Serial.begin(9600);
    delay(100); // For serial print
  }
  
  // Setup and configure rf radio
  if (serialOn) {
    Serial.print("Starting up radio.");
  }
  radio.begin();
  radio.enableDynamicPayloads();
  // The delay between retries & # of retries
  radio.setRetries(10,15);

  // Open 'our' pipe for writing
  // Open the 'other' pipe for reading, in position #1 (we can have up to 5 pipes open for reading)

  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1,pipes[1]);

  radio.startListening();

  radio.printDetails();

  // Temperature sensor setup
  if (serialOn) {
    Serial.print("Starting up DHT22.");
  }
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
    if (serialOn) {
      Serial.println("Failed to read from DHT sensor!");
    }
    delay(2000); // 2 seconds
    return;
  }

  if (serialOn) {
    Serial.print("Humidity: ");
    Serial.print(h);
  
    Serial.print("%, degrees C: ");
    Serial.print(temp_c);
  
    Serial.print(", degrees F: ");
    Serial.println(temp_f);
  }

  // Build json as char array
  char json[28];
  char h_char[6];
  char temp_c_char[6];
  //char temp_f_char[6];
  dtostrf(h,4,1,h_char);
  dtostrf(temp_c,4,1,temp_c_char);
  sprintf(json, "{\"h\":\"%s\",\"tempc\":\"%s\"}", h_char, temp_c_char);

  // First, stop listening so we can talk.
  radio.stopListening();

  // Take the time, and send it.  This will block until complete
  if (serialOn) {
    Serial.println("Now sending temperature JSON...");
    Serial.println(json);
  }
  radio.write( json, sizeof(json));

  // Now, continue listening
  radio.startListening();

  // Wait here until we get a response, or timeout
  unsigned long started_waiting_at = millis();
  bool timeout = false;
  while ( ! radio.available() && ! timeout )
    if (millis() - started_waiting_at > timeoutPeriod )
      timeout = true;

  // Describe the results
  if ( timeout )
  {
    if (serialOn) {
      Serial.println(F("Failed, response timed out."));
      Serial.print("Delaying 60 seconds...");
    }
    delay(60000); // 60 seconds
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

    if (serialOn) {
      // Spew it
      Serial.print(F("Got response size="));
      Serial.print(len);
      Serial.print(F(" value="));
      Serial.println(receive_payload);
  
      Serial.print("Sleeping 1 hour...");
    }
    sleep.pwrDownMode();
    sleep.sleepDelay(sleepTime);
  }
}
