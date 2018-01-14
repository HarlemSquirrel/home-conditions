# home-conditions

## Wiring

### NRF24L01

Signal | NRF24L01 PIN | Cable COLOR | "Base Module" PIN | Arduino PIN | RPi | RPi 3 PIN
--|--|--|--|--|--|--
GND  | 1 | Brown  | GND | GND      | GND     | 6
VCC  | 2 | Red    | VCC | 3.3 V    | 3.3 V   | 1
CE   | 3 | Orange | CD  | digIO 7  | GPIO 22 | 15
CSN  | 4 | Yellow | CSN | digIO 8  | GPIO 8  | 24
SCK  | 5 | Green  | SCK | digIO 13 | SCKL    | 23
MOSI | 6 | Blue   | MO  | digIO 11 | MOSI    | 19
MISO | 7 | Violet | MI  | digIO 12 | MISO    | 21
IRQ  | 8 | Gray   | IRQ | -        | -       | -

### DHT22

DHT22 (left to right) | Arduino pin
--|--
1 VCC (5V)         | VCC 5V
2 SIGnal           | D2
3 NC Not connected | - / GND
4 GND              | GND



## Running the webserver

```
$ FLASK_APP=webserver.py python -m flask run --host=0.0.0.0
```

## Running as systemd services

### Setup

Copy the service files

```
sudo cp systemd_services/*.service /etc/systemd/system/
```

Enable and start the services

```
sudo systemctl enable home-conditions-receive.service
sudo systemctl enable home-conditions-webserver.service
sudo systemctl start home-conditions-receive.service
sudo systemctl start home-conditions-webserver.service
```

### Viewing the logs

```
journalctl -u home-conditions-receive
journalctl -u home-conditions-webserver
```
