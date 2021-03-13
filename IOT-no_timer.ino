//IOT Sanitizer Project for IC
//Using ESP32

//#include <neotimer.h>           // Timer interrupt driver
#include <ButtonDebounce.h>
#include <WiFi.h>               // Wifi driver
#include <PubSubClient.h>       // MQTT server library
#include <ArduinoJson.h>        // JSON library

#define LED1 32
#define TRIG 36


// MQTT and WiFi set-up
WiFiClient espClient;
PubSubClient client(espClient);
//Neotimer mytimer(30000); // Set timer interrupt to 30sec

// Key debounce set-up
ButtonDebounce trigger(TRIG, 700);

const char *ssid = "U103";      // Your SSID             
const char *password = "IcU103wifi";  // Your Wifi password
//const char *ssid = "icw502g";      // Your SSID             
//const char *password = "8c122ase";  // Your Wifi password
//const char *ssid = "EIA-W311MESH";      // Your SSID             
//const char *password = "42004200";  // Your Wifi password
//const char *mqtt_server = "mqtt.eclipse.org"; // MQTT server name
const char *mqtt_server = "ia.ic.polyu.edu.hk"; // MQTT server name
char *mqttTopic = "IC/SENSOR";    


int count = 0;

char msg[200];
String ipAddress;
String macAddr;
String recMsg="";

StaticJsonDocument<50> Jsondata; // Create a JSON document of 200 characters max

//Set up the Wifi connection
void setup_wifi() {
  WiFi.disconnect();
  delay(100);
  // We start by connecting to a WiFi network
  Serial.printf("\nConnecting to %s\n", ssid);
  WiFi.begin(ssid, password); // start the Wifi connection with defined SSID and PW

    // Indicate "......" during connecting and flashing LED1
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    digitalWrite(LED1,digitalRead(LED1)^1);
  }
  // Show "WiFi connected" once linked and light up LED1
  Serial.printf("\nWiFi connected\n");
  digitalWrite(LED1,HIGH);
  
  // Show IP address and MAC address
  ipAddress=WiFi.localIP().toString();
  Serial.printf("\nIP address: %s\n", ipAddress.c_str());
  macAddr=WiFi.macAddress();
  Serial.printf("MAC address: %s\n", macAddr.c_str());
}

/*// Routine to receive message from MQTT server
void callback(char* topic, byte* payload, unsigned int length) {
  
  recMsg ="";
  for (int i = 0; i < length; i++) {
    recMsg = recMsg + (char)payload[i];
  }
  Serial.printf("%d: Message arrived [%s] %s\n", millis(), topic, recMsg.c_str());
  Serial.println(recMsg);
  delay(500);
}*/


// Reconnect mechanism for MQTT Server
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.printf("Attempting MQTT connection...");
    // Attempt to connect
    //if (client.connect("ESP32Client")) {
    if (client.connect(macAddr.c_str())) {
      Serial.println("Connected");
      // Once connected, publish an announcement...
      snprintf(msg, 75, "IoT System (%s) is READY", ipAddress.c_str());
      client.subscribe(mqttTopic);
      delay(1000);
      client.publish(mqttTopic, msg);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void buttonChanged(int state){
  if (digitalRead(TRIG) == HIGH) {
   count = 1;
   Jsondata["COUNT"] = count;
   Serial.println("Number of counts is: 1");
   count = 0;  //reset the counter

   // Packing the JSON message into msg
   serializeJson(Jsondata, Serial);
   serializeJson(Jsondata, msg);
      
   //Publish msg to MQTT server
   client.publish(mqttTopic, msg);
   Serial.println();
  }
}

void setup() {
  pinMode(TRIG, INPUT_PULLUP);
  pinMode(LED1, OUTPUT);

  digitalWrite(LED1, LOW);
  
  Serial.begin(115200); 
  Serial.println("System Start!");  

  trigger.setCallback(buttonChanged);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  //client.setCallback(callback);

  //Initalize Json message
  Jsondata["hand_id"] = 5;
  Jsondata["COUNT"] = 0; 
}

void loop() {
   trigger.update();
   
   if (!client.connected()){
    reconnect();
   }
   client.loop();

   /*//Use Timer Interrupt to repeat the following task regularly
   //if(mytimer.repeat()){  
      Jsondata["COUNT"] = count;
      Serial.print("Number of counts is: ");
      Serial.println(count);
      count = 0;  //reset the counter

      // Packing the JSON message into msg
      serializeJson(Jsondata, Serial);
      serializeJson(Jsondata, msg);
      
      //Publish msg to MQTT server
      client.publish(mqttTopic, msg);
      Serial.println();
     }*/ 
}
