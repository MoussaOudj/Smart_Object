#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>  

//WiFi Connection configuration
char ssid[] = "freeboxoudj";     //  le nom du reseau WIFI
char password[] = "868C0D6EA0";  // le mot de passe WIFI
//mqtt server
char mqtt_server[] = "group5.local";  //adresse IP serveur (mosquitto publique)
#define MQTT_USER "esgi" 
#define MQTT_PASS "esgi4moc"
#define WATER_SENSOR_POWER D2
#define WATER_LEVEL_PIN A0

WiFiClient espClient;
PubSubClient MQTTclient(espClient);

int SHK_PIN = D3; //Pin for the shock sensor
int shk; //Variable to store the shk reading
int wtr = 0; //Variable to store the wtr reading
int WTR_MAX_LEVEL = 30;
int WTR_MIN_LEVEL = 0;

int redpin = D5; //select the pin for the red LED
int greenpin = D6 ;// select the pin for the green LED
int bluepin = D7; // select the pin for the  blue LED
int RED_TEMP = 0;
int GREEN_TEMP = 0;
int BLUE_TEMP = 0;
String RGB_STATUS = "Off";

//Fonctions blink pour l'envoi
void blinkOnBoardLed(){
   digitalWrite(LED_BUILTIN, LOW);
   delay(200);
   digitalWrite(LED_BUILTIN, HIGH);
}

//Fonctions separator String pour reception RGB
String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

//Fonction callback pour lecture sur topic MQTT
void callback(String topic, byte* payload, unsigned int length) {

  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  String data;

  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    data += (char)payload[i];
  }

  if (topic == "4moc/groupe5/led/rgb") {
    String red = getValue(data, ',', 0);
    String green = getValue(data, ',', 1);
    String blue = getValue(data, ',', 2);
    Serial.print("Red = ");
    Serial.println(red);
    Serial.print("Green = ");
    Serial.println(green);
    Serial.print("Blue = ");
    Serial.println(blue);
    if (RGB_STATUS == "On") {
      analogWrite(redpin, red.toInt());
      analogWrite(greenpin, green.toInt());
      analogWrite(bluepin, blue.toInt());
    }else {
      analogWrite(redpin, 0);
      analogWrite(greenpin, 0);
      analogWrite(bluepin, 0);
    }
      RED_TEMP = red.toInt();
      GREEN_TEMP = green.toInt();
      GREEN_TEMP = blue.toInt();
  } else if (topic == "4moc/groupe5/led/status") {
    RGB_STATUS = data;
    if (RGB_STATUS == "On") {
      analogWrite(redpin, RED_TEMP);
      analogWrite(greenpin, GREEN_TEMP);
      analogWrite(bluepin, BLUE_TEMP);
    } else {
      analogWrite(redpin, 0);
      analogWrite(greenpin, 0);
      analogWrite(bluepin, 0);
    }
  }
  Serial.println();
  Serial.println("-----------------------");
}

//Fonctions publication dans topic MQTT
void MQTTsend() {
 delay(100);
 MQTTclient.publish("4moc/groupe5/shock/status",String(1).c_str());
 MQTTclient.publish("4moc/groupe5/water/level/value",String(wtr).c_str());

 int range = map(wtr, WTR_MIN_LEVEL, WTR_MAX_LEVEL, 0, 3);
 String level_status = "NONE";

  // do something different depending on the range value:
  switch (range) {
    case 0:    // your hand is on the sensor
      level_status = "NOT DEEP";
      break;
    case 1:    // your hand is close to the sensor
      level_status = "FEW";
      break;
    case 2:    // your hand is a few inches from the sensor
      level_status = "MEDIUM";
      break;
    case 3:    // your hand is nowhere near the sensor
      level_status = "DEEP";
      break;
  }

   MQTTclient.publish("4moc/groupe5/water/level/status",level_status.c_str());
  
 blinkOnBoardLed();
}

//Fonction connection MQTT
void MQTTconnect() {
  while (!MQTTclient.connected()) {
      Serial.print("Attente  MQTT connection...");
      String clientId = "TestClient-";
      clientId += String(random(0xffff), HEX);

    // test connexion
    if (MQTTclient.connect(clientId.c_str(),MQTT_USER, MQTT_PASS)) {
      Serial.println("connected");
      Serial.println("subscribe to topics");
      MQTTclient.subscribe("4moc/groupe5/led/#");
    } else {  // si echec affichage erreur
      Serial.print("ECHEC, rc=");
      Serial.print(MQTTclient.state());
      Serial.println(" nouvelle tentative dans 5 secondes");
      delay(5000);
    }
  }
}

//This is a function used to get the reading
void readSensor() {
  digitalWrite(WATER_SENSOR_POWER, HIGH);  // Turn the sensor ON
  delay(10);              // wait 10 milliseconds
  wtr = analogRead(WATER_LEVEL_PIN);    // Read the analog value form sensor
  digitalWrite(WATER_SENSOR_POWER, LOW);   // Turn the sensor OFF
  Serial.print("water level : ");
  Serial.println(wtr);
}

//Setup the Arduino
void setup() 
{
  Serial.begin(115200);
  //Button + LED init
  pinMode(SHK_PIN, INPUT); //Make the pin you used an input on the Arduino
  pinMode(LED_BUILTIN, OUTPUT);
  // Set D7 as an OUTPUT
  pinMode(WATER_SENSOR_POWER, OUTPUT);
  // Set to LOW so no power flows through the sensor
  digitalWrite(WATER_SENSOR_POWER, LOW);
  
  // Conexion WIFI
   WiFi.begin(ssid, password);
   Serial.println("");
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
   Serial.println("Connected");
   MQTTclient.setServer(mqtt_server, 1883);
   MQTTclient.setCallback(callback);
}

//This code will run infinitely
void loop() 
{
   // connect serveur MQTT
  if (!MQTTclient.connected()) {
    MQTTconnect();
  }
  MQTTclient.loop();
  delay(1); 
  if(digitalRead(SHK_PIN) == LOW) //Read the shock sensor
  {
    readSensor();
    Serial.println("Shock Detected"); //Print out that we have been shaken
    MQTTsend();
    delay(1000); //Wait half a second before reading again
  }
  
}
