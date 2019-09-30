// OPL2 MIDI synthesizer
//
// Dependencies:
// ArduinoOPL2 by DhrBaksteen
// Bounce2 for button press
//
// Input: Stream of MIDI data
// Output: OPL2 data stream

// Connections:
//
//  0..1:     Midi
//  2..7:     LCD
//  8..11+13: OPL2
//  A0..A3:   Menu
//  A4:       Potensiometer
//
//  0 MIDI rx
//  1 MIDI tx
//  2 LCD d7
//  3 LCD d6
//  4 LCD d5
//  5 LCD d4
//  6 LCD en
//  7 LCD rs
//  8 OPL2 Reset
//  9 OPL2 A0
// 10 OPL2 Latch
// 11 OPL2 Data
// 12
// 13 OPL2 Shift
//
// A0 MENU back
// A1 MENU left
// A2 MENU right
// A3 MENU enter
// A4 MENU potensiometer
// A5


#include <LiquidCrystal.h>


//const int LED_rs = 7, LED_en = 6, LED_d4 = 5, LED_d5 = 4, LED_d6 = 3, LED_d7 = 2;
//enum { LED_rs = 7, LED_en = 6, LED_d4 = 5, LED_d5 = 4, LED_d6 = 3, LED_d7 = 2};
enum { LED_rs = 7, LED_en = 6, LED_d4 = 5, LED_d5 = 4, LED_d6 = 3, LED_d7 = 2};
LiquidCrystal lcd(LED_rs, LED_en, LED_d4, LED_d5, LED_d6, LED_d7);


#include <Bounce2.h>

#include "Parameters.h"
#include "Menu.h"
#include "Debug.h"
#include "Opl2Instrument.h"

//OPL2
extern const unsigned char *midiInstruments[];

//Potmeter
int PotmeterLock = -1;
byte LastPotmeterReadoutTime = 0;

//Buttons
unsigned long lastDebounceTime = 0;
#define NUM_BUTTONS 4
enum {
  BtnMenuBackPin = A0,
  BtnMenuLeftPin = A1,
  BtnMenuRightPin = A2,
  BtnMenuEnterPin = A3,
  PotmeterPin = A4
};
const int BUTTON_PINS[NUM_BUTTONS] = {BtnMenuBackPin, BtnMenuLeftPin, BtnMenuRightPin, BtnMenuEnterPin};
Bounce * buttons = new Bounce[NUM_BUTTONS];



void printChar(char c)
{
  Serial.print(c);
  lcd.print(c);
}

void printString(char *s)
{
  Serial.print(s);
  lcd.print(s);
}



void setup() {

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  //Opl2
  Opl2Instrument1.onSystemReset();

  //Buttons
  for (int i = 0; i < NUM_BUTTONS; i++) {
    buttons[i].attach( BUTTON_PINS[i] , INPUT  );       //setup the bounce instance for the current button
    buttons[i].interval(25);              // interval in ms
  }

  PotmeterLock = analogRead(PotmeterPin);

//Midi input
  //Serial.begin(57600);
  Serial.begin(31250);
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

int fetchSerial()
{
  int data = Serial.read();
  return data;
}


void appendToMonitor(char c)
{
  MENU_ITEM* m = getCurrentMenuItem();
  //if (m->MenuItemParameter == &monMidiInput)
  {
    //Make space for 3 chars

    for (int i = 0; i < 16 - 3; ++i)
    {
      monitorText[i] = monitorText[i + 3];
    }

    byte dataB = (byte)c;
    InsertHex8(&dataB, 1, &monitorText[16 - 3]);
  }

  Serial.print("Midi: ");
  Serial.println((unsigned char)c);
  
  //monitorText[2] = 'T';
  //return data;
}

void readMidiFromSerial() {
  /*if (Serial.available() >= 1)
  {
    appendToMonitor(fetchSerial());
  }
  return;*/
  
  //while (Serial.available() && Serial.peek()
  if (Serial.available() >= 3)
  {
    int cmd = fetchSerial();
    if (cmd < 128) return;
    int origCmd = cmd;
    int channel = cmd & 0x0F;
    cmd >>= 4;
    switch (cmd){
      case 8: //Note off
      {
        int pitch = fetchSerial();
        int velocity = fetchSerial();
        if (pitch >= 128) return;
        if (velocity >= 128) return;

        appendToMonitor(origCmd);
        appendToMonitor(pitch);
        appendToMonitor(velocity);

        Opl2Instrument1.onNoteOff(channel, pitch, velocity);

        break;
      }

      case 9: //Note on
      {
        int pitch = fetchSerial();
        int velocity = fetchSerial();
        if (pitch >= 128) return;
        if (velocity >= 128) return;
  
        int noteOn = 0;
        if (velocity >= 1) noteOn = 1;


        Opl2Instrument1.onNoteOn(channel, pitch, velocity);
        
        appendToMonitor(origCmd);
        appendToMonitor(pitch);
        appendToMonitor(velocity);
 
        break;
  
      }
      case 11: //Control change (pedal)
      {
        int controller = fetchSerial();
        int value = fetchSerial();

        appendToMonitor(origCmd);
        appendToMonitor(controller);
        appendToMonitor(value);

        Opl2Instrument1.onControlChange(channel, controller, value);
        

        if (controller == 64) //Sustain pedal
        {
          Opl2Instrument1.onSustain(value);
        }

        
      }

      default: //other
      {
        if (origCmd != 0xFE)
        {
          appendToMonitor(origCmd);
          //while (Serial.available() && (cmd = fetchSerial()) < 128) appendToMonitor(cmd); //cmd parameters, busy wait
        }
      }
    }
  }  
}


void readButtons()
{
  for (int i = 0; i < NUM_BUTTONS; i++)  {
    buttons[i].update();
    if ( buttons[i].rose() ) {
      PotmeterLock = analogRead(PotmeterPin); //Lock potmeter to current position
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


uint16_t lastLedRefresh;

void loop() {

  readButtons();
  if (debugMode.val) debugInterface();
  else readMidiFromSerial();

  uint16_t currentTime = millis();
  if (currentTime - lastLedRefresh > 1000)
  {
    lastLedRefresh = currentTime;
    drawMenuOption(); //Redraw every 1000 ms
  }

  
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

  for (byte ch = 0; ch < OPL2_NUM_CHANNELS; ++ch)
  {
    Opl2Instrument1._opl2.setInstrument(ch, midiInstruments[instrument]);
  delay(35); //Add 35ms delay between instruments

  }

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

//Hardcode some demo


#include "stunts.h"

unsigned long offset       = 0;
unsigned long songLength   = 0;
//unsigned long songDuration = 0;
byte codeShortDelay    = 0;
byte codeLongDelay     = 0;
byte registerMapLength = 0;
//byte registerMap[256];


void playSong() {
  unsigned long time = millis();

  loadDroSong();

  while (songLength > 0) {
    int wait = playDroSong();
    wait -= (millis() - time);      // Take into account time that was spent on IO.

    if (wait > 0) {
      delay(wait);
      time = millis();
    }

    songLength --;
  }

  Opl2Instrument1.silence();
}



uint16_t droOffset = 0;
uint8_t readDroByte()
{
  return pgm_read_byte_near(load_005_dro + droOffset++);
}

void loadDroSong() {
  //droFile = SD.open(fileName, FILE_READ);
  //droFile.seek(12);
  droOffset = 12;

  songLength  = readDroByte();
  songLength += readDroByte() << 8;
  songLength += readDroByte() << 16;
  songLength += readDroByte() << 24;

//  songDuration  = readDroByte();
//  songDuration += readDroByte() << 8;
//  songDuration += readDroByte() << 16;
//  songDuration += readDroByte() << 24;

  //droFile.seek(23);
  droOffset = 23;
  codeShortDelay = readDroByte();
  codeLongDelay  = readDroByte();
  registerMapLength = readDroByte();

  //for (byte i = 0; i < registerMapLength; i ++) {
  //  registerMap[i] = readDroByte();
  //}
}

uint8_t getRegisterFromCode(uint8_t code)
{
  return pgm_read_byte_near(load_005_dro + code + 26);
}



int playDroSong() {
  byte code = readDroByte();
  byte data = readDroByte();

  if (code == codeShortDelay) {
    return data + 1;
  } else if (code == codeLongDelay) {
    return (data + 1) << 8;
  } else if (code < 128) {
    //opl2.write(registerMap[code], data);
    Opl2Instrument1._opl2.write(getRegisterFromCode(code), data);
  }

  return 0;
}
