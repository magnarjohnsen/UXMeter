// Date and time functions using a DS3231 RTC connected via I2C and Wire lib
#include "RTClib.h"
#include "Mouse.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

RTC_DS3231 rtc;

int sensorLeftPin = A0;
float sensorLeftValue = 0;
int sensorRightPin = A1;
float sensorRightValue = 0;
const byte rtcTimerIntPin = 5;
const byte enablePin = 9;
int clockpulse = 0;
float latency= 0;
float lastlatency= 0;
float idletime= 0;
int lastread = 0;
int lastsensorValue = 0;
int prevsensorValue = 0;
int startsensorValue = 0;
int buttonPress = 0;
int enable = 0;
int fps = 0;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
char displaybuffer[4] = {' ', ' ', ' ', ' '};

Adafruit_AlphaNum4 alpha4 = Adafruit_AlphaNum4();

void setup () {

//#ifndef ESP8266
//  while (!Serial); // for Leonardo/Micro/Zero
//#endif

  Serial.begin(115200);
  Serial.println("UXMeter 0.0.9 Started");
  Serial.print("Firmware: ");
  Serial.println(__FILE__);
  Serial.print("Build time: ");
  Serial.print(__DATE__);
  Serial.print(" ");
  Serial.println(__TIME__); 
  delay(3000); // wait for console opening

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    
  }


  Serial.print("Current time: ");
  DateTime now = rtc.now();
  
  Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" (");
    Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
    Serial.print(") ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();

  // enable the 1 kHz output

  rtc.writeSqwPinMode (DS3231_SquareWave1kHz);
  
  
  // set up to handle interrupt from 1 Hz pin
  pinMode (rtcTimerIntPin, INPUT_PULLUP);
  // set up to handle enable button
  pinMode (enablePin, INPUT_PULLUP);
  delay(2000);
  Mouse.begin();
  alpha4.begin(0x70);  // pass in the address
  alpha4.writeDigitAscii(0, '0');
  alpha4.writeDigitAscii(1, '0');
  alpha4.writeDigitAscii(2, '0');
  alpha4.writeDigitAscii(3, '0');
  alpha4.writeDisplay();
}

void loop () {

  // Read right and left photodiode analogue value
  //A=Left B=Right
  sensorLeftValue = analogRead(sensorLeftPin);
  //delay(2);
  sensorRightValue = analogRead(sensorRightPin);
  //delay(2);
  // Read clock pulse from digital pin rtcTimerIntPin (4)
  clockpulse = digitalRead(rtcTimerIntPin);
  // Read enable button value
  enable = digitalRead(enablePin);
   //debug info
 //Serial.print("sensorLeftValue=");
 //Serial.print(analogRead(sensorLeftPin));
 //Serial.print(", sensorRightValue=");
 //Serial.print(analogRead(sensorRightPin));
 //Serial.print(", buttonPress=");
 //Serial.print(buttonPress);
 //Serial.print(", latency=");
 //Serial.print(latency);
 //Serial.print(", idletime=");
 //Serial.print(idletime);
 //Serial.print(", lastread=");
 //Serial.print(lastread);
 //Serial.print(", lastsensorValue=");
 //Serial.print(lastsensorValue); 
 //Serial.print(", enable=");
 //Serial.println(enable); 
 //delay(1000);

  //Check if new pulse from clock
  if (lastread!=clockpulse) {
   lastread=clockpulse; 
   
   // if mouseclick has been sent, increase latency counter, otherwise, increade idle counter
   // Clock pulse is 1024 hz and we read both rising and falling pulses so we have to multiply with 0.48828125 to get to 1000 hz  
   if (buttonPress == 1) {
     latency+=0.48828125;
     //latency+=0.5;
   }
   if (enable == 1) {
     latency+=0.48828125;
     //latency+=0.5;
   }
   if (buttonPress == 0) {
      idletime+=0.48828125;
      //idletime+=0.5;
    }
   } 
   
  // Check if left or right is brightest
  // 1=Right is brightest 0=left is brightest
  if (sensorLeftValue < sensorRightValue) {
    lastsensorValue = 1;
  }
  if (sensorLeftValue > sensorRightValue) {
    lastsensorValue = 0;
  } 
        
  //check if left and right brightness has changed from mousebutton was pressed
  if (startsensorValue != lastsensorValue) {
    if (buttonPress == 1) {
      startsensorValue = lastsensorValue;
      //send latency result over serial (unless latency is more than 3 seconds, concidered a timeout.
      if (latency <= 3000) {
        if (latency-lastlatency > 5) {
          //Serial.print("Anomaly detected");
          //Serial.println(latency-lastlatency);
        }
        if (lastlatency-latency > 5) {
          //Serial.print("Anomaly detected");
          //S erial.println(latency-lastlatency);
        }
        //only output if value is rising
        //if (lastsensorValue == 1) {
          Serial.println(latency);
          dtostrf(latency,4,0,displaybuffer); //convert double to char array
          alpha4.writeDigitAscii(0, displaybuffer[0]); //display char 1
          alpha4.writeDigitAscii(1, displaybuffer[1]); //display char 2
          alpha4.writeDigitAscii(2, displaybuffer[2]); //display char 3
          alpha4.writeDigitAscii(3, displaybuffer[3]); //display char 4
          alpha4.writeDisplay();
        //}
      }
      lastlatency=latency;
      latency=0;
      //turn off buttonpress mode, return to idle mode
      buttonPress = 0;
    }
 }

 // FPS mode
 //if (enable == 1) {
 // if (prevsensorValue != lastsensorValue) {            
 //     prevsensorValue=lastsensorValue;
 //     fps+=1;      
 // }
 // if (latency>=1000){
 //   Serial.println(fps);
 //   fps=0;
 //   latency=0;
 //   }
 //}

 // Send mouse press when idle for 1 sec
 if (idletime >= 100) {
    //only if enabled
    if (enable == 0) {
      latency=0;
      Mouse.press(MOUSE_LEFT);    
      Mouse.release(MOUSE_LEFT);
      startsensorValue = lastsensorValue;
      buttonPress = 1;
      idletime=0;
    }  
 }
}

