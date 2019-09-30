
#ifndef OPL2_INSTRUMENT_cpp
#define OPL2_INSTRUMENT_cpp

#include "Opl2Instrument.h"

#include <midi_instruments.h>
#include <midi_drums.h>

Opl2Instrument Opl2Instrument1; // preinstatiate


/**
 * Get a free OPL2 channel to play a note. If all channels are occupied then recycle the oldes one.
 */
byte Opl2Instrument::getFreeChannel(byte midiChannel) {  
	byte opl2Channel = 255;

	// Look for a free OPL2 channel.
	for (byte i = 0; i < OPL2_NUM_CHANNELS; i ++) {
		if (!_opl2.getKeyOn(i)) {
			opl2Channel = i;
			break;
		}
	}

	// If no channels are free then recycle the oldest, where drum channels will be the first to be recycled. Only if
	// no drum channels are left will the actual oldest channel be recycled.
	if (opl2Channel == 255) {
		opl2Channel = _oldestChannel[0];
		for (byte i = 0; i < OPL2_NUM_CHANNELS; i ++) {
			if (_channelMap[_oldestChannel[i]].midiChannel == MIDI_DRUM_CHANNEL) {
				opl2Channel = _oldestChannel[i];
			}
		}
	}

	// Update the list of last used channels by moving the current channel to the bottom so the last updated channel
	// will move to the front of the list. If no more OPL2 channels are free then the last updated one will be recycled.
	byte i;
	for (i = 0; i < OPL2_NUM_CHANNELS && _oldestChannel[i] != opl2Channel; i ++) {}

	while (i < OPL2_NUM_CHANNELS - 1) {
		byte temp = _oldestChannel[i + 1];
		_oldestChannel[i + 1] = _oldestChannel[i];
		_oldestChannel[i] = temp;
		i ++;
	}

	return opl2Channel;
}


/**
 * Set the volume of operators 1 and 2 of the given OPL2 channel according to the settings of the given MIDI channel.
 */
void Opl2Instrument::setOpl2ChannelVolume(byte opl2Channel, byte midiChannel) {
	float volume = _channelMap[opl2Channel].midiVelocity * _channelVolumes[midiChannel];
	byte volumeOp1 = round(_channelMap[opl2Channel].op1Level * volume * 63.0);
	byte volumeOp2 = round(_channelMap[opl2Channel].op2Level * volume * 63.0);
	_opl2.setVolume(opl2Channel, OPERATOR1, 63 - volumeOp1);
	_opl2.setVolume(opl2Channel, OPERATOR2, 63 - volumeOp2);
}





/**
 * Handle a note on MIDI event to play a note.
 */
void Opl2Instrument::onNoteOn(byte channel, byte note, byte velocity) {
  byte index = 255;
	// Treat notes with a velocity of 0 as note off.
	if (velocity == 0) {
		onNoteOff(channel, note, velocity);
		return;
	}

  //Is this note sustained? If so, reuse channel
  for (byte i = 0; i < OPL2_NUM_CHANNELS; i ++) {
    if (_channelMap[i].midiChannel == channel && _channelMap[i].midiNote == note && _opl2.getKeyOn(i)) {
      index = i;
      break;
    }
  }

	// Get an available OPL2 channel and setup instrument parameters.
	if (index == 255) index = getFreeChannel(channel);
	
	/*
	if (channel != MIDI_DRUM_CHANNEL) {
		_opl2.setInstrument(i, midiInstruments[_programMap[channel]]);
	} else {
		if (note >= DRUM_NOTE_BASE && note < DRUM_NOTE_BASE + NUM_MIDI_DRUMS) {
			_opl2.setInstrument(i, midiDrums[note - DRUM_NOTE_BASE]);
		} else {
			return;
		}
	}
	*/

	// Register channel mapping.
	_channelMap[index].midiChannel  = channel;
	_channelMap[index].midiNote     = note;
	_channelMap[index].midiVelocity = 1; //round(velocity / 127.0);
	_channelMap[index].op1Level     = round((63 - _opl2.getVolume(index, OPERATOR1)) / 63.0);
  _channelMap[index].op2Level     = round((63 - _opl2.getVolume(index, OPERATOR2)) / 63.0);
  _channelMap[index].isSustained  = false;

	// Set operator output levels based on note velocity.
	setOpl2ChannelVolume(index, channel);

	// Calculate octave and note number and play note!
	byte opl2Octave = 4;
	byte opl2Note = NOTE_C;
	/*if (channel != MIDI_DRUM_CHANNEL) {
		note = max(24, min(note, 119));
		opl2Octave = 1 + (note - 24) / 12;
		opl2Note   = note % 12;
	}*/
	_opl2.playNote(index, opl2Octave, opl2Note);
	
	//HACK - set frequency
	float logTone = 8.781; //log2(440) (A4)
	logTone += (float)(note - 69) / 12;
	double exptone = pow(2, logTone);
	_opl2.setFrequency(index, exptone);
}


/**
 * Handle a note off MIDI event to stop playing a note.
 */
void Opl2Instrument::onNoteOff(byte channel, byte note, byte velocity) {
	for (byte i = 0; i < OPL2_NUM_CHANNELS; i ++) {
		if (_channelMap[i].midiChannel == channel && _channelMap[i].midiNote == note) {
      if (_sustain)
      {
        _channelMap[i].isSustained = true;
      }
      else
      {
        _opl2.setKeyOn(i, false);
        _channelMap[i].isSustained = false;
      }
			break;
		}
	}
}

void Opl2Instrument::onSustain(byte value) {
  _sustain = value;

  //try to turn off sustained notes
  if (!_sustain)
  {
    for (byte i = 0; i < OPL2_NUM_CHANNELS; i ++) {
      if (_channelMap[i].isSustained) {
        _opl2.setKeyOn(i, false);
        _channelMap[i].isSustained = false;
      }
    }
  }
}

/**
 * Turn off all channels
 */
void Opl2Instrument::silence() {
  for (byte i = 0; i < OPL2_NUM_CHANNELS; i ++) {
    _opl2.setKeyOn(i, false);
  }
}



/**
 * Handle instrument change on the given MIDI channel.
 */
void Opl2Instrument::onProgramChange(byte channel, byte program) {
	_programMap[channel] = min(program, 127);
}

/**
 * Handle MIDI control changes on the given channel.
 */
void Opl2Instrument::onControlChange(byte channel, byte control, byte value) {
	switch (control) {

		// Change volume of a MIDI channel.
		case CONTROL_VOLUME: {
			_channelVolumes[channel] = round(min(value, 63) / 63.0);

			for (byte i = 0; i < OPL2_NUM_CHANNELS; i ++) {
				if (_channelMap[i].midiChannel == channel && _opl2.getKeyOn(i)) {
					setOpl2ChannelVolume(i, channel);
				}
			}
			break;
		}

		// Reset all controller values.
		case CONTROL_RESET_ALL:
			for (byte i = 0; i < MIDI_NUM_CHANNELS; i ++) {
				_channelVolumes[channel] = 0.8;
			}
		break;

		// Immediately silence all channels.
		// Intentionally cascade into CONTROL_ALL_NOTES_OFF!
		case CONTROL_ALL_SOUND_OFF:
			for (byte i = 0; i < OPL2_NUM_CHANNELS; i ++) {
				_opl2.setRelease(i, OPERATOR1, 0);
				_opl2.setRelease(i, OPERATOR2, 0);
			}

    // Silence all MIDI channels.
    case CONTROL_ALL_NOTES_OFF: {
      for (byte i = 0; i < OPL2_NUM_CHANNELS; i ++) {
        if (_channelMap[i].midiChannel == channel) {
          onNoteOff(_channelMap[i].midiChannel, _channelMap[i].midiNote, 0);
        }
      }
      break;
    }

    // Sustain pedal
    case CONTROL_SUSTAIN: {
      onSustain(value);
      break;
    }

		// Ignore any other MIDI controls.
		default:
			break;
	}
}


/**
 * Handle full system reset.
 */
void Opl2Instrument::onSystemReset() {
	_opl2.init();

	// Silence all channels and set default instrument.
	for (byte i = 0; i < OPL2_NUM_CHANNELS; i ++) {
		_opl2.setKeyOn(i, false);
		_opl2.setInstrument(i, midiInstruments[0]);
		_oldestChannel[i] = i;
	}

	// Reset default MIDI player parameters.
	for (byte i = 0; i < MIDI_NUM_CHANNELS; i ++) {
		_programMap[i] = 0;
		_channelVolumes[i] = 0.8;
	}
	
	
	
}


#endif
