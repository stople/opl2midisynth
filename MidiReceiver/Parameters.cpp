#ifndef PARAMETERS_cpp
#define PARAMETERS_cpp

#include <Arduino.h>
#include "parameters.h"



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



//Public functions
void incrementParameterWithChangeTrigger(void *par)
{
  PARAMETER *p = (PARAMETER*)par;
  p->val++;
  if (p->val > p->max) p->val = 0;
  if (p->OnChange) p->OnChange();
}




//OnChange return functions
void setMidiInstrument();
void loadCustomInstrumentParameters();
void setCustomInstrument();



//Parameters in main menu
PARAMETER midiInstrument {0, 127, midiInstrumentNames, setMidiInstrument};
PARAMETER partMidiInstrument {0, 127, midiInstrumentNames, NULL};
PARAMETER partMidiPos {0, 10, NULL, NULL};
PARAMETER debugMode {0, 1, disableEnableNames, NULL};


//Parameters in instrument tweak
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

//Monitor placeholders
PARAMETER monMidiInput {0, 0, NULL, NULL};


#endif
