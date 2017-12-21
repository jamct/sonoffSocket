# sonoffSocket
Switching Sonoff Basic and Sonoff S20 (Webserver or MQTT).

## Getting started
Copy sonoffSocket.ino into your Arduino-IDE folder. To use mqtt you have to install external library "PubSubClient". Change your SSID and WiFi-Key and activate MQTT if needed (choose a topic too).
Connect Sonoff Basic or Sonoff S20 to your FTDI-Adapter and flash it using the Arduino-IDE.

## How it works
The socket can be controlled by opening <ip of socket>/ein (to activate) and <ip of socket>/aus to deactivate. If MQTT is enabled, send 1 or 0 to your chosen topic (defined at the beginning).

## Known issues

-

## More information
This repository is part of article "Bastelfreundlich" from German computer magazine "c't". 