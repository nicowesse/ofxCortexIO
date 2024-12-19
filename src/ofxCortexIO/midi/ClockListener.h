#pragma once

#include "ofxMidi.h"
#include "ofUtils.h"
#include "ofxCortex/utils/AudioUtils.h"
#include "ofxCortex/types/Status.h"
#include "ofxCortex/types/OutputValue.h"

namespace ofxCortex::io::midi {

class ClockListener : public ofxMidiListener {
public:
  ClockListener()
  {
    midiIn.ignoreTypes(false, false, false);
    midiIn.setVerbose(true);
    midiIn.addListener(this);
    
    this->setup("IAC Driver Bus 1");
    
    auto devices = midiIn.getInPortList();
    ofxCortex::core::types::Select<int> currentDevices;
    for (int i = 0; i < devices.size(); i++)
    {
      currentDevices.add(i, devices[i]);
    }
    devicesDropdown.setName("Device");
    devicesDropdown = currentDevices;
    onDeviceChange = devicesDropdown.newListener([this](ofxCortex::core::types::Select<int> & param){
      this->setup(param.getSelectedString());
    });
    
    parameters.setName("MIDI Clock Listener");
    parameters.add(devicesDropdown, status, BPM);
    status.setSerializable(false);
    BPM.setSerializable(false);
    
    lastPulseTime = std::chrono::high_resolution_clock::now();
    
    ofAddListener(ofEvents().update, this, &ClockListener::updateHandler);
  }
  
  ~ClockListener()
  {
    ofRemoveListener(ofEvents().update, this, &ClockListener::updateHandler);
  }
  
  void setup(const std::string & portName)
  {
    midiIn.closePort();
    
    if (midiIn.openPort(portName)) status = ofxCortex::types::Status::CONNECTED;
    else { status = ofxCortex::types::Status::DISCONNECTED; }
  }
  
  inline static std::shared_ptr<ClockListener> create() { return std::make_shared<ClockListener>(); }
  
  
  operator ofParameterGroup&() { return parameters; }
  float getBPM() const { return actualBPM; }
  
  ofEvent<ofxCortex::core::utils::BeatEvent> onBeatE;
  
private:
  ofxMidiIn midiIn;
  uint64_t beatCounter { 0 };
  
  float actualBPM { 120 };
  
  ofParameterGroup parameters;
  ofParameter<ofxCortex::types::OutputValue> BPM { "BPM", ofxCortex::types::OutputValue(120, "BPM") };
  ofParameter<ofxCortex::types::Status> status { "Status", ofxCortex::types::Status::CONNECTING };
  ofParameter<ofxCortex::core::types::Select<int>> devicesDropdown;
  ofEventListener onDeviceChange;
  
  std::chrono::high_resolution_clock::time_point lastPulseTime;
  std::deque<float> intervals;
  
  virtual void newMidiMessage(ofxMidiMessage &msg) override
  {
//    std::cout << "MIDI Status = " << ofxMidiMessage::getStatusString(msg.status) << " Beat Counter = " << beatCounter << std::endl;
    
    if (msg.status == MIDI_TIME_CLOCK)
    {
      ofxCortex::core::utils::BeatEvent e;
      e.isBar = (beatCounter % Divisions::BAR) == 0;
      e.isHalf = (beatCounter % Divisions::HALF) == 0;
      e.isQuarter = (beatCounter % Divisions::QUARTER) == 0;
      e.is8th = (beatCounter % Divisions::EIGHT) == 0;
      e.is16th = (beatCounter % Divisions::SIXTEENTH) == 0;
      e.is32th = (beatCounter % Divisions::THIRTYSECONDTH) == 0;

      onBeatE.notify(e);
      
      beatCounter = (beatCounter + 1) % Divisions::BAR;
      
      auto currentPulseTime = std::chrono::high_resolution_clock::now();
      
      auto intervalFine = std::chrono::duration_cast<std::chrono::microseconds>(currentPulseTime - lastPulseTime).count() * 0.001;
      lastPulseTime = currentPulseTime;
      
      if (intervalFine > 0.0) intervals.push_back(intervalFine);
      if ((beatCounter % Divisions::BEAT) == 0)
      {
        float averageInterval = ofxCortex::core::utils::Array::average(intervals) * 0.001;
        
        float intervalToBPM = actualBPM = 60.0 / (averageInterval * Divisions::BEAT);
        
        auto updatedBPM = BPM.get();
        updatedBPM = round(actualBPM);
        
        BPM = updatedBPM;
        
        intervals.clear();
      }
      
      status = ofxCortex::types::Status::RECEIVING;
    }
    
    if (msg.status == MIDI_STOP)
    {
      status = ofxCortex::types::Status::CONNECTED;
      beatCounter = 0;
    }
  }
  
  uint64_t lastPollingTime { 0 };
  const uint64_t pollingInterval { 2000 };
  void updateHandler(ofEventArgs & e)
  {
//    if (midiIn.hasWaitingMessages()) {  // Check if messages are in the queue
//      ofxMidiMessage midiMessage;
//      while (midiIn.getNextMessage(midiMessage))
//      {
//        processMessage(midiMessage);
//      }
//    }
    
    auto currentTime = ofGetElapsedTimeMillis();
    if (currentTime - lastPollingTime)
    {
      auto devices = midiIn.getInPortList();
      
      if (devicesDropdown->size() != devices.size())
      {
        ofxCortex::core::types::Select<int> currentDevices;
        for (int i = 0; i < devices.size(); i++)
        {
          currentDevices.add(i, devices[i]);
        }
        devicesDropdown.setWithoutEventNotifications(currentDevices);
      }
      
      lastPollingTime = currentTime;
    }
  }
  
  enum Divisions : int {
    BAR = 96,
    HALF = BAR >> 1,
    QUARTER = HALF >> 1,
    BEAT = QUARTER,
    EIGHT = QUARTER >> 1,
    SIXTEENTH = EIGHT >> 1,
    THIRTYSECONDTH = SIXTEENTH >> 1
  };
};

}
