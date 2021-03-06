#!/usr/bin/env python

from __future__ import print_function
import json
import time
import sqlite3

# Import the RF24 library to control the nRF24L01+ transceiver module
# https://github.com/nRF24/RF24
from RF24 import *

import RPi.GPIO as GPIO

DB_PATH = 'data/data.sqlite3'
irq_gpio_pin = None
radio = RF24(22, 0);

# Setup for connected IRQ pin, GPIO 24 on RPi B+; uncomment to activate
#irq_gpio_pin = RPI_BPLUS_GPIO_J8_18
#irq_gpio_pin = 24

##########################################
def create_temps_table(cursor):
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS temps(id INTEGER PRIMARY KEY, timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
                           location TEXT, humidity TEXT, temp_c TEXT)
    ''')

def save_temp(data):
    db = sqlite3.connect(DB_PATH)

    cursor = db.cursor()
    create_temps_table(cursor)
    cursor.execute('''INSERT INTO temps(location, humidity, temp_c)
                      VALUES(?,?,?)''', ('home', data['h'], data['tempc']))
    db.commit()
    db.close()

def try_read_data(channel=0):
    if radio.available():
        while radio.available():
            len = radio.getDynamicPayloadSize()
            receive_payload = radio.read(len)
            print('Got payload size={} value="{}"'.format(len, receive_payload.decode('utf-8')))
            decoded_payload = receive_payload.decode('utf-8')
            print('Decoded payload: {}'.format(decoded_payload))
            # Fix extra bytes after decoding.
            # https://stackoverflow.com/questions/14150823/python-json-decode-valueerror-extra-data
            temp_info = json.loads("".join([decoded_payload.rsplit("}" , 1)[0] , "}"]) )
            print('Parsed temperature as {} degrees C'.format(temp_info['tempc']))

            save_temp(temp_info)

            # First, stop listening so we can talk
            radio.stopListening()

            # Send the final one back.
            radio.write(receive_payload)
            print('Sent response.')

            # Now, resume listening so we catch the next packets.
            radio.startListening()

pipes = [0xF0F0F0F0E1, 0xF0F0F0F0D2]
# min_payload_size = 4
# max_payload_size = 44
# payload_size_increments_by = 1
# next_payload_size = min_payload_size
# inp_role = 'none'
# millis = lambda: int(round(time.time() * 1000))

radio.begin()
radio.enableDynamicPayloads()
radio.setRetries(5,15)
radio.printDetails()

print(' ************ Starting up receiver *********** ')
if irq_gpio_pin is not None:
    # set up callback for irq pin
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(irq_gpio_pin, GPIO.IN, pull_up_down=GPIO.PUD_UP)
    GPIO.add_event_detect(irq_gpio_pin, GPIO.FALLING, callback=try_read_data)

radio.openWritingPipe(pipes[1])
radio.openReadingPipe(1,pipes[0])
radio.startListening()

# forever loop
while 1:
    # Pong back role.  Receive each packet, dump it out, and send it back

    # if there is data ready
    if irq_gpio_pin is None:
        # no irq pin is set up -> poll it
        try_read_data()
        time.sleep(8)
    else:
        # callback routine set for irq pin takes care for reading -
        # do nothing, just sleeps in order not to burn cpu by looping
        time.sleep(1000)
