#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <Ticker.h>
#include "WiFiConfig.h"

//To use MQTT, install Library "PubSubClient" and switch next line to 1
#define USE_MQTT 0
//If you don`t want to use local button switch to 0
#define USE_LOCAL_BUTTON 1

// Allow WebUpdate
#define USE_WEBUPDATE 1
#if USE_WEBUPDATE == 1
#include <ESP8266HTTPUpdateServer.h>
#endif

#if USE_MQTT == 1
	#include <PubSubClient.h>
	const char* mqtt_in_topic = "socket/switch/set";
  const char* mqtt_out_topic = "socket/switch/status";
	
#endif


// constants
const char* switchOnHtmlLink = "<a href=\"ein\">Einschalten</a>";
const char* switchOffHtmlLink = "<a href=\"aus\">Ausschalten</a>";
const char* switchStateJavascript = "<script>" +
	"function httpGet(theUrl){" +
	"var xmlHttp = new XMLHttpRequest();" +
	"xmlHttp.open( 'GET', theUrl, false );" +
	"xmlHttp.send( null );" +
	"return xmlHttp.responseText;" +
	"}" +
	"function switchState(newState){" +
	"httpGet(newState);" +
	"location.reload();" +
	"}" +
	"</script>";
const char* switchOnHtmlButton = switchStateJavascript + "<p style="display:flex"><button style="flex-grow:1;height:10rem" onclick="switchState('ein')">Einschalten</button></p>";
const char* switchOffHtmlButton = switchStateJavascript + "<p style="display:flex"><button style="flex-grow:1;height:10rem" onclick="switchState('aus')">Ausschalten</button></p>";

ESP8266WebServer server(80);
#if USE_WEBUPDATE == 1
ESP8266HTTPUpdateServer httpUpdater;
#endif

#if USE_MQTT == 1
  WiFiClient espClient;
  PubSubClient client(espClient);
  bool status_mqtt = 1;
#endif

int gpio13Led = 13;
int gpio12Relay = 12;

bool lamp = 0;
bool relais = 0;

//Test  GPIO 0 every 0.1 sec
Ticker checker;
bool status = 1;

// Timer
unsigned long stopAt = 0;
unsigned long startAt = 0;
// "0" means timer not running
// reference is millis()

void setup(void){
  Serial.begin(115200); 
  delay(5000);
  Serial.println("");

 
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
 
  pinMode(gpio13Led, OUTPUT);
  digitalWrite(gpio13Led, lamp);
  
  pinMode(gpio12Relay, OUTPUT);
  digitalWrite(gpio12Relay, relais);

  // Wait for WiFi
  Serial.println("");
  Serial.print("Verbindungsaufbau mit:  ");
  Serial.println(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if(lamp == 0){
       digitalWrite(gpio13Led, 1);
       lamp = 1;
     }else{
       digitalWrite(gpio13Led, 0);
       lamp = 0;
     }
    Serial.print(".");
  }
  Serial.println("Verbunden");
  Serial.print("IP-Adresse: ");
  Serial.println(WiFi.localIP());
  digitalWrite(gpio13Led, HIGH);


#if USE_MQTT == 1  
  client.setServer(mqtt_server, 1883);
  client.setCallback(MqttCallback);
#endif

  server.on("/", [](){
    String msg = "<H1>Sonoff</H1>\n";
    msg += "uptime " + String(millis() / 1000) + "s<br />\n";
    msg += "<H2>Befehle</H2>\n<p>";
    msg += switchOnHtmlLink + "<br />\n";
    msg += switchOffHtmlLink + "<br />\n";
    msg += "<a href=\"toggle\">Umschalten</a><br />\n";
    msg += "<a href=\"state\">Schaltstatus</a><br />\n";
    msg += "<a href=\"timer\">Timer</a><br />\n";
#if USE_WEBUPDATE == 1
    msg += "<a href=\"update\">Update</a><br />\n";
#endif
    msg += "</p>\n";
    msg += "<H2>WiFi</H2>\n<p>";
    msg += "IP: " + WiFi.localIP().toString() + "<br />\n";
    msg += "Mask: " + WiFi.subnetMask().toString() + "<br />\n";
    msg += "GW: " + WiFi.gatewayIP().toString() + "<br />\n";
    msg += "MAC: " + WiFi.macAddress() + "<br />\n";
    msg += "SSID: " + String(WiFi.SSID()) + "<br />\n";
    msg += "RSSI: " + String(WiFi.RSSI()) + "<br />\n";
    msg += "</p>\n";
    msg += "<H2>Status</H2>\n";
    if ((startAt) > 0)
    {
      msg += "Timer bis zum Einschalten noch " + String((startAt - millis()) / 1000) + "s. ";
    }
    if ((stopAt) > 0)
    {
      msg += "Timer bis zum Ausschalten noch " + String((stopAt - millis()) / 1000) + "s. ";
    }
    if (relais == 0) {
      server.send(200, "text/html", msg + "Schaltsteckdose ist aktuell aus." + switchOffHtmlButton);
    } else {
      server.send(200, "text/html", msg + "Schaltsteckdose ist aktuell ein." + switchOnHtmlButton);
    }
    server.send ( 302, "text/plain", "");  
  });  
  
  //German Path, left for compatibility
  server.on("/ein", [](){
    server.send(200, "text/html", "Schaltsteckdose ist aktuell ein." + switchOffHtmlButton);
    Switch_On();
    delay(1000);
  });
  
  server.on("/aus", [](){
    server.send(200, "text/html", "Schaltsteckdose ist aktuell aus." + switchOnHtmlButton);
    Switch_Off();
    delay(1000); 
  });

   server.on("/on", [](){
    server.send(200, "text/html", "Schaltsteckdose ist aktuell ein." + switchOffHtmlButton);
    Switch_On();
    delay(1000);
  });
  
  server.on("/off", [](){
    server.send(200, "text/html", "Schaltsteckdose ist aktuell aus." + switchOnHtmlButton);
    Switch_Off();
    delay(1000);
  });

  
  server.on("/state", [](){
    if(relais == 0){
    server.send(200, "text/html", "off");
    }else{
    server.send(200, "text/html", "on");
    }
  });

   server.on("/toggle", [](){
    if(relais == 0){
      server.send(200, "text/html", "Schaltsteckdose ist aktuell ein." + switchOffHtmlButton);
      Switch_On();
      delay(1000);
    }else{
      server.send(200, "text/html", "Schaltsteckdose ist aktuell aus." + switchOnHtmlButton);
      Switch_Off();
      delay(1000);
    }
    server.send ( 302, "text/plain", "");  
  }); 

 // Timer
  server.on("/timer", []() {
    String message = "";
    for (int i = 0; i < server.args(); i++) {

      //message += "Arg nº" + (String)i + " –> ";
      //message += server.argName(i) + ": ";
      //message += server.arg(i) + " <br />\n";

      if (server.argName(i) == "off") {
        int dauer = server.arg(i).toInt();
        startAt = millis() + (dauer * 1000);
        message += "Aus die nächsten " + String(dauer) + "s";
        server.send(200, "text/html", message);
        Switch_Off();
        delay(100);
      }

      if (server.argName(i) == "on") {
        int dauer = server.arg(i).toInt();
        stopAt = millis() + (dauer * 1000);
        message += "An die nächsten " + String(dauer) + "s";
        server.send(200, "text/html", message);
        Switch_On();
      }
    }
    if (message == "") //No Parameter
    {
      message += "<form action=\"timer\" id=\"on\">Timer bis zum Ausschalten <input type=\"text\" name=\"on\" id=\"on\" maxlength=\"5\"> Sekunden<button type=\"submit\">Starten</button></form>";
      message += "<form action=\"timer\" id=\"off\">Timer bis zum Einschalten <input type=\"text\" name=\"off\" id=\"off\" maxlength=\"5\"> Sekunden <button type=\"submit\">Starten</button></form>";
    server.send(200, "text/html", message);
    server.send ( 302, "text/plain", "");
    }
  });

#if USE_WEBUPDATE == 1
  httpUpdater.setup(&server);
#endif
  
  server.begin();
  Serial.println("HTTP server started");

  checker.attach(0.1, check);

}

#if USE_MQTT == 1
void MqttCallback(char* topic, byte* payload, unsigned int length) {
  // Switch on
  if ((char)payload[0] == '1') {
    Switch_On();
  //Switch off
  } else {
    Switch_Off();
  }

}

void MqttReconnect() {
  String clientID = "SonoffSocket_"; // 13 chars
  clientID += WiFi.macAddress();//17 chars

  while (!client.connected()) {
    Serial.print("Connect to MQTT-Broker");
    if (client.connect(clientID.c_str())) {
      Serial.print("connected as clientID:");
      Serial.println(clientID);
      //publish ready
      client.publish(mqtt_out_topic, "mqtt client ready");
      //subscribe in topic
      client.subscribe(mqtt_in_topic);
    } else {
      Serial.print("failed: ");
      Serial.print(client.state());
      Serial.println(" try again...");
      delay(5000);
    }
  }
}

void MqttStatePublish() {
  if (relais == 1 and not status_mqtt)
     {
      status_mqtt = relais;
      client.publish(mqtt_out_topic, "on");
      Serial.println("MQTT publish: on");
     }
  if (relais == 0 and status_mqtt)
     {
      status_mqtt = relais;
      client.publish(mqtt_out_topic, "off");
      Serial.println("MQTT publish: off");
     }
}

#endif
void Switch_On(void) {
  relais = 1;
  digitalWrite(gpio13Led, LOW);
  digitalWrite(gpio12Relay, relais);
  startAt = 0;
}
void Switch_Off(void) {
  relais = 0;
  digitalWrite(gpio13Led, HIGH);
  digitalWrite(gpio12Relay, relais);
  stopAt = 0;
}


void check(void)
{
  // Check if timers are finished
  if (stopAt != 0)
  {
    if (millis() >= stopAt )
    {
      Switch_Off();
    }
    else if ((millis() % 3000) < 300) digitalWrite(gpio13Led, HIGH);
    else digitalWrite(gpio13Led, LOW);

  }
  if (startAt != 0)
  {
    if (millis() >= startAt )
    {
      Switch_On();
    }
    else if ((millis() % 3000) < 300) digitalWrite(gpio13Led, LOW);
    else digitalWrite(gpio13Led, HIGH);

  }
#if USE_LOCAL_BUTTON == 1
  //check gpio0 (button of Sonoff device)
  
if ((digitalRead(0) == 0) and not status)
  {
    status=(digitalRead(0) == 0);
    if (relais == 0) {
      Switch_On();
  //Switch off
    } else {
      Switch_Off();
    }
  }
  status=(digitalRead(0) == 0);
#endif
}


void loop(void){
  //Webserver 
  server.handleClient();
  
#if USE_MQTT == 1
//MQTT
   if (!client.connected()) {
    MqttReconnect();
   }
   if (client.connected()) {
    MqttStatePublish();
   }
  client.loop();
#endif  
} 
