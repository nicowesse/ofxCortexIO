#pragma once

#include "ofxCortex/utils/Helpers.h"
#include "ofxCortexIO/utils/Artnet.h"
#include "ofxCortexIO/hardware/Devices.h"

namespace ofxCortex { namespace io { namespace hardware {

class ArtnetController {
public:
  ArtnetController(size_t outputCount) {
    for (int i = 0; i < outputCount; i++)
    {
      outputs.insert({ i, std::vector<std::shared_ptr<ArtnetDevice>>() });
    }
  }
  
  ~ArtnetController() { clear(); }
  
  inline void connect(const std::string & IP) { isConnected = artnet.setup(IP); }
  
  inline void addDeviceToOutput(const std::shared_ptr<ArtnetDevice> & device, unsigned int port) { outputs[port].push_back(device); }
  
  template<typename DeviceType>
  inline void addDevicesToOutput(const std::vector<std::shared_ptr<DeviceType>> & devices, unsigned int port) { outputs[port].insert(outputs[port].end(), devices.begin(), devices.end()); }
  
  void drawStructure(const ofRectangle & bounds = ofGetCurrentViewport());
  
  void send();
  void clear();
  
protected:
  ofxArtnetSender artnet;
  bool isConnected { false };
  std::map<unsigned int, std::vector<std::shared_ptr<ArtnetDevice>>> outputs;
};

}}}
