# sonoffSocket
Switching Sonoff Basic and Sonoff S20 (Webserver, local button and MQTT).

## Getting started
This project ist split in two parts: sonoffSimple/sonoffSimple.ino is just a simple demo as described [here](https://ct.de/yxgs).

The more advanced software (sonoffSocket.ino) supports MQTT, toggle, status and is actively developed. Just copy sonoffSocket.ino into your Arduino-IDE folder. To use mqtt you have to install external library "PubSubClient". Activate MQTT if needed (choose a topic too) in sonoffSocket.ino.

Create a WiFiConfig.h with this content:
```
//Your Wifi SSID
const char* ssid = "your_ssid";
//Your Wifi Key
const char* password = "your_key";
//Your MQTT Broker
const char* mqtt_server = "your.mqtt.broker.IP";
```

Connect Sonoff Basic or Sonoff S20 to your FTDI-Adapter and flash it using the Arduino-IDE.

## How it works
The socket can be controlled by opening <ip of socket>/on (to activate) and <ip of socket>/off to deactivate. If MQTT is enabled, send 1 or 0 to your chosen topic (defined at the beginning of your code).
Get the current status with <ip of socket>/state and toggle  with <ip of socket>/toggle

## More information
This repository is part of article ["Bastelfreundlich"](https://ct.de/yxgs) from German computer magazine "c't".
