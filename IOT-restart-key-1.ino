//IOT Sanitizer Project for IC
//Using ESP32
// v0.1 - initial release
// v0.2 - Add wifi retry mechanism by auto-restart
// v0.3 - Add the function key to generate a message
// v0.4 - correct the method to detect the key longpress
// Every unit has an unique ID# and need to check WiFi SSID and PW

#include <neotimer.h>           // Timer interrupt driver
#include <ButtonDebounce.h>
#include <WiFi.h>               // Wifi driver
#include <PubSubClient.h>       // MQTT server library
#include <ArduinoJson.h>        // JSON library

#define LED1 32
#define TRIG 36
#define FUNC_KEY 35
#define ID 10


// MQTT and WiFi set-up
WiFiClient espClient;
PubSubClient client(espClient);
Neotimer mytimer(900000); // Set timer interrupt to 15min

// Key debounce set-up
ButtonDebounce trigger(TRIG, 100);//IO debouncing
ButtonDebounce function_key(FUNC_KEY, 100); //IO debouncing

//const char *ssid = "ap_u401";      // Your SSID             
//const char *password = "machine4320";  // Your Wifi password

//const char *ssid = "W001-Guest";      // Your SSID             
//const char *password = "W001-Guest";  // Your Wifi password

const char *ssid = "U103";      // Your SSID             
const char *password = "IcU103wifi";  // Your Wifi password

//const char *ssid = "EiA-Mbot"; 
//const char *password = "42004200";  // Your Wifi password

//const char *ssid = "icw502g"; 
//const char *password = "8c122ase";  // Your Wifi password

//const char *ssid = "EIA-W311MESH";      // Your SSID             
//const char *password = "42004200";  // Your Wifi password

//const char *mqtt_server = "mqtt.eclipse.org"; // MQTT server name
const char *mqtt_server = "ia.ic.polyu.edu.hk"; // MQTT server name
char *mqttTopic = "IC/SENSOR";    

byte reconnect_count = 0;
int count = 0;
long currentTime = 0;

boolean keyactive = false;
boolean longkeypress = false;

unsigned long keytimer = 0;
unsigned long longpresstime = 2000;//2 second

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
  // Restart if WiFi cannot be connected for 30sec
  currentTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    digitalWrite(LED1,digitalRead(LED1)^1);
    if (millis()-currentTime > 30000){
      ESP.restart();
    }
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
      reconnect_count = 0;
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      reconnect_count++;
      
      //reconnect wifi by restart if retrial up to 5 times
      if (reconnect_count == 5){
        ESP.restart();//reset if not connected to server 
      }
        
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

void buttonChanged1(int state){
  if (digitalRead(FUNC_KEY) == HIGH) {
    if (keyactive == false){
      keyactive = true;
      keytimer = millis();
      Serial.println("key is active!!");
    }
  }
  else {
    keyactive = false;
    longkeypress = false;
  }
}

void setup() {
  pinMode(TRIG, INPUT);
  pinMode(FUNC_KEY, INPUT);
  pinMode(LED1, OUTPUT);

  digitalWrite(LED1, LOW);
  
  Serial.begin(115200); 
  Serial.println("System Start!");  

  trigger.setCallback(buttonChanged);
  function_key.setCallback(buttonChanged1);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  //client.setCallback(callback);

  //Initalize Json message
  Jsondata["hand_id"] = ID;
  Jsondata["COUNT"] = 0; 
  //Jsondata["ACK"] = 1;
}

void loop() {
   trigger.update();
   function_key.update();
   
   if (!client.connected()){
    reconnect();
   }
   client.loop();

   if ((millis()-keytimer > longpresstime) && (keyactive == true) && (longkeypress ==false)){
      longkeypress = true;
      //fast flash the LED for 3 times
      for (int n = 1; n<=6; n++) {
        digitalWrite(LED1,digitalRead(LED1)^1);
        delay(250);
      }

      //send the special count value as detection of the key to server
      count = 0;
      Jsondata["COUNT"] = count;
      Serial.println("Function key is pressed");
      count = 0;  //reset the counter

      // Packing the JSON message into msg
      serializeJson(Jsondata, Serial);
      serializeJson(Jsondata, msg);
      
      //Publish msg to MQTT server
      client.publish(mqttTopic, msg);
      Serial.println();
      keyactive = false; //reset key status
   }

   /*//Use Timer Interrupt to repeat the following task regularly
   if(mytimer.repeat()){  
      Serial.println("Alive");
      
      //Sending "alive" message to server regularly
      Jsondata["hand_id"] = ID;
      Jsondata["COUNT"] = -2; 
      // Packing the JSON message into msg
      serializeJson(Jsondata, Serial);
      serializeJson(Jsondata, msg);
      
      //Publish msg to MQTT server
      client.publish(mqttTopic, msg);
      Serial.println();
     } */
}
