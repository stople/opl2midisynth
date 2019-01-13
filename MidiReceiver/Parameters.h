#ifndef PARAMETERS_h
#define PARAMETERS_h

//Defines the parameters


//Const string defines
#include "midi_instruments_names.h"

const char textDisabled[] PROGMEM = "Disabled";
const char textEnabled[] PROGMEM = "Enabled";
const char* const disableEnableNames[] PROGMEM = {textDisabled, textEnabled};

const char textFULL[] PROGMEM = "FULL";
const char textPOSITIVE_HALF[] PROGMEM = "POSITIVE_HALF";
const char textPOSITIVE_FULL[] PROGMEM = "POSITIVE_FULL";
const char textPOSITIVE_CAPPED[] PROGMEM = "POSITIVE_CAPPED";
const char* const waveFormNames[] PROGMEM = {textFULL, textPOSITIVE_HALF, textPOSITIVE_FULL, textPOSITIVE_CAPPED};

const char textOp2Only[] PROGMEM = "Op2Only";
const char textBothOperators[] PROGMEM = "Both operators";
const char* const synthModeNames[] PROGMEM = {textOp2Only, textBothOperators};




typedef struct _PARAMETER
{
  byte val;
  const byte max;
  const char ** paramNames;
  void (*OnChange)();
} PARAMETER;


//Public functions
void incrementParameterWithChangeTrigger(void *par);




//Return functions
void setMidiInstrument();
void loadCustomInstrumentParameters();
void setCustomInstrument();


//Parameters in main menu
extern PARAMETER midiInstrument;
extern PARAMETER partMidiInstrument;
extern PARAMETER partMidiPos;
extern PARAMETER debugMode;


//Parameters in instrument tweak
extern PARAMETER parOperator;
extern PARAMETER parWaveformSelect;
extern PARAMETER parTremolo;
extern PARAMETER parVibrato;
extern PARAMETER parSustainEnabled;
extern PARAMETER parKSR;
extern PARAMETER parFreqMult;
extern PARAMETER parKeyScaleLevel;
extern PARAMETER parOutputLevel;
extern PARAMETER parAttack;
extern PARAMETER parDecay;
extern PARAMETER parSustain;
extern PARAMETER parRelease;
extern PARAMETER parModFbFactor;
extern PARAMETER parSynthType;
extern PARAMETER parWaveform;
extern PARAMETER parFNumber;
extern PARAMETER parDeepTremolo;
extern PARAMETER parDeepVibrato;


#endif
