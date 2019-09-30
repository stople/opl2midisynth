#ifndef DEBUG_cpp
#define DEBUG_cpp

#include <Arduino.h>
#include "Debug.h"
#include "Opl2Instrument.h"
#include "Parameters.h"
void incrementMidiInstrument();

void printChar(char c); //Print to uart + LCD
void printString(char *s); //Print to uart + LCD

void debugInterface() {
  /*
  if (Serial.available())
  {
    int cmd = Serial.read();
    if (cmd == 'a') //debug
    {
      Serial.write("T");
      Serial.write("E");
      Serial.write("S");
      Serial.write("T");
      return;
    }
    if (cmd == 's') //debug
    {
      Serial.println("TestNL");
      return;
    }
    if (cmd == 'd') //play floppy note 220hz
    {
      Serial.println("StartFloppy");
      //Floppy1.setTone(220);
      return;
    }
    if (cmd == 'f') //play floppy note 220hz
    {
      Serial.println("StopFloppy");
      //Floppy1.stopTone();
      return;
    }
    if (cmd == 'g') //play opl tone
    {
      Serial.println("Play 80");
      Opl2Instrument1.onNoteOn(0, 80, 80);
      return;
    }
    if (cmd == 'b') //play opl tone
    {
      Serial.println("Stop 80");
      Opl2Instrument1.onNoteOff(0, 80, 0);
      return;
    }
    if (cmd == 'h') //play opl tone
    {
      Serial.println("Play 100");
      Opl2Instrument1.onNoteOn(0, 100, 80);
      return;
    }
    if (cmd == 'n') //play opl tone
    {
      Serial.println("Stop 100");
      Opl2Instrument1.onNoteOff(0, 100, 0);
      return;
    }
    if (cmd == 'i') //instrument
    {
      Serial.print("Instrument ");
	  incrementMidiInstrument();
      return;
    }
    if (cmd == 'p') //panic
    {
      Serial.println("Panic");
            Opl2Instrument1.onSystemReset();
      return;
    }
    
  }
*/
}


void InsertHex8(uint8_t *data, uint8_t length, uint8_t *dest) // prints 8-bit data in hex with leading zeroes (https://forum.arduino.cc/index.php?topic=38107.0)
{
  //Note: Does not append string stop byte (0)
  uint8_t *p = dest;
       char tmp[16];
       for (int i=0; i<length; i++) { 
         sprintf(tmp, "%.2X",data[i]); 

         *dest++ = tmp[0];
         *dest++ = tmp[1];
         *dest++ = ' ';
       }
}

void PrintHex8(uint8_t *data, uint8_t length) // prints 8-bit data in hex with leading zeroes (https://forum.arduino.cc/index.php?topic=38107.0)
{
     char tmp[16];
       for (int i=0; i<length; i++) { 
         sprintf(tmp, "%.2X",data[i]); 
         //Serial.print(tmp); Serial.print(" ");
         printString(tmp); printChar(" ");
       }
}

void dumpOplReg()
{
  int i, j;
  byte data;
  for (i = 0; i < 256; i += 16)
  {
    data = (byte)i;
    PrintHex8(&data, 1);
    Serial.print(" ");
    for (j = 0; j < 16; ++j)
    {
      data = Opl2Instrument1._opl2.getRegister(i + j);
      PrintHex8(&data, 1);
      Serial.print(" ");
    }
    Serial.print("\n");
  }
  Serial.print("\n\n\n");
}

void dumpCurrentInstrument()
{
  byte ch = 0;
  Serial.print("SynthType ");
  Serial.println(parSynthType.val);
  Serial.print("Op0 waveform ");
  Serial.println(Opl2Instrument1._opl2.getWaveForm(ch, 0));
  Serial.print("Op1 waveform ");
  Serial.println(Opl2Instrument1._opl2.getWaveForm(ch, 1));
  
  Serial.print("\n\n\n");

}



#endif
