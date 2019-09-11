const byte interruptPin = 13;
float numberOfInterrupts = 0;
const byte RelayPin = 4;
const byte echo = 14;//d5
const byte trig = 12; //d6
const byte MoisturePin = A0; 
float limit_water=1.0;
int limit_moistury=10;
int val=0;
#include <FS.h>   
#include <ESP8266WiFi.h>
//this needs to be first, or it all crashes and burns...
//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>    


#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
 
#define MQTT_SERV "io.adafruit.com"
#define MQTT_PORT 1883
#define MQTT_NAME "thienlh96"
#define MQTT_PASS "dbb940203d23440fb8a0652382b6a0d9"

//Set up MQTT and WiFi clients
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, MQTT_SERV, MQTT_PORT, MQTT_NAME, MQTT_PASS);
 
//Set up the feed you're subscribing to
Adafruit_MQTT_Subscribe onoff = Adafruit_MQTT_Subscribe(&mqtt, MQTT_NAME "/f/relay2");
Adafruit_MQTT_Subscribe limit = Adafruit_MQTT_Subscribe(&mqtt, MQTT_NAME "/f/limit2");
Adafruit_MQTT_Subscribe Min_moisture = Adafruit_MQTT_Subscribe(&mqtt, MQTT_NAME "/f/Min_moisture2");

Adafruit_MQTT_Publish moisture = Adafruit_MQTT_Publish(&mqtt, MQTT_NAME "/f/moisture2");
Adafruit_MQTT_Publish water = Adafruit_MQTT_Publish(&mqtt, MQTT_NAME "/f/water2");

#include <SimpleTimer.h>

// the timer object
SimpleTimer timer;

void Moisture(){
  getdistance();
  Serial.println("do am la:");
  val = analogRead(MoisturePin);  // read the input pin
  int percent = map(val, 0, 1023, 0, 100);
  Serial.println(100-percent); 
  moisture.publish(100-percent);
  

}

void setup() {
 
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //exit after config instead of connecting
  wifiManager.setBreakAfterConfig(true);

  //reset settings - for testing
  //wifiManager.resetSettings();


  //tries to connect to last known settings
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP" with password "password"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("AutoConnectAP", "password")) {
    Serial.println("failed to connect, we should reset as see if it connects");
    delay(3000);
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");


  Serial.println("local ip");
  Serial.println(WiFi.localIP());
  //Subscribe to the onoff feed
  mqtt.subscribe(&onoff); 
  mqtt.subscribe(&limit);
  mqtt.subscribe(&Min_moisture);
  
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, FALLING);
  
  pinMode(RelayPin, OUTPUT);
  digitalWrite(RelayPin,LOW );
  pinMode(trig,OUTPUT); 
  pinMode(echo,INPUT); 
  timer.setInterval(3000, Moisture);
  
}
 
ICACHE_RAM_ATTR  void handleInterrupt() {
  numberOfInterrupts++;
  Serial.println("luong nuoc:"); 
  Serial.println(numberOfInterrupts/450*4);
  if(numberOfInterrupts/(450*4)>=limit_water){
    digitalWrite(RelayPin,LOW );
    water.publish(numberOfInterrupts/(450*4));
    
    numberOfInterrupts=0;
  }
}

void loop() {

  //timer.run();
  MQTT_connect();
  Adafruit_MQTT_Subscribe * subscription;
  Moisture();
  while ((subscription = mqtt.readSubscription(5000)))
  {
    if (subscription == &limit){
      limit_water= String ((char*)limit.lastread).toFloat();
      Serial.println("limit_water: ");
       Serial.print(limit_water);
    }
    if (subscription == &Min_moisture){
      limit_moistury= String ((char*)Min_moisture.lastread).toInt();
      Serial.println("limit_moistury: ");
      Serial.print(limit_moistury);
    }
    if (subscription == &onoff)
    {
     //If the new value is  "ON", turn the light on.
     //Otherwise, turn it off.
      Serial.print("onoff: ");
      Serial.println((char*) onoff.lastread);
      if (!strcmp((char*) onoff.lastread, "ON"))
      {
        //Active low logic
        digitalWrite(RelayPin,HIGH );
      }
      else
      {
        
        digitalWrite(RelayPin,LOW );
        water.publish(numberOfInterrupts/(450));
        numberOfInterrupts=0;
      }
    }
     

  }
   // ping the server to keep the mqtt connection alive
  if (!mqtt.ping())
  {
    mqtt.disconnect();
  }
  
  
 
}


void MQTT_connect()
{
  int8_t ret;
  // Stop if already connected.
  if (mqtt.connected())
  {
    return;
  }
  Serial.print("Connecting to MQTT... ");
  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) // connect will return 0 for connected
  {
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0)
       {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}
int getdistance(){
  unsigned long duration; // biến đo thời gia
  int distance;           // biến lưu khoảng các
 
  digitalWrite(trig,0);   // tắt chân tri
  delayMicroseconds(2);
  digitalWrite(trig,1);   // phát xung từ chân tri
  delayMicroseconds(5);   // xung có độ dài 5 microSecond
  digitalWrite(trig,0);   // tắt chân tri

  duration = pulseIn(echo,HIGH); 
  distance = int(duration/2/29.412);
  Serial.println(distance);
  Serial.println("cm");
  return distance;
  
}
//END CODE
