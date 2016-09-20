// requiert arduino 1.7.10 depuis arduino.org 

// permet d'ajuster les timming en Milliseconde du filtre a l'entre et a la sortie
#define TIMEOUTENTRE 250
#define TIMEOUTSORTIE 1000

// nombre de senseur
#define SENSORS 2

// endroit sur la board des senseurs
#define PINPOT A0
#define PINSONAR A1

// endroit de la led de status
#define LED 3

#include <SPI.h>
#include <Ethernet2.h>
#include <EthernetUdp2.h>
#include <SPI.h>    
#include <OSCMessage.h>

EthernetUDP Udp;
//Arduino IP
IPAddress ip(192, 168, 1, 50);
//destination IP
IPAddress outIp(192, 168, 1, 255);
// port d'envoi
const unsigned int outPort = 10000;

byte mac[] = {  
  0x90, 0xA2, 0xDA, 0x10, 0x00, 0xDE }; // mac address de l'arduino.  a changer si plus de 1 arduino sur le meme reseau


// activer le debugage seriel
bool serialDebug = false;

// configuration des valeurs de smooth min max et map des donné de senseurs
float filterVal  [SENSORS]= {0.9   ,    0.9};  // smoothing
int sensorMin    [SENSORS]= {0     ,      0}; 
int sensorMax    [SENSORS]= {1024  ,    600};
int cookedMin    [SENSORS]= {0     ,      0};
int cookedMax    [SENSORS]= {3000  ,    6000};
int mapping      [SENSORS]= {PINPOT, PINSONAR};  //endroit des senseurs

// memoire pour les datas
int raw        [SENSORS];
int cooked     [SENSORS];
int oldMaxDistance  = 0;

// memoire pour les states binaires
bool standby = true;
bool presence = false; 
bool versEntre = false;
bool entre = false;

// memoire pour le calcul de timming
double timeoutEntre = 0;
double timeoutSortie = 0;

// memoire pour l intensite de la led
int brightness = 0;

// memoire 255 pour le id de message envoye
byte count[3];


//--------------------setup----------------------------------------//
void setup()

{
  
  pinMode(LED, OUTPUT);
  Serial.begin(9600);
  Serial.println("Debug ");
  
    Ethernet.begin(mac,ip);
    Udp.begin(outPort);
    
}

//--------------------loop-----------------------------------------//
void loop()
{ 
getSensors(); 
detectePresence();
analogWrite(LED, 255-brightness);
checkMaxDistance();

}

//--------------------checkMaxDistance-----------------------------//

void checkMaxDistance(){

if (cooked[0]!=oldMaxDistance){
  sendOSC ("/maxDistance", cooked[0], cooked[1]);
}

oldMaxDistance = cooked[0];
}


//--------------------sendOSC-------------------------------------//
void sendOSC(char myMessage[], int count, int distance){

  
  OSCMessage msg(myMessage);
    msg.add((int32_t)count);
    msg.add((int32_t)distance);
   
  
    Udp.beginPacket(outIp, outPort);
    msg.send(Udp); // send the bytes to the SLIP stream
    Udp.endPacket(); // mark the end of the OSC Packet
    msg.empty(); // free space occupied by message

  

}

//--------------------detectePresence------------------------------//  

void detectePresence(){
  
  int oldPresence = presence;
  
  // il y a presence si la valeur du senseur est plus petite que le maximum
  presence = cooked[1]<cooked[0];  
  
  
  if (standby) {
      brightness = 0;  
    
    if (presence>oldPresence){  //changement entre absence et presence
      startTimeOutEntre();
      versEntre = 1;
      standby = 0;
      }

  }
  
  
  if (versEntre){
    brightness = 70;
    
      if (timeoutEntre < millis()){
      entre = 1;
      versEntre = 0;   
      }
      
      if (!presence){
      versEntre = 0;
      standby = 1;
      }
  }
  
  if (entre){
     
    
    if (presence){
    count[1] = (count[1] +1)%255;
    brightness = 200;
    sendOSC("/presence", count[1], cooked[1]);
    startTimeOutSortie();
    }
    
    if (!presence){
    brightness = 100;
    }
    
    
    if (timeoutSortie<millis()){
    standby = true;
    entre = false;
    count[2] = (count[2] +1)%255;
    sendOSC("/sortie", count[2], cooked[1]);
    }
  
  }
  
 
}

//--------------------startTimeOutEntre-----------------------------//

void startTimeOutEntre(){
  
count[0] = (count[0] +1)%255;
sendOSC("/entre", count[0], cooked[1]);
startTimeOutSortie();
  
timeoutEntre = millis() + TIMEOUTENTRE;
}

void startTimeOutSortie(){
timeoutSortie = millis() + TIMEOUTSORTIE;
}



//--------------------getSensor-----------------------------//

void getSensors(){

  for (int i = 0; i < 100; i++){
    for (int j = 0; j< SENSORS; j++){
      raw[j] =  smooth(analogRead(mapping[j]), filterVal[j], raw[j]);   
    }     
  }
  
  if (serialDebug){Serial.print("raw and cooked ");} 
  
for (int i = 0; i< SENSORS; i++){
  cooked[i] = constrain( map (raw[i], sensorMin[i], sensorMax[i], cookedMin[i], cookedMax[i]), cookedMin[i], cookedMax[i]);
  
  if (serialDebug){
  Serial.print (raw[i]);  
  Serial.print(" ");
  Serial.print (cooked[i]); 
  Serial.print(" ");
  }
}

 if (serialDebug){
      Serial.print ("vE ");  
  Serial.print(versEntre);
   Serial.print (" E ");  
  Serial.print(entre);

 Serial.println();
 }
  
}


//--------------------smooth---------------------------------------//


int smooth(int data, float filterVal, float smoothedVal){


  if (filterVal > 1){     
    filterVal = .99;
  }
  else if (filterVal <= 0){
    filterVal = 0;
  }

  smoothedVal = (data * (1 - filterVal)) + (smoothedVal  *  filterVal);

  return (int)smoothedVal;
}



