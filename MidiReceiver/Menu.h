#ifndef MENU_h
#define MENU_h

#include "Parameters.h"

//Defines the menu


//_MENU_ITEM: partly reused definition from ArduinoUserInterface

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
  MENU_ITEM_TYPE_PARAMETER        = 6,
  MENU_ITEM_TYPE_MONITOR          = 7
} MENU_ITEM_TYPE;


extern const MENU_ITEM mainMenu[];
extern const MENU_ITEM tweakMenu[];
extern const MENU_ITEM midiToolsMenu[];

extern char monitorText[17];

void menuInput(int button);
void setCurrentParameterFromPotmeter(int potmeter);
void drawMenuOption();

MENU_ITEM* getCurrentMenuItem();

#endif
