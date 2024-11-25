#pragma once

#include "ofxMidi.h"
#include "ofUtils.h"
#include "ofxCortex/utils/AudioUtils.h"

namespace ofxCortex::io::midi {

class ClockListener : public ofxMidiListener {
public:
  ClockListener()
  {
    midiIn.openPort("IAC Driver Bus 1");
    midiIn.ignoreTypes(true, false, true);
    midiIn.setVerbose(true);
//    midiIn.addListener(this);
    
    ofAddListener(ofEvents().update, this, &ClockListener::updateHandler);
  }
  
  ~ClockListener()
  {
    ofRemoveListener(ofEvents().update, this, &ClockListener::updateHandler);
  }
  
  inline static std::shared_ptr<ClockListener> create() { return std::make_shared<ClockListener>(); }
  
  ofEvent<ofxCortex::core::utils::BeatEvent> onBeatE;
  
private:
  ofxMidiIn midiIn;
  uint64_t beatCounter { 0 };
  float lastBeatTime { 0 };
  
  std::vector<ofxMidiMessage> messages;
  void updateHandler(ofEventArgs & e)
  {
    if (midiIn.hasWaitingMessages()) {  // Check if messages are in the queue
      ofxMidiMessage midiMessage;
      while (midiIn.getNextMessage(midiMessage))
      {
        processMessage(midiMessage);
      }
    }
  }
  
  virtual void newMidiMessage(ofxMidiMessage& msg) override {} // Just a placeholder for now
  
  void processMessage(ofxMidiMessage & msg)
  {
    if (msg.status == MIDI_NOTE_ON)
    {
      beatCounter = 0;
    }
    if (msg.status == MIDI_TIME_CLOCK) {
      beatCounter = (beatCounter + 1) % Divisions::BAR;

      ofxCortex::core::utils::BeatEvent e;
      e.isBar = (beatCounter % Divisions::BAR) == 0;
      e.isHalf = (beatCounter % Divisions::HALF) == 0;
      e.isQuarter = (beatCounter % Divisions::QUARTER) == 0;
      e.is8th = (beatCounter % Divisions::EIGHT) == 0;
      e.is16th = (beatCounter  % Divisions::SIXTEENTH) == 0;
      e.is32th = (beatCounter  % Divisions::THIRTYSECONDTH) == 0;

      onBeatE.notify(e);
    }
  }
  
  enum Divisions : int {
    BAR = 96,
    HALF = BAR >> 1,
    QUARTER = HALF >> 1,
    EIGHT = QUARTER >> 1,
    SIXTEENTH = EIGHT >> 1,
    THIRTYSECONDTH = SIXTEENTH >> 1
  };
};

}
