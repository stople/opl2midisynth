// OPL2 MIDI synthesizer
//
// Dependencies:
// ArduinoOPL2 by DhrBaksteen
// Bounce2 for button press
//
// Input: Stream of MIDI data
// Output: OPL2 data stream


#include <Bounce2.h>

#include "midi_instruments_names.h"

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


typedef struct _PARAMETER
{
  byte val;
  const byte max;
  const char ** paramNames;
  void (*OnChange)();
} PARAMETER;

const char textDisabled[] PROGMEM = "Disabled";
const char textEnabled[] PROGMEM = "Enabled";
const char* const disableEnableNames[] PROGMEM = {textDisabled, textEnabled};
PARAMETER debugMode {0, 1, disableEnableNames, NULL};

void incrementParameter(void *par)
{
  PARAMETER *p = (PARAMETER*)par;
  p->val++;
  if (p->val > p->max) p->val = 0;
  if (p->OnChange) p->OnChange();
}




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

//Copied from ArduinoUserInterface

//
// definition of an entry in the menu table
//




PARAMETER midiInstrument {0, 127, midiInstrumentNames, setMidiInstrument};

PARAMETER partMidiInstrument {0, 127, midiInstrumentNames, NULL};
PARAMETER partMidiPos {0, 10, NULL, NULL};


typedef struct _MENU_ITEM
{
  byte MenuItemType;
  const char *MenuItemText;
  void (*MenuItemFunction)();
  _MENU_ITEM *MenuItemSubMenu;
  _PARAMETER *MenuItemParameter;
} MENU_ITEM;


//
// menu item types
//

enum {
  MENU_ITEM_TYPE_MAIN_MENU_HEADER = 0,
  MENU_ITEM_TYPE_SUB_MENU_HEADER  = 1,
  MENU_ITEM_TYPE_SUB_MENU         = 2,
  MENU_ITEM_TYPE_COMMAND          = 3,
  MENU_ITEM_TYPE_TOGGLE           = 4,
  MENU_ITEM_TYPE_END_OF_MENU      = 5,
  MENU_ITEM_TYPE_PARAMETER        = 6
} MENU_ITEM_TYPE;



const char textDebugInput[] PROGMEM = "Debug input";
const char textMidiInstrument[] PROGMEM = "Midi instrument";
const char textInstrumentTweak[] PROGMEM = "Instrument tweak";
const char textDumpOplReg[] PROGMEM = "Dump opl registers";
const char textDumpCurrentInstrument[] PROGMEM = "Dump current instrument";
const char textAbout[] PROGMEM = "About";
const char textPartMidiInstrument[] PROGMEM = "Part midi instrument";
const char textPartMidiPos[] PROGMEM = "Part midi pos";

extern MENU_ITEM tweakMenu[];

MENU_ITEM mainMenu[] = {
  {MENU_ITEM_TYPE_MAIN_MENU_HEADER,  NULL,                NULL,              mainMenu,  NULL},
  {MENU_ITEM_TYPE_PARAMETER,         textDebugInput,      NULL,              NULL,      &debugMode},
  {MENU_ITEM_TYPE_PARAMETER,         textMidiInstrument,  setMidiInstrument, NULL,      &midiInstrument},
  {MENU_ITEM_TYPE_SUB_MENU,          textInstrumentTweak, NULL,              tweakMenu, NULL},
  {MENU_ITEM_TYPE_COMMAND,           textAbout,           NULL,    NULL, NULL},
  {MENU_ITEM_TYPE_COMMAND,           textDumpOplReg,      dumpOplReg,    NULL, NULL},
  {MENU_ITEM_TYPE_COMMAND,           textDumpCurrentInstrument,      dumpCurrentInstrument,    NULL, NULL},
  {MENU_ITEM_TYPE_PARAMETER,         textPartMidiInstrument,  NULL, NULL,      &partMidiInstrument},
  {MENU_ITEM_TYPE_PARAMETER,         textPartMidiPos,  setPartMidiPos, NULL,      &partMidiPos},
  {MENU_ITEM_TYPE_END_OF_MENU,       "",                NULL,                       NULL, NULL}
};






const char textOperator[] PROGMEM = "Operator";
const char textWaveformSelect[] PROGMEM = "Waveform select";
const char textTremolo[] PROGMEM = "Tremolo";
const char textVibrato[] PROGMEM = "Vibrato";
const char textSustainEnabled[] PROGMEM = "Sustain enabled";
const char textKSR[] PROGMEM = "KSR";
const char textFreqMult[] PROGMEM = "Frequency multiplier";
const char textKeyScaleLevel[] PROGMEM = "Key scale level";
const char textOutputLevel[] PROGMEM = "Output level";
const char textAttack[] PROGMEM = "Attack";
const char textDecay[] PROGMEM = "Decay";
const char textSustain[] PROGMEM = "Sustain";
const char textRelease[] PROGMEM = "Release";
const char textModFbFactor[] PROGMEM = "Modulation feedback factor";
const char textSynthType[] PROGMEM = "Synth mode";
const char textWaveform[] PROGMEM = "Waveform";
const char textFNumber[] PROGMEM = "Frequency F-number";
const char textDeepTremolo[] PROGMEM = "Deep tremolo";
const char textDeepVibrato[] PROGMEM = "Deep vibrato";


const char textFULL[] PROGMEM = "FULL";
const char textPOSITIVE_HALF[] PROGMEM = "POSITIVE_HALF";
const char textPOSITIVE_FULL[] PROGMEM = "POSITIVE_FULL";
const char textPOSITIVE_CAPPED[] PROGMEM = "POSITIVE_CAPPED";
const char* const waveFormNames[] PROGMEM = {textFULL, textPOSITIVE_HALF, textPOSITIVE_FULL, textPOSITIVE_CAPPED};

const char textOp2Only[] PROGMEM = "Op2Only";
const char textBothOperators[] PROGMEM = "Both operators";
const char* const synthModeNames[] PROGMEM = {textOp2Only, textBothOperators};

PARAMETER parOperator {0, 1, NULL, loadCustomInstrumentParameters};

PARAMETER parWaveformSelect {0, 1, disableEnableNames, setCustomInstrument};
PARAMETER parTremolo {0, 1, disableEnableNames, setCustomInstrument};
PARAMETER parVibrato {0, 1, disableEnableNames, setCustomInstrument};
PARAMETER parSustainEnabled {0, 1, disableEnableNames, setCustomInstrument};
PARAMETER parKSR {0, 1, disableEnableNames, setCustomInstrument};
PARAMETER parFreqMult {0, 15, NULL, setCustomInstrument};
PARAMETER parKeyScaleLevel {0, 3, NULL, setCustomInstrument};
PARAMETER parOutputLevel {0, 63, NULL, setCustomInstrument};
PARAMETER parAttack {0, 15, NULL, setCustomInstrument};
PARAMETER parDecay {0, 15, NULL, setCustomInstrument};
PARAMETER parSustain {0, 15, NULL, setCustomInstrument};
PARAMETER parRelease {0, 15, NULL, setCustomInstrument};
PARAMETER parModFbFactor {0, 7, NULL, setCustomInstrument};
PARAMETER parSynthType {0, 1, synthModeNames, setCustomInstrument};
PARAMETER parWaveform {0, 3, waveFormNames, setCustomInstrument};
PARAMETER parFNumber {0, 1023, NULL, setCustomInstrument};
PARAMETER parDeepTremolo {0, 1, disableEnableNames, setCustomInstrument};
PARAMETER parDeepVibrato {0, 1, disableEnableNames, setCustomInstrument};


MENU_ITEM tweakMenu[] = {
  {MENU_ITEM_TYPE_SUB_MENU_HEADER,   "",                             NULL,       mainMenu, NULL},
  {MENU_ITEM_TYPE_PARAMETER,         textOperator,                   NULL,       NULL,     &parOperator},
  {MENU_ITEM_TYPE_PARAMETER,         textTremolo,                    NULL,       NULL,     &parTremolo},
  {MENU_ITEM_TYPE_PARAMETER,         textVibrato,                    NULL,       NULL,     &parVibrato},
  {MENU_ITEM_TYPE_PARAMETER,         textSustainEnabled,             NULL,       NULL,     &parSustainEnabled},
  {MENU_ITEM_TYPE_PARAMETER,         textKSR,                        NULL,       NULL,     &parKSR},
  {MENU_ITEM_TYPE_PARAMETER,         textFreqMult,                   NULL,       NULL,     &parFreqMult},
  {MENU_ITEM_TYPE_PARAMETER,         textKeyScaleLevel,              NULL,       NULL,     &parKeyScaleLevel},
  {MENU_ITEM_TYPE_PARAMETER,         textOutputLevel,                NULL,       NULL,     &parOutputLevel},
  {MENU_ITEM_TYPE_PARAMETER,         textAttack,                     NULL,       NULL,     &parAttack},
  {MENU_ITEM_TYPE_PARAMETER,         textDecay,                      NULL,       NULL,     &parDecay},
  {MENU_ITEM_TYPE_PARAMETER,         textSustain,                    NULL,       NULL,     &parSustain},
  {MENU_ITEM_TYPE_PARAMETER,         textRelease,                    NULL,       NULL,     &parRelease},
  {MENU_ITEM_TYPE_PARAMETER,         textModFbFactor,                NULL,       NULL,     &parModFbFactor},
  {MENU_ITEM_TYPE_PARAMETER,         textSynthType,                  NULL,       NULL,     &parSynthType},
  {MENU_ITEM_TYPE_PARAMETER,         textWaveform,                   NULL,       NULL,     &parWaveform},
  {MENU_ITEM_TYPE_PARAMETER,         textFNumber,                    NULL,       NULL,     &parFNumber},
  {MENU_ITEM_TYPE_PARAMETER,         textDeepTremolo,                NULL,       NULL,     &parDeepTremolo},
  {MENU_ITEM_TYPE_PARAMETER,         textDeepVibrato,                NULL,       NULL,     &parDeepVibrato},
  {MENU_ITEM_TYPE_END_OF_MENU,       "",                             NULL,       NULL,     NULL}
};



MENU_ITEM* currentMenu = mainMenu;
int currentMenuIndex = 1;

void menuInput(int button)
{
  MENU_ITEM* m = &currentMenu[currentMenuIndex];
  if (button == 0) //Go back
  {
    currentMenu = currentMenu[0].MenuItemSubMenu;
    //currentMenuOption = mainMenu[1];
    currentMenuIndex = 1;
    drawMenuOption();
  }
  else if (button == 1) //Left
  {
    currentMenuIndex--;
    if (currentMenuIndex <= 0)
    {
      while (currentMenu[++currentMenuIndex].MenuItemType != MENU_ITEM_TYPE_END_OF_MENU){}
      --currentMenuIndex;
    }
    drawMenuOption();
  }
  else if (button == 2) //Right
  {
    currentMenuIndex++;
    if (currentMenu[currentMenuIndex].MenuItemType == MENU_ITEM_TYPE_END_OF_MENU)
    {
      currentMenuIndex = 1;
    }
    drawMenuOption();
  }
  else if (button == 3) //Enter
  {
    switch (m->MenuItemType)
    {
      case MENU_ITEM_TYPE_SUB_MENU:
      currentMenu = m->MenuItemSubMenu;
      break;

      case MENU_ITEM_TYPE_PARAMETER:
      //incrementParameter((void*)m->MenuItemParameter);
      if (m->MenuItemParameter->OnChange) m->MenuItemParameter->OnChange();
      break;      
    }

    if (m->MenuItemFunction) m->MenuItemFunction();
    
    drawMenuOption();

  }
}

void printStringFromProgMem(char* s)
{
  for (byte k = 0; k < strlen_P(s); k++)
  {
    char c =  pgm_read_byte_near(s + k);
    Serial.print(c);
  }  
}

void drawMenuOption(){
  char buffer[40];
  MENU_ITEM* m = &currentMenu[currentMenuIndex];

  printStringFromProgMem(m->MenuItemText);

  if (m->MenuItemType == MENU_ITEM_TYPE_PARAMETER)
  {
    PARAMETER* p = m->MenuItemParameter;
    Serial.print(" (");
    Serial.print(p->val);
    if (p->paramNames)
    {
      Serial.print(" ");
      strcpy_P(buffer, (char*)pgm_read_word(&(p->paramNames[p->val]))); // Necessary casts and dereferencing, just copy.
      Serial.print(buffer);
    }
    Serial.print(")");
  }
  Serial.print("\n");  
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
  incrementParameter((void*)&midiInstrument);
  //setMidiInstrument();
}

void setCurrentParameterFromPotmeter(int potmeter)
{
  MENU_ITEM* m = &currentMenu[currentMenuIndex];
  if (m->MenuItemType == MENU_ITEM_TYPE_PARAMETER)
  {
    PARAMETER* p = m->MenuItemParameter;
    int newVal = (int)((float)potmeter / 1024 * (p->max + 1));
    if (newVal > p->max) newVal = p->max;
    if (newVal != p->val)
    {
      p->val = newVal;
      drawMenuOption();
      if (p->OnChange) p->OnChange();

    }
  }
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
