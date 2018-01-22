#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <Ticker.h>

//To use MQTT, install Library "PubSubClient" and switch next line to 1
#define USE_MQTT 0
//If you don`t want to use local button switch to 0
#define USE_LOCAL_BUTTON 1

#if USE_MQTT == 1
	#include <PubSubClient.h>
	//Your MQTT Broker
	const char* mqtt_server = "your mqtt broker";
	const char* mqtt_in_topic = "socket/switch/set";
  const char* mqtt_out_topic = "socket/switch/status";
	
#endif


//Yout Wifi SSID
const char* ssid = "your_ssid";
//Your Wifi Key
const char* password = "your_key";


ESP8266WebServer server(80);

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
    if(relais == 0){
    server.send(200, "text/html", "Schaltsteckdose ist aktuell aus.<p><a href=\"ein\">Einschalten</a></p>");
    }else{
    server.send(200, "text/html", "Schaltsteckdose ist aktuell ein.<p><a href=\"aus\">Ausschalten</a></p>");
    }
    server.send ( 302, "text/plain", "");  
  });  
  server.on("/ein", [](){
    server.send(200, "text/html", "Schaltsteckdose ist aktuell ein.<p><a href=\"aus\">Ausschalten</a></p>");
    relais = 1;
    digitalWrite(gpio13Led, LOW);
    digitalWrite(gpio12Relay, relais);
    delay(1000);
  });
  
  server.on("/aus", [](){
    server.send(200, "text/html", "Schaltsteckdose ist aktuell aus.<p><a href=\"ein\">Einschalten</a></p>");
    relais = 0;
    digitalWrite(gpio13Led, HIGH);
    digitalWrite(gpio12Relay, relais);
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
      server.send(200, "text/html", "Schaltsteckdose ist aktuell ein.<p><a href=\"aus\">Ausschalten</a></p>");
      relais = 1;
      digitalWrite(gpio13Led, LOW);
      digitalWrite(gpio12Relay, relais);
      delay(1000);
    }else{
      server.send(200, "text/html", "Schaltsteckdose ist aktuell aus.<p><a href=\"ein\">Einschalten</a></p>");
      relais = 0;
      digitalWrite(gpio13Led, HIGH);
      digitalWrite(gpio12Relay, relais);
      delay(1000);
    }
    server.send ( 302, "text/plain", "");  
  }); 

  
  server.begin();
  Serial.println("HTTP server started");

 #if USE_LOCAL_BUTTON == 1
  checker.attach(0.1, check);
 #endif

}

#if USE_MQTT == 1
void MqttCallback(char* topic, byte* payload, unsigned int length) {
  // Switch on
  if ((char)payload[0] == '1') {
    relais = 1;
    digitalWrite(gpio13Led, LOW);
    digitalWrite(gpio12Relay, relais);
	//Switch off
  } else {
    relais = 0;
    digitalWrite(gpio13Led, HIGH);
    digitalWrite(gpio12Relay, relais);
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

//check gpio0 (button of Sonoff device)
void check(void)
{
if ((digitalRead(0) == 0) and not status)
  {
    status=(digitalRead(0) == 0);
    if (relais == 0) {
    relais = 1;
    digitalWrite(gpio13Led, LOW);
    digitalWrite(gpio12Relay, relais);
  //Switch off
    } else {
    relais = 0;
    digitalWrite(gpio13Led, HIGH);
    digitalWrite(gpio12Relay, relais);
    }
  }
  status=(digitalRead(0) == 0);
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
