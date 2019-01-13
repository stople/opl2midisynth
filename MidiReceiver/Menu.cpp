#ifndef MENU_cpp
#define MENU_cpp

#include <Arduino.h>
#include "Menu.h"



//Const string defines

//Main menu
const char textDebugInput[] PROGMEM = "Debug input";
const char textMidiInstrument[] PROGMEM = "Midi instrument";
const char textInstrumentTweak[] PROGMEM = "Instrument tweak";
const char textDumpOplReg[] PROGMEM = "Dump opl registers";
const char textDumpCurrentInstrument[] PROGMEM = "Dump current instrument";
const char textAbout[] PROGMEM = "About";
const char textPartMidiInstrument[] PROGMEM = "Part midi instrument";
const char textPartMidiPos[] PROGMEM = "Part midi pos";

//Tweak menu
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


void drawMenuOption();

//Public functions

static MENU_ITEM* currentMenu = mainMenu;
static int currentMenuIndex = 1;

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





//OnClick return functions
void setMidiInstrument();
void dumpOplReg();
void dumpCurrentInstrument();
void setPartMidiPos();




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



#endif