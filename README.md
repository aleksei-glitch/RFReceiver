# RFReceiver

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

use latest version of VirtualWire. Currently obsolete but needed for old code.
http://www.airspayce.com/mikem/arduino/VirtualWire/
