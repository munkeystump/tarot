#include <rdm630.h> // library downloaded from here https://github.com/LieBtrau/Aiakos/tree/a6ed7b2c91bc4e02473b41cb376f8add3917920a/RDM630
#define nreader  3
#define stages 3
#define cardCount 9

String cardMap[cardCount][2] = {
{"world","604e6f8a0"},
{"strength","604e6f6a0"},
{"fool","604e6f450"},
{"justice","604e6ff90"},  
{"moon","604e6f4f0"}, 
{"death","604e76860"},  
{"judgement","604e70f0"},  
{"devil","604e6f5f0"},  
{"fortune","604e6f940"}  
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
  {"world","fool","strength"},
  {"moon","judgement","justice"},
  {"devil","fortune","death"}
};

int currentstage = 0;
String lastTag = "";
boolean locked = false;   //set false for LOW to lock, and true for HIGH to lock
boolean latching = true; //do locks latch open once solved ?
int lockdelay = 500;      //number of millis for lock to remain open if NOT latching

String objects[nreader] = solutions[currentstage]; //fill objects array with IDs of correct tags for first solution
String master = "6077e0c30"; //tagID of master override/reset card

void setup()
{
  Serial.begin(9600);
  Serial.println("Serial Started");
  for(int x=0; x<stages; x++){
    pinMode(locks[x], OUTPUT);
    pinMode(leds[x], OUTPUT);
    digitalWrite(locks[x], locked); //lock all locks
    Serial.print(x);
    Serial.println(" : Lock Engaged");
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
      //Serial.println(tagID);
      Serial.println(cardLookup(tagID));
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
  //Serial.println(card);
  return card;
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
  Serial.println("=== RESET ===");
  for(int x = 0; x<stages; x++){
    currentstage = x;
    unlock();                                                  //unlock the lock if it isn't already unlocked
    delay(1000);
    digitalWrite(locks[x], locked);                 // relock the locks
    digitalWrite(leds[x], LOW);
  }
  currentstage = 0;
  for(int x = 0; x<nreader; x++){                 // refill objects array with new solution
        objects[x] = solutions[currentstage][x];
      }
}

