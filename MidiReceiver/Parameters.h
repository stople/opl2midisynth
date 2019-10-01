#ifndef PARAMETERS_h
#define PARAMETERS_h

//Defines the parameters



typedef struct _PARAMETER
{
  byte val;
  const byte max;
  const char ** paramNames;
  void (*OnChange)();
} PARAMETER;


//Public functions
void incrementParameterWithChangeTrigger(void *par);



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

//Monitor placeholders
extern PARAMETER monMidiInput;
extern PARAMETER parQuarterTones;

#endif
