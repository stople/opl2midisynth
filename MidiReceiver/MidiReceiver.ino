#include <Bounce2.h>

//Floppy
//#include <FloppyInstrument.h>
//#include <TimerOne.h>
//boolean firstRun = true; // Used for one-run-only stuffs;

//OPL2
//#include <midi_instruments.h>
extern const unsigned char *midiInstruments[];

#include <Opl2Instrument.h>
byte CurrentOplProgram = 0;

//Instrument control
bool BuzzerEnabled = false;
//bool FloppyEnabled = false;
bool Opl2Enabled = true;
int ActiveInstruments = 1;

int PotmeterLock = -1;
byte LastPotmeterReadoutTime = 0;
//const int BuzzerPin = 4;

enum {
  DebugTogglePin = A0,
  InstrumentCyclePin = A1,
  DemoPin = A2,
  ExtraButtonPin = A3,
  PotmeterPin = A4,

  BuzzerPin = 4
};

//const int DebugTogglePin = A0;
//const int InstrumentCyclePin = A1;
//const int DemoPin = A2;
//const int ExtraButtonPin = A3;

//const int PotmeterPin = A4;

//int DebugMode = 0;
//int CurrentInstrument = 0;
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
//Floppy
  //Floppy1.init(2, 3);

  //Opl2
  Opl2Instrument1.onSystemReset();

  //Buzzer
  pinMode(BuzzerPin, OUTPUT);

  //Buttons
  for (int i = 0; i < NUM_BUTTONS; i++) {
    buttons[i].attach( BUTTON_PINS[i] , INPUT  );       //setup the bounce instance for the current button
    buttons[i].interval(25);              // interval in ms
  }

  PotmeterLock = analogRead(PotmeterPin);

  /*
    pinMode(DebugTogglePin, INPUT);
    pinMode(InstrumentCyclePin, INPUT);
    pinMode(DemoPin, INPUT);
    pinMode(ExtraButtonPin, INPUT);
*/
//Midi input
  Serial.begin(57600);
  /*while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.write(STARTUP_MSG);*/
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
        //if (pitch == currentFloppyKey) Floppy1.stopTone();
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

        /*
        //Control key?
        if (pitch == 21) //A0 - toggle buzzer
        {
          toggleInstrument(&BuzzerEnabled);
          return;
        }
        //if (pitch == 22) //Bb0 - toggle floppy
        //{
        //  toggleInstrument(&FloppyEnabled);
        //  return;
        //}
        if (pitch == 23) //B0 - toggle OPL2
        {
          toggleInstrument(&Opl2Enabled);
          if (!Opl2Enabled)
          {
            Opl2Instrument1.onSystemReset();
            CurrentOplProgram = 0;
          }
          return;
        }
        if (pitch == 24 && Opl2Enabled) //C1 - Cycle instruments
        {
          incrementMidiInstrument();
          return;
        }
        */


        if (Opl2Enabled) Opl2Instrument1.onNoteOn(channel, pitch, velocity);
        
        //currentKey = pitch;
        
        if (noteOn)
        {
          int freq = midiToFrequency(pitch);

/*
          if (BuzzerEnabled && FloppyEnabled)
          {
            //Split on C4 (60)
            if (pitch < 60)
            {
              currentFloppyKey = pitch;
              Floppy1.setTone(freq);
            }
            else
            {
              currentBuzzerKey = pitch;
              tone(BuzzerPin, freq);
            }
          }
          else if (FloppyEnabled)
          {
              currentFloppyKey = pitch;
              Floppy1.setTone(freq);
          }
          else */if (BuzzerEnabled)
          {
              currentBuzzerKey = pitch;
              tone(BuzzerPin, freq);
          }

          
          //if (BuzzerEnabled) tone(8, freq);
          //if (FloppyEnabled) Floppy1.setTone(freq);
        }
        else
        {
          if (pitch == currentBuzzerKey) noTone(BuzzerPin);
          //if (pitch == currentFloppyKey) Floppy1.stopTone();
        }
        break;
  
      }
    }
  }  
}

void tick()
{

  //Floppy1.togglePin();
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
      //needToToggleLed = true;
      Serial.print("button ");
      Serial.println(i);

      /*if (i == 0)
      {
        DebugMode++;
        if (DebugMode > 1) DebugMode = 0;
        Serial.print("DebugTogglePin toggled, new mode ");
        Serial.println(DebugMode);
      }*/

      menuInput(i);
      
    }
  }

  //Potensiometer
  //Require certain offset before using the value after menu option is set
  int potmeter = analogRead(PotmeterPin); //0-1023

  if (PotmeterLock != -1 && abs(PotmeterLock - potmeter) < 256) return; //Require 1/4 turn before activating
  PotmeterLock = -1;

  byte diff = (byte)millis() - LastPotmeterReadoutTime;

  //if ((byte)millis() - LastPotmeterReadoutTime < 100) return; //100 ms between each update
  if (diff < 100) return; //100 ms between each update
  LastPotmeterReadoutTime = (byte)millis();
  setCurrentParameterFromPotmeter(potmeter);


/*  
  int debounce = 0;
  //const int debounceDelay = 50;
  const long debounceDelay = 200;
  int debugToggleReading = digitalRead(DebugTogglePin);
  int debugToggleReading = digitalRead(DebugTogglePin);
  int debugToggleReading = digitalRead(DebugTogglePin);
  int debugToggleReading = digitalRead(DebugTogglePin);
  
  if (digitalRead(DebugTogglePin) != DebugTogglePinState) debounce = 1;
  if (digitalRead(InstrumentCyclePin) != InstrumentCyclePinState) debounce = 1;
  if (digitalRead(DemoPin) != DemoPinState) debounce = 1;
  if (digitalRead(ExtraButtonPin) != ExtraButtonPinState) debounce = 1;

  if (debounce != 0) lastDebounceTime = millis();
  //Serial.println(debounce);
  Serial.println(digitalRead(DebugTogglePin));

  if ((millis() - lastDebounceTime) > debounceDelay)
  {
          //Serial.println(millis() - lastDebounceTime);
          //Serial.println(millis());
          //Serial.println(lastDebounceTime);

    if (DebugTogglePinState == LOW && digitalRead(DebugTogglePin) == HIGH) {
      DebugMode++;
      if (DebugMode > 1) DebugMode = 0;
      Serial.print("DebugTogglePin toggled, new mode ");
      Serial.println(DebugMode);
    }
    
    DebugTogglePinState = digitalRead(DebugTogglePin);
    InstrumentCyclePinState = digitalRead(InstrumentCyclePin);
    DemoPinState = digitalRead(DemoPin);
    ExtraButtonPinState = digitalRead(ExtraButtonPin);
  }

  DebugTogglePinLast = digitalRead(DebugTogglePin);
  InstrumentCyclePinLast = digitalRead(InstrumentCyclePin);
  DemoPinLast = digitalRead(DemoPin);
  ExtraButtonPinLast = digitalRead(ExtraButtonPin);
*/  
}

void loop() {
  //The first loop, reset all the drives, and wait 2 seconds...
  /*if (firstRun)
  {
    firstRun = false;
    //Floppy1.firstRun();

    Floppy1.reset();
    delay(2000);
    Timer1.initialize(1136);
    Timer1.attachInterrupt(tick);
    Timer1.stop();

  }*/
/*
  if (toggleCtr > 440)
  {
    cli();
    toggleCtr = 0;
    sei();
    Serial.println("tick * 440");
    
  }
  */
  //readMidiFromSerial();

  readButtons();
  if (debugMode.val) debugInterface();
  else readMidiFromSerial();
  //debugInterface();
  
}
/*
struct MenuOption {
  uint8_t parent;
  uint8_t options;
  uint8_t next;
  uint8_t child;
}

enum Menu {
  MENU_ROOT = 0,
  INSSELECT = 1,
  DEBUGSEL = 2,
}

#define MENU 1
#define MENU_ 1
MenuOption menu[200];
#define 

const menu[MENU_ROOT] = {
  
}


const unsigned char MENU_ROOT_D[2]   PROGMEM = { INSSELECT, 0x33, 0x5A, 0xB2, 0x50, 0x01, 0x00, 0x31, 0x00, 0xB1, 0xF5, 0x01 };
*/

//Copied from ArduinoUserInterface

//
// definition of an entry in the menu table
//



const char textPIANO1[] PROGMEM = "PIANO1";
const char textPIANO2[] PROGMEM = "PIANO2";
const char textPIANO3[] PROGMEM = "PIANO3";
const char textHONKTONK[] PROGMEM = "HONKTONK";
const char textEP1[] PROGMEM = "EP1";
const char textEP2[] PROGMEM = "EP2";
const char textHARPSIC[] PROGMEM = "HARPSIC";
const char textCLAVIC[] PROGMEM = "CLAVIC";
const char textCELESTA[] PROGMEM = "CELESTA";
const char textGLOCK[] PROGMEM = "GLOCK";
const char textMUSICBOX[] PROGMEM = "MUSICBOX";
const char textVIBES[] PROGMEM = "VIBES";
const char textMARIMBA[] PROGMEM = "MARIMBA";
const char textXYLO[] PROGMEM = "XYLO";
const char textTUBEBELL[] PROGMEM = "TUBEBELL";
const char textSANTUR[] PROGMEM = "SANTUR";
const char textORGAN1[] PROGMEM = "ORGAN1";
const char textORGAN2[] PROGMEM = "ORGAN2";
const char textORGAN3[] PROGMEM = "ORGAN3";
const char textPIPEORG[] PROGMEM = "PIPEORG";
const char textREEDORG[] PROGMEM = "REEDORG";
const char textACORDIAN[] PROGMEM = "ACORDIAN";
const char textHARMONIC[] PROGMEM = "HARMONIC";
const char textBANDNEON[] PROGMEM = "BANDNEON";
const char textNYLONGT[] PROGMEM = "NYLONGT";
const char textSTEELGT[] PROGMEM = "STEELGT";
const char textJAZZGT[] PROGMEM = "JAZZGT";
const char textCLEANGT[] PROGMEM = "CLEANGT";
const char textMUTEGT[] PROGMEM = "MUTEGT";
const char textOVERDGT[] PROGMEM = "OVERDGT";
const char textDISTGT[] PROGMEM = "DISTGT";
const char textGTHARMS[] PROGMEM = "GTHARMS";
const char textACOUBASS[] PROGMEM = "ACOUBASS";
const char textFINGBASS[] PROGMEM = "FINGBASS";
const char textPICKBASS[] PROGMEM = "PICKBASS";
const char textFRETLESS[] PROGMEM = "FRETLESS";
const char textSLAPBAS1[] PROGMEM = "SLAPBAS1";
const char textSLAPBAS2[] PROGMEM = "SLAPBAS2";
const char textSYNBASS1[] PROGMEM = "SYNBASS1";
const char textSYNBASS2[] PROGMEM = "SYNBASS2";
const char textVIOLIN[] PROGMEM = "VIOLIN";
const char textVIOLA[] PROGMEM = "VIOLA";
const char textCELLO[] PROGMEM = "CELLO";
const char textCONTRAB[] PROGMEM = "CONTRAB";
const char textTREMSTR[] PROGMEM = "TREMSTR";
const char textPIZZ[] PROGMEM = "PIZZ";
const char textHARP[] PROGMEM = "HARP";
const char textTIMPANI[] PROGMEM = "TIMPANI";
const char textSTRINGS[] PROGMEM = "STRINGS";
const char textSLOWSTR[] PROGMEM = "SLOWSTR";
const char textSYNSTR1[] PROGMEM = "SYNSTR1";
const char textSYNSTR2[] PROGMEM = "SYNSTR2";
const char textCHOIR[] PROGMEM = "CHOIR";
const char textOOHS[] PROGMEM = "OOHS";
const char textSYNVOX[] PROGMEM = "SYNVOX";
const char textORCHIT[] PROGMEM = "ORCHIT";
const char textTRUMPET[] PROGMEM = "TRUMPET";
const char textTROMBONE[] PROGMEM = "TROMBONE";
const char textTUBA[] PROGMEM = "TUBA";
const char textMUTETRP[] PROGMEM = "MUTETRP";
const char textFRHORN[] PROGMEM = "FRHORN";
const char textBRASS1[] PROGMEM = "BRASS1";
const char textSYNBRAS1[] PROGMEM = "SYNBRAS1";
const char textSYNBRAS2[] PROGMEM = "SYNBRAS2";
const char textSOPSAX[] PROGMEM = "SOPSAX";
const char textALTOSAX[] PROGMEM = "ALTOSAX";
const char textTENSAX[] PROGMEM = "TENSAX";
const char textBARISAX[] PROGMEM = "BARISAX";
const char textOBOE[] PROGMEM = "OBOE";
const char textENGLHORN[] PROGMEM = "ENGLHORN";
const char textBASSOON[] PROGMEM = "BASSOON";
const char textCLARINET[] PROGMEM = "CLARINET";
const char textPICCOLO[] PROGMEM = "PICCOLO";
const char textFLUTE1[] PROGMEM = "FLUTE1";
const char textRECORDER[] PROGMEM = "RECORDER";
const char textPANFLUTE[] PROGMEM = "PANFLUTE";
const char textBOTTLEB[] PROGMEM = "BOTTLEB";
const char textSHAKU[] PROGMEM = "SHAKU";
const char textWHISTLE[] PROGMEM = "WHISTLE";
const char textOCARINA[] PROGMEM = "OCARINA";
const char textSQUARWAV[] PROGMEM = "SQUARWAV";
const char textSAWWAV[] PROGMEM = "SAWWAV";
const char textSYNCALLI[] PROGMEM = "SYNCALLI";
const char textCHIFLEAD[] PROGMEM = "CHIFLEAD";
const char textCHARANG[] PROGMEM = "CHARANG";
const char textSOLOVOX[] PROGMEM = "SOLOVOX";
const char textFIFTHSAW[] PROGMEM = "FIFTHSAW";
const char textBASSLEAD[] PROGMEM = "BASSLEAD";
const char textFANTASIA[] PROGMEM = "FANTASIA";
const char textWARMPAD[] PROGMEM = "WARMPAD";
const char textPOLYSYN[] PROGMEM = "POLYSYN";
const char textSPACEVOX[] PROGMEM = "SPACEVOX";
const char textBOWEDGLS[] PROGMEM = "BOWEDGLS";
const char textMETALPAD[] PROGMEM = "METALPAD";
const char textHALOPAD[] PROGMEM = "HALOPAD";
const char textSWEEPPAD[] PROGMEM = "SWEEPPAD";
const char textICERAIN[] PROGMEM = "ICERAIN";
const char textSOUNDTRK[] PROGMEM = "SOUNDTRK";
const char textCRYSTAL[] PROGMEM = "CRYSTAL";
const char textATMOSPH[] PROGMEM = "ATMOSPH";
const char textBRIGHT[] PROGMEM = "BRIGHT";
const char textGOBLIN[] PROGMEM = "GOBLIN";
const char textECHODROP[] PROGMEM = "ECHODROP";
const char textSTARTHEM[] PROGMEM = "STARTHEM";
const char textSITAR[] PROGMEM = "SITAR";
const char textBANJO[] PROGMEM = "BANJO";
const char textSHAMISEN[] PROGMEM = "SHAMISEN";
const char textKOTO[] PROGMEM = "KOTO";
const char textKALIMBA[] PROGMEM = "KALIMBA";
const char textBAGPIPE[] PROGMEM = "BAGPIPE";
const char textFIDDLE[] PROGMEM = "FIDDLE";
const char textSHANNAI[] PROGMEM = "SHANNAI";
const char textTINKLBEL[] PROGMEM = "TINKLBEL";
const char textAGOGO[] PROGMEM = "AGOGO";
const char textSTEELDRM[] PROGMEM = "STEELDRM";
const char textWOODBLOK[] PROGMEM = "WOODBLOK";
const char textTAIKO[] PROGMEM = "TAIKO";
const char textMELOTOM[] PROGMEM = "MELOTOM";
const char textSYNDRUM[] PROGMEM = "SYNDRUM";
const char textREVRSCYM[] PROGMEM = "REVRSCYM";
const char textFRETNOIS[] PROGMEM = "FRETNOIS";
const char textBRTHNOIS[] PROGMEM = "BRTHNOIS";
const char textSEASHORE[] PROGMEM = "SEASHORE";
const char textBIRDS[] PROGMEM = "BIRDS";
const char textTELEPHON[] PROGMEM = "TELEPHON";
const char textHELICOPT[] PROGMEM = "HELICOPT";
const char textAPPLAUSE[] PROGMEM = "APPLAUSE";
const char textGUNSHOT[] PROGMEM = "GUNSHOT";

const char* const midiInstrumentNames[] PROGMEM = {
  textPIANO1,   textPIANO2,   textPIANO3,   textHONKTONK, textEP1,      textEP2,
  textHARPSIC,  textCLAVIC,   textCELESTA,  textGLOCK,    textMUSICBOX, textVIBES,
  textMARIMBA,  textXYLO,     textTUBEBELL, textSANTUR,   textORGAN1,   textORGAN2,
  textORGAN3,   textPIPEORG,  textREEDORG,  textACORDIAN, textHARMONIC, textBANDNEON,
  textNYLONGT,  textSTEELGT,  textJAZZGT,   textCLEANGT,  textMUTEGT,   textOVERDGT,
  textDISTGT,   textGTHARMS,  textACOUBASS, textFINGBASS, textPICKBASS, textFRETLESS,
  textSLAPBAS1, textSLAPBAS2, textSYNBASS1, textSYNBASS2, textVIOLIN,   textVIOLA,
  textCELLO,    textCONTRAB,  textTREMSTR,  textPIZZ,     textHARP,     textTIMPANI,
  textSTRINGS,  textSLOWSTR,  textSYNSTR1,  textSYNSTR2,  textCHOIR,    textOOHS,
  textSYNVOX,   textORCHIT,   textTRUMPET,  textTROMBONE, textTUBA,     textMUTETRP,
  textFRHORN,   textBRASS1,   textSYNBRAS1, textSYNBRAS2, textSOPSAX,   textALTOSAX,
  textTENSAX,   textBARISAX,  textOBOE,     textENGLHORN, textBASSOON,  textCLARINET,
  textPICCOLO,  textFLUTE1,   textRECORDER, textPANFLUTE, textBOTTLEB,  textSHAKU,
  textWHISTLE,  textOCARINA,  textSQUARWAV, textSAWWAV,   textSYNCALLI, textCHIFLEAD,
  textCHARANG,  textSOLOVOX,  textFIFTHSAW, textBASSLEAD, textFANTASIA, textWARMPAD,
  textPOLYSYN,  textSPACEVOX, textBOWEDGLS, textMETALPAD, textHALOPAD,  textSWEEPPAD,
  textICERAIN,  textSOUNDTRK, textCRYSTAL,  textATMOSPH,  textBRIGHT,   textGOBLIN,
  textECHODROP, textSTARTHEM, textSITAR,    textBANJO,    textSHAMISEN, textKOTO,
  textKALIMBA,  textBAGPIPE,  textFIDDLE,   textSHANNAI,  textTINKLBEL, textAGOGO,
  textSTEELDRM, textWOODBLOK, textTAIKO,    textMELOTOM,  textSYNDRUM,  textREVRSCYM,
  textFRETNOIS, textBRTHNOIS, textSEASHORE, textBIRDS,    textTELEPHON, textHELICOPT,
  textAPPLAUSE, textGUNSHOT
};

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

/*const byte MENU_ITEM_TYPE_MAIN_MENU_HEADER = 0;
const byte MENU_ITEM_TYPE_SUB_MENU_HEADER  = 1;
const byte MENU_ITEM_TYPE_SUB_MENU         = 2;
const byte MENU_ITEM_TYPE_COMMAND          = 3;
const byte MENU_ITEM_TYPE_TOGGLE           = 4;
const byte MENU_ITEM_TYPE_END_OF_MENU      = 5;
const byte MENU_ITEM_TYPE_PARAMETER        = 6;
*/



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
//  {MENU_ITEM_TYPE_SUB_MENU,        "External instrument",  NULL,           blinkMenu, NULL},
  {MENU_ITEM_TYPE_PARAMETER,         textMidiInstrument,  setMidiInstrument, NULL,      &midiInstrument},
  {MENU_ITEM_TYPE_SUB_MENU,          textInstrumentTweak, NULL,              tweakMenu, NULL},
//  {MENU_ITEM_TYPE_SUB_MENU,          "Blink",           NULL,                       blinkMenu},
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
    //float flPot = (float)potmeter;
    //float flNewVal = (float)potmeter / 1023 * p->max;
    
    //int newVal = potmeter / (1024 / (p->max + 1));
    //int newVal = (int)flNewVal;
    int newVal = (int)((float)potmeter / 1024 * (p->max + 1));
    if (newVal > p->max) newVal = p->max;
    if (newVal != p->val)
    {
      p->val = newVal;
      drawMenuOption();
      if (p->OnChange) p->OnChange();

    }
    //p->val = (int)((float)potmeter / 1023 * p->max);
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
    //Serial.print(i, HEX);
    data = (byte)i;
    PrintHex8(&data, 1);
    Serial.print(" ");
    for (j = 0; j < 16; ++j)
    {
      data = Opl2Instrument1._opl2.getRegister(i + j);
      PrintHex8(&data, 1);
      //Serial.print(Opl2Instrument1._opl2.getRegister(i + j), HEX);
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
  /*
  //byte data;
  
  Serial.print("SetInstrument ch:");
  //data = (byte)channel;
    PrintHex8(&channel, 1);

  //Serial.print(channel);
  Serial.print(" ptr ");
  
  //data = (byte)((uint16_t)instrument >> 8);
    PrintHex8(instrument, 2);

  
  //Serial.print((uint16_t)instrument);
  Serial.print(" percch:");
    PrintHex8(&percussionChannel, 1);
  //Serial.print(percussionChannel);
  Serial.print("\n");
*/
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
