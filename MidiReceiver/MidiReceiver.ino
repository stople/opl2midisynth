// OPL2 MIDI synthesizer
//
// Dependencies:
// ArduinoOPL2 by DhrBaksteen
// Bounce2 for button press
//
// Input: Stream of MIDI data
// Output: OPL2 data stream


#include <Bounce2.h>

#include "Parameters.h"
#include "Menu.h"

//OPL2
extern const unsigned char *midiInstruments[];

#include "Opl2Instrument.h"
byte CurrentOplProgram = 0;

//Instrument control
bool BuzzerEnabled = false;
bool Opl2Enabled = true;
int ActiveInstruments = 1;

int PotmeterLock = -1;
byte LastPotmeterReadoutTime = 0;

enum {
  DebugTogglePin = A0,
  InstrumentCyclePin = A1,
  DemoPin = A2,
  ExtraButtonPin = A3,
  PotmeterPin = A4,

  BuzzerPin = 4
};

int Demo = 0;

#define NUM_BUTTONS 4
const int BUTTON_PINS[NUM_BUTTONS] = {DebugTogglePin, InstrumentCyclePin, DemoPin, ExtraButtonPin};

Bounce * buttons = new Bounce[NUM_BUTTONS];

void menuInput(int button);
void setCurrentParameterFromPotmeter(int potmeter);
void PrintHex8(uint8_t *data, uint8_t length); // prints 8-bit data in hex with leading zeroes (https://forum.arduino.cc/index.php?topic=38107.0)
void setPartMidiPos();








void setup() {

  //Opl2
  Opl2Instrument1.onSystemReset();

  //Buttons
  for (int i = 0; i < NUM_BUTTONS; i++) {
    buttons[i].attach( BUTTON_PINS[i] , INPUT  );       //setup the bounce instance for the current button
    buttons[i].interval(25);              // interval in ms
  }

  PotmeterLock = analogRead(PotmeterPin);

//Midi input
  Serial.begin(57600);
}

//60 -> tone C4 (262)
//69 -> tone A4 (440)

//Special keys:
//21 -> Tone A0 (Select buzzer)
//22 -> Tone Bb0 (Select buzzer)
//23 -> Tone B0 (Select OPL2)

int midiToFrequency(int midiPitch)
{
  
  float logTone = 8.781; //log2(440) (A4)
  logTone += (float)(midiPitch - 69) / 12;
  
  double exptone = pow(2, logTone);
  return (int)exptone;
}


//int currentFloppyKey = 0;
int currentBuzzerKey = 0;


void toggleInstrument(bool *instrumentCtrl)
{
  if (*instrumentCtrl)
  {
    if (ActiveInstruments <= 1) return;
    *instrumentCtrl = false;
    ActiveInstruments--;
  }
  else
  {
    *instrumentCtrl = true;
    ActiveInstruments++;
  }
}

//extern int toggleCtr;

void debugInterface() {
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
          CurrentOplProgram++;
          if (CurrentOplProgram > 127) CurrentOplProgram = 0;
      Serial.println(CurrentOplProgram);
          for (byte i = 0; i < Opl2Instrument::MIDI_NUM_CHANNELS; ++i)
          {
            Opl2Instrument1.onProgramChange(i, CurrentOplProgram);
          }
      return;
    }
    if (cmd == 'p') //panic
    {
      Serial.println("Panic");
            Opl2Instrument1.onSystemReset();
      return;
    }
    
  }

}

void readMidiFromSerial() {
  if (Serial.available() >= 3)
  {
    int cmd = Serial.read();
    if (cmd < 128) return;
    int channel = cmd & 0x0F;
    cmd >>= 4;
    switch (cmd){
      case 8: //Note off
      {
        int pitch = Serial.read();
        if (pitch >= 128) return;
        int velocity = Serial.read();
        if (velocity >= 128) return;

        if (pitch == currentBuzzerKey) noTone(BuzzerPin);
        if (Opl2Enabled) Opl2Instrument1.onNoteOff(channel, pitch, velocity);

        break;
      }

      case 9: //Note on
      {
        int pitch = Serial.read();
        if (pitch >= 128) return;
        int velocity = Serial.read();
        if (velocity >= 128) return;
  
        int noteOn = 0;
        if (velocity >= 1) noteOn = 1;


        if (Opl2Enabled) Opl2Instrument1.onNoteOn(channel, pitch, velocity);
        
 
        break;
  
      }
    }
  }  
}

void tick()
{

}

int DebugTogglePinState = LOW;
int InstrumentCyclePinState = LOW;
int DemoPinState = LOW;
int ExtraButtonPinState = LOW;

int DebugTogglePinLast = LOW;
int InstrumentCyclePinLast = LOW;
int DemoPinLast = LOW;
int ExtraButtonPinLast = LOW;

unsigned long lastDebounceTime = 0;

void readButtons()
{
  for (int i = 0; i < NUM_BUTTONS; i++)  {
    // Update the Bounce instance :
    buttons[i].update();
    // If it fell, flag the need to toggle the LED
    if ( buttons[i].rose() ) {
      PotmeterLock = analogRead(PotmeterPin);
      Serial.print("button ");
      Serial.println(i);

      menuInput(i);
      
    }
  }

  //Potensiometer
  //Require certain offset before using the value after menu option is set
  int potmeter = analogRead(PotmeterPin); //0-1023

  if (PotmeterLock != -1 && abs(PotmeterLock - potmeter) < 256) return; //Require 1/4 turn before activating
  PotmeterLock = -1;

  byte diff = (byte)millis() - LastPotmeterReadoutTime;

  if (diff < 100) return; //100 ms between each update
  LastPotmeterReadoutTime = (byte)millis();
  setCurrentParameterFromPotmeter(potmeter);

}

void loop() {

  readButtons();
  if (debugMode.val) debugInterface();
  else readMidiFromSerial();  
}





void setMidiInstrument()
{
  byte instrument = midiInstrument.val;
  /*for (byte ch = 0; ch < Opl2Instrument::MIDI_NUM_CHANNELS; ++ch)
  {
    Opl2Instrument1.onProgramChange(ch, instrument);
  }*/

  Serial.print("SetInstrument ins:");
  Serial.print(instrument);
  Serial.print("\n");

    //Opl2Instrument1._opl2.setInstrument(3, midiInstruments[instrument]);

  for (byte ch = 0; ch < OPL2_NUM_CHANNELS; ++ch)
  {
    Opl2Instrument1._opl2.setInstrument(ch, midiInstruments[instrument]);
  delay(35); //Add 35ms delay between instruments

  }

    //dumpOplReg();
  loadCustomInstrumentParameters();
}

void setCustomInstrument()
{
  byte op = parOperator.val;
  Opl2Instrument1._opl2.setWaveFormSelect(parWaveformSelect.val);
  Opl2Instrument1._opl2.setDeepTremolo(parDeepTremolo.val);
  Opl2Instrument1._opl2.setDeepVibrato(parDeepVibrato.val);

  for (byte ch = 0; ch < OPL2_NUM_CHANNELS; ++ch)
  {
    delay(1); //Wait 1 ms between commands (necessary?)
    Opl2Instrument1._opl2.setTremolo(ch, op, parTremolo.val);
    Opl2Instrument1._opl2.setVibrato(ch, op, parVibrato.val);
    Opl2Instrument1._opl2.setMaintainSustain(ch, op, parSustainEnabled.val);
    Opl2Instrument1._opl2.setEnvelopeScaling(ch, op, parKSR.val);
    Opl2Instrument1._opl2.setMultiplier(ch, op, parFreqMult.val);
    Opl2Instrument1._opl2.setScalingLevel(ch, op, parKeyScaleLevel.val);
    Opl2Instrument1._opl2.setAttack(ch, op, parAttack.val);
    Opl2Instrument1._opl2.setDecay(ch, op, parDecay.val);
    Opl2Instrument1._opl2.setSustain(ch, op, parSustain.val);
    Opl2Instrument1._opl2.setRelease(ch, op, parRelease.val);
    //Opl2Instrument1._opl2.setFNumber(ch, parFNumber.val);
    Opl2Instrument1._opl2.setFeedback(ch, parModFbFactor.val);
    Opl2Instrument1._opl2.setSynthMode(ch, parSynthType.val);
    Opl2Instrument1._opl2.setWaveForm(ch, op, parWaveform.val);
  }

}

void loadCustomInstrumentParameters()
{
  byte instrument = midiInstrument.val;
  //Load from channel 0
  byte op = parOperator.val;
  byte ch = 0;

  parWaveformSelect.val = Opl2Instrument1._opl2.getWaveFormSelect();
  parDeepTremolo.val = Opl2Instrument1._opl2.getDeepTremolo();
  parDeepVibrato.val = Opl2Instrument1._opl2.getDeepVibrato();

  parTremolo.val = Opl2Instrument1._opl2.getTremolo(ch, op);
  parVibrato.val = Opl2Instrument1._opl2.getVibrato(ch, op);
  parSustainEnabled.val = Opl2Instrument1._opl2.getMaintainSustain(ch, op);
  parKSR.val = Opl2Instrument1._opl2.getEnvelopeScaling(ch, op);
  parFreqMult.val = Opl2Instrument1._opl2.getMultiplier(ch, op);
  parKeyScaleLevel.val = Opl2Instrument1._opl2.getScalingLevel(ch, op);
  parAttack.val = Opl2Instrument1._opl2.getAttack(ch, op);
  parDecay.val = Opl2Instrument1._opl2.getDecay(ch, op);
  parSustain.val = Opl2Instrument1._opl2.getSustain(ch, op);
  parRelease.val = Opl2Instrument1._opl2.getRelease(ch, op);
  parModFbFactor.val = Opl2Instrument1._opl2.getFeedback(ch);
  parSynthType.val = Opl2Instrument1._opl2.getSynthMode(ch);
  parTremolo.val = Opl2Instrument1._opl2.getTremolo(ch, op);
  parWaveform.val = Opl2Instrument1._opl2.getWaveForm(ch, op);

}

void incrementMidiInstrument()
{
  incrementParameterWithChangeTrigger((void*)&midiInstrument);
  //setMidiInstrument();
}

void PrintHex8(uint8_t *data, uint8_t length) // prints 8-bit data in hex with leading zeroes (https://forum.arduino.cc/index.php?topic=38107.0)
{
     char tmp[16];
       for (int i=0; i<length; i++) { 
         sprintf(tmp, "%.2X",data[i]); 
         Serial.print(tmp); Serial.print(" ");
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

void setPartParam(byte ins, byte pos)
{

    const unsigned char *instrument = midiInstruments[ins];
    //byte channel = 3;
  
    #if BOARD_TYPE == OPL2_BOARD_TYPE_ARDUINO
    unsigned char percussionChannel = pgm_read_byte_near(instrument);
  #else
    unsigned char percussionChannel = instrument[0];
  #endif

  Opl2Instrument1._opl2.setWaveFormSelect(true);

  byte i = pos;
      for (byte channel = 0; channel < 9; channel ++) {

        byte reg;
        if (i == 5)
          reg = 0xC0 + max(0x00, min(channel, 0x08));
        else
          reg = Opl2Instrument1._opl2.instrumentBaseRegs[i % 6] + Opl2Instrument1._opl2.getRegisterOffset(channel, i > 5);

        Opl2Instrument1._opl2.setRegister(
          reg,
          //#if BOARD_TYPE == OPL2_BOARD_TYPE_ARDUINO
            pgm_read_byte_near(instrument + i + 1)
          //#else
          //  instrument[i + 1]
          //#endif
        );
      }


}

void setPartMidiPos()
{
  setPartParam(partMidiInstrument.val, partMidiPos.val);
}
