#pragma once

#include "ofxCortex/utils/Helpers.h"
#include "ofxCortexIO/utils/Artnet.h"
#include "ofxCortexIO/hardware/Devices.h"

namespace ofxCortex { namespace io { namespace hardware {

class ArtnetController {
public:
  ~ArtnetController() { 
    clear();
    outputs.clear();
    
    address.removeListener(this, &ArtnetController::onAddressChanged);
  }
  
  template<typename ... T>
  static std::shared_ptr<ArtnetController> create(T&& ... t) {
    struct EnableMakeShared : public ArtnetController { EnableMakeShared(T&&... arg) : ArtnetController(std::forward<T>(arg)...) {} };
    
    return std::make_shared<EnableMakeShared>(std::forward<T>(t)...);
  }
  
  inline void connect(const std::string & IP) { address.setWithoutEventNotifications(IP); isConnected = artnet.setup(IP); }
  
  inline void addDeviceToOutput(const std::shared_ptr<ArtnetDevice> & device, unsigned int port) { outputs[port].push_back(device); }
  
  template<typename DeviceType>
  inline void addDevicesToOutput(const std::vector<std::shared_ptr<DeviceType>> & devices, unsigned int port) { outputs[port].insert(outputs[port].end(), devices.begin(), devices.end()); }
  
  void resetOutputs();
  
  void drawStructure(const ofRectangle & bounds = ofGetCurrentViewport());
  
  void send();
  void clear();
  
  operator ofParameterGroup&() { return parameters; }
  
protected:
  ArtnetController(size_t outputCount) {
    parameters.setName("Artnet Controller");
    parameters.add(address);
    address.addListener(this, &ArtnetController::onAddressChanged);
    
    for (int i = 0; i < outputCount; i++)
    {
      outputs.insert({ i, std::vector<std::shared_ptr<ArtnetDevice>>() });
    }
    
    artnet.disableThread();
  }
  
  ofxArtnetSender artnet;
  bool isConnected { false };
  unsigned long lastTransmit { 0 };
  std::map<unsigned int, std::vector<std::shared_ptr<ArtnetDevice>>> outputs;
  
  // Parameters
  ofParameterGroup parameters;
  ofParameter<std::string> address { "IP", "0.0.0.0" };
  void onAddressChanged(std::string & address) { this->connect(address); }
};

}}}
