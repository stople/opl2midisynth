
#ifndef OPL2_INSTRUMENT_h
#define OPL2_INSTRUMENT_h

#include "Arduino.h"
#include <OPL2.h>

class Opl2Instrument
{
  
	
  public:
  
  static const byte MIDI_NUM_CHANNELS = 16;
  static const byte MIDI_DRUM_CHANNEL = 10;
  
  static const byte CONTROL_VOLUME = 7;
  static const byte CONTROL_ALL_SOUND_OFF = 120;
  static const byte CONTROL_RESET_ALL = 121;
  static const byte CONTROL_ALL_NOTES_OFF = 123;
  
  void onNoteOn(byte channel, byte note, byte velocity);
  void onNoteOff(byte channel, byte note, byte velocity);
  void onProgramChange(byte channel, byte program);
  void onControlChange(byte channel, byte control, byte value);
  void onSystemReset();

  	OPL2 _opl2;

  private:

// Channel mapping to keep track of MIDI to OPL2 channel mapping.
struct ChannelMapping {
	byte midiChannel;
	byte midiNote;
	float midiVelocity;
	float op1Level;
	float op2Level;
};

  
	ChannelMapping _channelMap[OPL2_NUM_CHANNELS];
	byte _oldestChannel[OPL2_NUM_CHANNELS];
	byte _programMap[MIDI_NUM_CHANNELS];
	float _channelVolumes[MIDI_NUM_CHANNELS]; 

  
  byte getFreeChannel(byte midiChannel);
  void setOpl2ChannelVolume(byte opl2Channel, byte midiChannel);
    
};

extern Opl2Instrument Opl2Instrument1;
#endif