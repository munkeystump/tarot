#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <rdm630.h> // library downloaded from here https://github.com/LieBtrau/Aiakos/tree/a6ed7b2c91bc4e02473b41cb376f8add3917920a/RDM630
#define nreader  3
#define stages 3
#define cardCount 3

// MQTT and wifi settings
const char* ssid = "SSID";                   // name of your wi-fi network
const char* password = "WiFI Password";      //add your wi-fi password here
const char* mqtt_server = "xxx.xxx.xxx.xxx"; // ip address of your MQTT broker , should be able to get away with host name also
const char* propName = "Tarot";              // name of your prop

WiFiClient espClient;
PubSubClient client(espClient);
char msg[50];

String cardMap[cardCount][2] = {
{"death", "604e6ee00"},
{"tower", "604e6f320"},
{"priest", "604e98c70"}  
};

rdm630 rfid1(9, 0);  //TX-pin of RDM630 connected to Arduino pin 9
rdm630 rfid2(10, 0);
rdm630 rfid3(11, 0);

int leds[nreader] = {2, 3, 4};                        // pins for leds to illuminate cabinet
rdm630 allreaders[nreader] = {rfid1, rfid2, rfid3};   //array containing each rfid to be polled
String tag[nreader];                                  // array to store tags of currently placed tags
int locks[stages] = {6,7,8};                          //pins for locks (not yet implemented fully)

//2D array to hold each of the solutions to each stage of the puzzle
String solutions[stages][nreader] = {
  {"death","tower","priest"},
  {"tower","priest","death"},
  {"priest","death","tower"}
};

int currentstage = 0;
String lastTag = "";
boolean locked = false;   //set false for LOW to lock, and true for HIGH to lock
boolean latching = true; //do locks latch open once solved ?
int lockdelay = 500;      //number of millis for lock to remain open if NOT latching

String objects[nreader] = solutions[currentstage]; //fill objects array with IDs of correct tags for first solution
String master = ""; //tagID of master override/reset card

void setup()
{
  Serial.begin(9600);
  Serial.println("Serial Started");
  for(int x=0; x<stages; x++){
    pinMode(locks[x], OUTPUT);
    pinMode(leds[x], OUTPUT);
    digitalWrite(locks[x], locked); //lock all locks
    digitalWrite(leds[x], LOW);     //turn off all LEDS
  }
}

void loop()
{
    for (int reader = 0 ; reader < nreader; reader ++) {    //cycle through each reader beginning software serial, reading tag then ending software serial
      allreaders[reader].begin();
      delay(90);                                            //delay to allow reader to begin successfully
      lastTag = readTag(reader);
      tag[reader] = cardLookup(lastTag);
      allreaders[reader].end();
    }
  
    if (itissolved(tag, objects)) {                           //check to see if all objects are placed
      Serial.println("Puzzle Solved");
      unlock();
      if(currentstage < stages){
        currentstage ++;
        Serial.print("Current Puzzle Stage is : ");
        Serial.println(currentstage);
      }
      if(currentstage == stages){
        currentstage = 0;
      }
      for(int x = 0; x<nreader; x++){                 // refill objects array with new solution
        objects[x] = solutions[currentstage][x];
      }
    }
}

String readTag(int r) {
  byte data[6];
  byte length;
  String tagID;
  for (int i = 0 ; i < 1000 ; i++) {
    if (allreaders[r].available()) {
      delay(30);                                  // delay before attempting to read buffer
      allreaders[r].getData(data, length);
      Serial.print("Reader :");
      Serial.print(r + 1);
      Serial.print(" || ");
      for (int i = 0; i < length; i++) {
        tagID += String(data[i], HEX);
      }
      String thecard;
      thecard=cardLookup(tagID);
      int count = thecard.length();
      char tcard[count];
      thecard.toCharArray(tcard, count);
      Serial.println(thecard);
      char thereader = r;
      char outchar[13] = strcat("Tarot/Reader",thereader);
      client.publish("Tarot/Reader1", tcard);
     if (tagID == master){
        Serial.println("=============MASTER RESET============");
        rset();
     }
      return tagID;
    }
  }
}

String cardLookup(String card){
  //Serial.print("Looking up :");
  //Serial.println(card);
  for(int i=0;i<cardCount;i++){
    if(cardMap[i][1]==card){
      return cardMap[i][0];
    }
  }
  return "";
}

boolean itissolved(String t[nreader], String o[nreader]) {
  boolean solved = true;
  for (int j = 0 ; j < nreader; j ++) {
    if (t[j] != o[j]) {
      solved = false;
    }
    else {
      Serial.print(t[j]);           //debugging to see which tags/objects are detected in correct positions
      Serial.print(" - ");
      Serial.print(o[j]);
      Serial.println("  -- OBJECT PLACED CORRECTLY");
    }
  }
  return solved;
}

void unlock() {
  //Serial.print(currentstage);
  Serial.println(" - Lock Open");
  digitalWrite(locks[currentstage], !locked);
  digitalWrite(leds[currentstage], HIGH);
  if(!latching){
    delay(lockdelay);
    digitalWrite(locks[currentstage], locked);
  }
}

void rset(){
  for(int x = 0; x<stages; x++){
    unlock();                                                  //unlock the lock if it isn't already unlocked
    delay(1000);
    digitalWrite(locks[x], locked);                 // relock the locks
    digitalWrite(leds[x], LOW);
  }
  currentstage = 0;
}
// MQTT and WIFI methods
void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  switch ((char)payload[0]) {
    case '0':
    currentstage = 0;
    unlock();
    client.publish("Tarot/Stage", "0");
    break;
  
    case '1':
    currentstage = 1;
    unlock();
    client.publish("Tarot/Stage", "1");
    break;
    
    case '2':
    currentstage = 2;
    unlock();
    client.publish("Tarot/Stage", "2");
    break;
    
    case '7': // reset puzzle
    rset();
    break;
  
    case '8': // set puzzle Solved status to 'No' and publish
    client.publish("Tarot/Solved", "No");
    break;
  
    case '9': // set conditon to solution and check_solved
    break;
  
    default:
    // no action, break out of switch statement
    break;
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("Tarot", "Tarot/Goodbye", 2, true ,"Tarot Offline")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(propName , "Tarot Active", 2);
      // ... and resubscribe
      client.subscribe("Tarot/Command");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
