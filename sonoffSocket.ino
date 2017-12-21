#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>

//To use MQTT, install Library "PubSubClient" and switch next line to 1
#define USE_MQTT 0

#if USE_MQTT == 1
	#include <PubSubClient.h>
	//Your MQTT Broker
	const char* mqtt_server = "your_mqtt";
	const char* mqtt_topic = "socket/switch";
	
#endif


//Yout Wifi SSID
const char* ssid = "your_ssid";
//Your Wifi Key
const char* password = "your_key";


ESP8266WebServer server(80);

#if USE_MQTT == 1
	WiFiClient espClient;
	PubSubClient client(espClient);
#endif

int gpio13Led = 13;
int gpio12Relay = 12;

bool lamp = 0;
bool relais = 0;

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

#if USE_MQTT == 1  
  client.setServer(mqtt_server, 1883);
  client.setCallback(MqttCallback);
#endif

  server.on("/", [](){
    if(relais == 0){
      server.sendHeader("Location", String("/ein"), true);
    }else{
      server.sendHeader("Location", String("/aus"), true);
    }
    server.send ( 302, "text/plain", "");  
  });
  
  server.on("/ein", [](){
    server.send(200, "text/html", "Schaltsteckdose ausschalten<p><a href=\"aus\">AUS</a></p>");
    relais = 1;
    digitalWrite(gpio13Led, LOW);
    digitalWrite(gpio12Relay, relais);
    delay(1000);
  });
  
  server.on("/aus", [](){
    server.send(200, "text/html", "Schaltsteckdose einschalten<p><a href=\"ein\">EIN</a></p>");
    relais = 0;
    digitalWrite(gpio13Led, HIGH);
    digitalWrite(gpio12Relay, relais);
    delay(1000); 
  });
  
  server.begin();
  Serial.println("HTTP server started");
  
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
  while (!client.connected()) {
    Serial.print("Connect to MQTT-Broker");
    if (client.connect("SonoffSocket")) {
      Serial.println("connected");
      client.subscribe(mqtt_topic);
    } else {
      Serial.print("failed: ");
      Serial.print(client.state());
      Serial.println(" try again...");
      delay(5000);
    }
  }
}
#endif


void loop(void){
	//Webserver	
  server.handleClient();
	
#if USE_MQTT == 1
//MQTT
   if (!client.connected()) {
    MqttReconnect();
  }
  client.loop();
#endif  
} 