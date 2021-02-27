/*
Simple RF receiver and poster to thingspeak.com
 
 Receives date from RF transpotter [RFTransmitter.ino] and posts it to thingspeak.com using ethernet.
 
 To see debug messages set variable serialInit to true. Will initiate Serial and prinr messages.
 For standalone mode set serialInit=false;
 
 Arduino pinout
 -------------------------------  
 +5v       ->  RF receiver Vcc
 PIN 12    ->  RF receiver Data pin
 GND       ->  RF receiver GND
 
 PIN 13    ->  Receive LED indicator 

 PIN 2     ->  RELAY for reating
 ------------------------------- 
 
 Created By: Aleksei Ivanov
 Creation Date: 2015/05/23
 Last Updated Date:2021/02/27 
 */
#include <avr/wdt.h>
#include <SPI.h>
#include <Ethernet.h>
#include <HttpClient.h>
#include <VirtualWire.h>  //Library Required


boolean serialInit = true;

long myTime = millis();
long RELAY_SET_TIME; // relay status change timestamp millis()
long AWAKE_H = 1L*60L*60L;
long AWAKE_MIN = 0L*60L;
long AWAKE_SEC= 0L;
long MODULE_REBOOT_LIMIT = 1000L*(AWAKE_H + AWAKE_MIN + AWAKE_SEC);//reboot every minute
long RELAY_DELAY_LIMIT = 1000L*60L*5L; //relay on for 5 min if no further signals arrive
int HEATING_RELAY_PIN = 2;//Relay pin to set high in order to close relay and set low to open/disconnect

String sFields;

char t_buffer[10];
String temp;

int RX_PIN = 12;// Tell Arduino on which pin you would like to receive data NOTE should be a PWM Pin
int RX_ID = 3;// Recever ID address 
int TX_ID;

//Data Structure 
typedef struct roverRemoteData 
{
  int    TX_ID;      // Initialize a storage place for the incoming TX ID  
  float    Sensor1Data;// Initialize a storage place for the first integar that you wish to Receive 
  float    Sensor2Data;// Initialize a storage place for the Second integar that you wish to Receive
  float    Sensor3Data;// Initialize a storage place for the  Third integar that you wish to Receive
  float    Sensor4Data;// Initialize a storage place for the Forth integar that you wish to Receive
  float    Sensor5Data;// Initialize a storage place for the Forth integar that you wish to Receive
  String   API_KEY; //Posting to server key

};
  struct roverRemoteData receivedData;
  uint8_t rcvdSize = sizeof(receivedData);//Incoming data size 

// MAC address for your Ethernet shield
byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Your ThingsSpeak API key to let you upload data
String sApiKeys[] = {"APIKEY0","APIKEY1","APIKEY2","APIKEY3"};
long lThingTimers[] = {0,0,0,0};
long lDelayLimit=25000L; //send sensor data to web not more often than every 25sec 

EthernetClient client;

/*---------------------------------------------------------------------------------
 -- setup
 ---------------------------------------------------------------------------------*/
void setup() {
  if (serialInit)Serial.begin(9600);// Begin Serial port at a Buad speed of 9600bps 
  if (serialInit)Serial.println("Setup start");

  pinMode(13, OUTPUT);     
  pinMode(HEATING_RELAY_PIN,OUTPUT);
  if (serialInit)Serial.println("Set Relay OFF...");
  setRelayOFF();//initially RELAY OFF - open     
  
  if (serialInit)Serial.println("Start radio communication...");
  //vw_set_ptt_inverted(true); // Required for DR3100
  vw_set_rx_pin(RX_PIN);// Set RX Pin 
  vw_setup(4000);// Setup and Begin communication over the radios at 2000bps( MIN Speed is 1000bps MAX 4000bps)
  vw_rx_start(); 
  
  if (serialInit)Serial.println("Connect Ethernet...");
  //Ethernet.begin(mac,ip);

  if(Ethernet.begin(mac)==1) {
    if (serialInit)Serial.println("Ethernet connect Successful");
    if (serialInit)Serial.print("IP:");
    if (serialInit)Serial.println(Ethernet.localIP());
  }
  else{
    if (serialInit)Serial.println("Error getting IP address via DHCP, try again by resetting...");
  }

}  


/*---------------------------------------------------------------------------------
 -- loop
 ---------------------------------------------------------------------------------*/
void loop(){
   
   myTime=millis();
   if (serialInit)Serial.print("millis / reboot limit / countdown : ");
   if (serialInit)Serial.print(myTime);
   if (serialInit)Serial.print(" / ");
   if (serialInit)Serial.print(MODULE_REBOOT_LIMIT); 
   if (serialInit)Serial.print(" / ");
   if (serialInit)Serial.println(myTime-MODULE_REBOOT_LIMIT); 
      
  if (myTime-MODULE_REBOOT_LIMIT>0)  reboot(); //reboot periodically whole module to avoid millis overflow
  
  //delay(10000);
  sFields="";
  //digitalWrite(13,LOW);  
  receivedData={};
  rcvdSize = sizeof(receivedData);
  
  //relay reset
  setRelayReset();

  
  vw_wait_rx();// Start to Receive data now 



  if (vw_get_message((uint8_t *)&receivedData, &rcvdSize)) // Check if data is available 
  {
    //digitalWrite(13,HIGH); 
    if (receivedData.TX_ID == 3 or receivedData.TX_ID == 2 or receivedData.TX_ID == 1) //radio signal recieved from basement
    { 
      sFields="&1="+getFString(receivedData.Sensor1Data);
      sFields=sFields+"&2="+getFString(receivedData.Sensor2Data);
      sFields=sFields+"&3="+getFString(receivedData.Sensor3Data);
      sFields=sFields+"&4="+getFString(receivedData.Sensor4Data);

      // RELAY control based on battery powered temperature sensor
      if (receivedData.TX_ID == 2 ){
        //
        if((receivedData.Sensor1Data) == 1.0) {
          //turn relay on for 30sec
         setRelayON();
        }else{
          //turn relay off for other cases 
          setRelayOFF();
        }
      }

      if (receivedData.API_KEY != ""){
        sendHttpGet(receivedData.API_KEY, sFields,0); //send to ThingsSpeak
      }
      else{
        if (receivedData.TX_ID==1)sendHttpGet("", sFields,1); //send to ThingsSpeak
        if (receivedData.TX_ID==2)sendHttpGet("", sFields,2); //send to ThingsSpeak
        if (receivedData.TX_ID==3)sendHttpGet("", sFields,3); //send to ThingsSpeak

      }

      // If data was Recieved print it to the serial monitor.
      if (serialInit)Serial.println("------------------------New MSG-----------------------");
      if (serialInit)Serial.print("TX ID:");
      if (serialInit)Serial.println(receivedData.TX_ID);
      if (serialInit)Serial.print("Sensor1Data:");
      if (serialInit)Serial.println(receivedData.Sensor1Data);
      if (serialInit)Serial.print("Sensor2Data:");
      if (serialInit)Serial.println(receivedData.Sensor2Data);
      if (serialInit)Serial.print("Sensor3Data:");
      if (serialInit)Serial.println(receivedData.Sensor3Data);
      if (serialInit)Serial.print("Sensor4Data:");
      if (serialInit)Serial.println(receivedData.Sensor4Data);
      if (serialInit)Serial.print("Sensor5Data:");
      if (serialInit)Serial.println(receivedData.Sensor5Data);
      if (serialInit)Serial.print("API_KEY:");
      if (serialInit)Serial.println(receivedData.API_KEY);

      if (serialInit)Serial.println("-----------------------End of MSG--------------------");

    } 
    else
    { 
      if (serialInit)Serial.println(" ID Does not match waiting for next transmission ");
    }
    //digitalWrite(13,LOW); 
  }
}

void reboot(){
  if (serialInit)Serial.println(""); 
  if (serialInit)Serial.println("... REBOOT ..."); 
  if (serialInit)Serial.println(""); 
  if (serialInit)Serial.flush(); 
  wdt_disable();
  wdt_enable(WDTO_15MS);
  while (1) {}
}

/*---------------------------------------------------------------------------------
 -- setRelayON
 -- - set RELAY ON - closed status  
 ---------------------------------------------------------------------------------*/
void setRelayON(){  
          digitalWrite(HEATING_RELAY_PIN,LOW);
          RELAY_SET_TIME = millis();
          if (serialInit)Serial.println("   --> RELAY SET ON");
}

/*---------------------------------------------------------------------------------
 -- setRelayOFF
 -- - set RELAY OFF - open status  
 ---------------------------------------------------------------------------------*/
void setRelayOFF(){  
          digitalWrite(HEATING_RELAY_PIN,HIGH);
          RELAY_SET_TIME = millis();
          if (serialInit)Serial.println("   --> RELAY SET OFF");
}
/*---------------------------------------------------------------------------------
 -- setRelayReset
 -- - after given delay_limit reset RELAY OFF - open status  
 ---------------------------------------------------------------------------------*/
void setRelayReset(){  
  //relay reset  
  long myTime=millis();
  if(myTime - RELAY_SET_TIME > RELAY_DELAY_LIMIT){    
         setRelayOFF();
  }
}
/*---------------------------------------------------------------------------------
 -- getFString
 -- - Converts float to string
 ---------------------------------------------------------------------------------*/
String getFString(float pFloat){
  temp=dtostrf(pFloat,0,5,t_buffer);
  return temp;
}

int iHTTPfaiCount=0;
char c;
/*---------------------------------------------------------------------------------
 -- sendHttpGet
 -- - sends data to thingspeak
 ---------------------------------------------------------------------------------*/
void sendHttpGet(String pApiKey, String pFields,int n){
  myTime = millis();
  if (serialInit)Serial.println(pApiKey+": "+pFields);

    if(myTime - lThingTimers[n]  > lDelayLimit || myTime - lThingTimers[n]  < 0L){
    lThingTimers[n]=millis();
  }else{  
    return;
  }
  if (client.connect("api.thingspeak.com", 80))
  {
    if (serialInit)Serial.println("connected to thingspeak..");
    if (serialInit)Serial.println("GET /update?api_key="+sApiKeys[n]+pFields+" HTTP/1.1");
    //client.println("GET /update?key="+pApiKey+pFields+" HTTP/1.1");//sApiKeys(2)
    //client.println("GET /update?key="+sApiKeys[n]+pFields+" HTTP/1.1");//
    client.println("GET /update?api_key="+sApiKeys[n]+pFields+" HTTP/1.1");//
    client.println("Host: api.thingspeak.com");
    //client.println("Host: 184.106.153.149");
    client.println("Connection: close");
    client.println();
    /* client.print("POST /update HTTP/1.1\n");
     //client.print("Host: api.thingspeak.com\n");
     client.println("Host: 184.106.153.149");
     client.print("Connection: close\n");
     client.print("X-THINGSPEAKAPIKEY: " + pApiKey + "\n");
     client.print("Content-Type: application/x-www-form-urlencoded\n");
     client.print("Content-Length: ");
     client.print(pFields.length());
     client.print("\n\n");
     client.print(pFields);
     */
    delay(1000);
    if (client.connected())
    {
      if (serialInit)Serial.println("ok");

      iHTTPfaiCount = 0;
      while (client.available()) {
        c = client.read();
        if (serialInit)Serial.print(c);
      }


    }
    else
    {
      if (serialInit)Serial.println("FAIL");
      iHTTPfaiCount++;
    }

  }
  else{
    if (serialInit)Serial.println("thingspeak connect FAILED");
    iHTTPfaiCount++;
    if(iHTTPfaiCount>2){
      iHTTPfaiCount=0;
      client.stop();
      if(Ethernet.begin(mac)==1) {
        if (serialInit)Serial.println("Ethernet connect Successful");
      }
      else{
        if (serialInit)Serial.println("Error getting IP address via DHCP, try again by re...");
      }
    }
  }
}
