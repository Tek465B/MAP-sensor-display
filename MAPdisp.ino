/*
MAP sensor display.
This read data from GM 2-bar MAP sensor, display the atmospheric pressure for 3 second and then display boost pressure in PSI and vacuum in In-Hg, refresh rate is 500ms.
Connect sensor to Analog pin 0.
Uncomment the define SensCalib to calibrate the sensor and write value to the EEPROM, must power on while plugged to your vehicle and key to on. 
Must wait 13 second till display turn on, once thats done unplug and bring back inside, comment the SensCalib line while making sure to re-upload before the 10 second delay so it do not overwrite the sensor value in eeprom.

tek465b.github.io
by Tek465B
*/


//#define SensCalib
#include "SevSeg.h"
#include <EEPROM.h>

SevSeg sevseg;

int MapVtot = 0;
float MapkPA;
float CalkPA = 101.3;
float kPAdiff;
float MapPSI;
static unsigned long previousMillis = 3000;
byte interval = 31;
byte StepCnt = 0;
float SensCal = 4;

void TakeReading() {
  for (byte i = 0; i < 4; i++) {
    MapVtot += analogRead(0);
    delay(20);
  }
  MapVtot = MapVtot >> 2; //we take 4 reading 20ms apart and get the average.
  MapkPA = map(MapVtot, 0, 1023, 88, 2080);
  MapkPA /= 10; //convert it to kPA

  //Section below should be edited to read an input pin with pullup resistor and jumper to enable/disable calibration instead of re-programming the flash without the calibration.
#ifdef SensCalib //wait 10 second then write the sensor value to eeprom, 10 sec give you time when re-uploading new program so it do not overwrite with a wrong value. You can replace the 101.3 with current atmos pressure.
  delay(10000);
  SensCal = 101.3 - MapkPA;
  EEPROM.put(0, SensCal);
#endif

  EEPROM.get(0, SensCal); //Here we read the value from eeprom and add it to the sensor reading so we have calibrated absolute pressure, we also write atmospheric presure to a variable so we can substract it later to get relative pressure value.
  MapkPA += SensCal;
  CalkPA = MapkPA;
  sevseg.setNumberF(MapkPA, 1);
  MapVtot = 0;
}

void setup() {
  byte numDigits = 4; //initializing the segment display library
  byte digitPins[] = { 7, 11, 10, 8 };
  byte segmentPins[] = { 13, 9, 5, 3, 2, 12, 6, 4 };
  bool resistorsOnSegments = true;
  byte hardwareConfig = COMMON_ANODE;
  bool updateWithDelays = false;
  bool leadingZeros = false;

  sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments, updateWithDelays, leadingZeros);
  sevseg.setBrightness(90);
  TakeReading(); //taking first reading to display atmospheric pressure in kPA for 3 second.
}

void loop() {

  if ((long)(millis() - previousMillis) >= 0) { //after 3 second, we take 16 reading each 31ms apart and get average so we get update rate of about half a second.
    previousMillis += interval;
    MapVtot += analogRead(0);
    StepCnt++;

    if (StepCnt >= 16) {
      MapVtot = MapVtot >> 4;
      MapkPA = map(MapVtot, 0, 1023, 88, 2080);
      MapkPA /= 10;
      MapkPA += SensCal;
      MapVtot = 0;
      StepCnt = 0;
      kPAdiff = MapkPA - CalkPA;
      if (kPAdiff > 0) { //if positive value we convert to PSI and 2 decimal place, if negative we convert to in-hg and 1 decimal place.
        MapPSI = kPAdiff * 0.145037;
        sevseg.setNumberF(MapPSI, 2);
      } else {
        MapPSI = kPAdiff * 0.295299;
        sevseg.setNumberF(MapPSI, 1);
      }
    }
  }
  sevseg.refreshDisplay();
}
