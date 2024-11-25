#pragma once

#include "ofParameter.h"
#include "ofLog.h"
#include "ofMath.h"
#include "ofxMidi.h"
#include "ofxCortex/utils/ParameterUtils.h"
#include "ofxCortex/utils/TimingUtils.h"
#include "ofxCortexUI.h"

namespace ofxCortex { namespace io { namespace midi {

class ParameterLinker {
public:
  ParameterLinker() {
    ofAddListener(ofEvents().update, this, &ParameterLinker::update);
    ofAddListener(ofEvents().keyPressed, this, &ParameterLinker::keyPressed);
    
    auto devices = midiIn.getInPortList();
    auto deviceDropdownItems = ofxCortex::core::types::Select<int>();
    for (int i = 0; i < devices.size(); i++) { deviceDropdownItems.add(i, devices[i]); }
    
    parameters.setName("Parameter Linker");
    parameters.add(deviceDropdown.set("Device", deviceDropdownItems));
    
    onDeviceChanged = deviceDropdown.newListener([this](ofxCortex::core::types::Select<int> & param) {
      this->setup(param.selected());
    });
    
    auto timer = core::Timing::setInterval([&, this](){
      auto devices = midiIn.getInPortList();
      
      if (devices.size() != deviceDropdown->size())
      {
        auto deviceDropdownItems = ofxCortex::core::types::Select<int>();
        for (int i = 0; i < devices.size(); i++) { deviceDropdownItems.add(i, devices[i]); }
        deviceDropdown.set(deviceDropdownItems);
      }
      
    }, 10000);
  };
  
  ~ParameterLinker() {
    ofRemoveListener(ofEvents().update, this, &ParameterLinker::update);
    ofRemoveListener(ofEvents().keyPressed, this, &ParameterLinker::keyPressed);
    
    midiIn.closePort();
  }
  
  inline static std::shared_ptr<ParameterLinker> create() { return std::make_shared<ParameterLinker>(); }
  
  void setup(int port)
  {
    ofLogNotice("ParameterLinker::setup(" + ofToString(port) + ")");
    
    if (midiIn.getNumInPorts() == 0) return;
    
    midiIn.closePort();
    midiIn.openPort(port);
    midiIn.ignoreTypes(true, true, true);
//    midiIn.setVerbose(true);
    
    auto ref = deviceDropdown.get();
    ref.setSelectedIndex(port);
    deviceDropdown.setWithoutEventNotifications(ref);
  }
  
  void setup(const std::string & portName)
  {
    if (midiIn.getNumInPorts() == 0) return;
    
    auto devices = midiIn.getInPortList();
    size_t index = ofFind(devices, portName);
    size_t found = index != devices.size();
    
    if (!found) return;
    
    this->setup(index);
  }
  
  void queueLink(const ofAbstractParameter & parameter)
  {
    if (!isParameterLinkable(parameter))
    {
      ofLogWarning("ParameterLinker") << "Invalid type for parameter '" << parameter.getName() << "'. Only float, int or bool is accepted.";
      return;
    }
    
    queuedParameters.push_back(parameter.newReference());
  }
  
  bool saveLinks(string path = "links.json")
  {
    ofJson json;
    
    for (auto & [hash, parameter] : links)
    {
      json[hash] = ofxCortex::core::utils::Parameters::serializeName(*parameter);
    }
    
    bool isSaved = ofSavePrettyJson(path, json);
    if (isSaved) ofLogNotice("📥 ParameterLinker::saveLinks()") << "💾 Links saved to '" << path << "'!";
    return isSaved;
  }
  
  bool loadLinks(const ofParameterGroup & parameters, string path = "links.json")
  {
    ofFile file(path);
    
    if (file.exists())
    {
      ofJson json;
      file >> json;
      
      for (auto & [hash, parameterName] : json.items()) 
      {
        const auto & param = ofxCortex::core::utils::Parameters::getParameter(parameterName, parameters);
        link(hash, param.newReference());
      }
      
      ofLogNotice("💾 ParameterLinker::loadLinks()") << "💾 Links loaded from '" << path << "'!";
      return true;
    }
    else
    {
      ofLogNotice("⚠️ ParameterLinker::loadLinks()") << "💾 File could not be loaded from '" << path << "'! Is it there?";
      
      return false;
    }
  }
  
  std::string getLinksString() const
  {
    std::stringstream output;
    for (auto & [hash, param] : links)
    {
      output << hash << " <-> " << param->getName() << '\n';
    }
    
    return output.str();
  }
  
  void keyPressed(ofKeyEventArgs & e)
  {
    bool shouldLink = e.key == 'l' && e.hasModifier(OF_KEY_COMMAND);
    bool shouldUnlink = e.key == 'u' && e.hasModifier(OF_KEY_COMMAND);
    auto focused = ofxCortex::ui::focusedParameter;
    
    if (shouldLink && focused) {
      queueLink(*focused.get());
    }
    if (shouldUnlink && focused) {
      unlink(focused);
    }
  }
  
  operator ofParameterGroup&() { return parameters; }
  
protected:
  ofxMidiIn midiIn;
  
//  virtual void newMidiMessage(ofxMidiMessage& msg) override {} // Just a placeholder for now
  
  bool link(const std::string & hash, std::shared_ptr<ofAbstractParameter> parameter)
  {
    bool linkIsTaken = links.count(hash);
    
    if (!linkIsTaken)
    {
      ofLogVerbose("🔗 ParameterLinker") << "Link: " << hash << " <=> '" << parameter->getName() << "'";
      
      links[hash] = parameter;
      ofxCortex::ui::linkedParameters.insert(ofxCortex::core::utils::Parameters::hash(*parameter));
      
      return true;
    }
    else
    {
      ofLogWarning("⚠️ ParameterLinker") << "Link for '" << hash << "' is already taken! Cmd+U to unlink.";
    }
    
    return false;
  }
  
  void update(ofEventArgs & e)
  {
    if (midiIn.hasWaitingMessages())
    {
      ofxMidiMessage message;
      while (midiIn.getNextMessage(message))
      {
        processMessage(message);
      }
    }
  }
  
  void processMessage(ofxMidiMessage & msg)
  {
    std::string messageHash = getMessageHash(msg);
    
    if (queuedParameters.size() && queuedParameters.front() != nullptr && link(messageHash, queuedParameters.front()))
    {
        queuedParameters.pop_front();
    }
    
    if (links.count(messageHash))
    {
      const auto & parameter = links[messageHash];
      
      if (parameter->valueType() == typeid(float).name())
      {
        ofParameter<float> & casted = parameter->cast<float>();
        
        casted = ofMap(msg.value, 0, 126, casted.getMin(), casted.getMax());
      }
      
      if (parameter->valueType() == typeid(int).name())
      {
        ofParameter<int> & casted = parameter->cast<int>();
        
        casted = ofMap(msg.value, 0, 126, casted.getMin(), casted.getMax());
      }
      
      if (parameter->valueType() == typeid(bool).name())
      {
        ofParameter<bool> & casted = parameter->cast<bool>();
        
        bool isSlider = msg.value > 0 && msg.value < 127;
        
        if (msg.value == 127 && !isSlider) casted = !casted; // Toggle
        else if (isSlider)
        {
          bool value = msg.value >= 64;
          if (casted.get() != value) casted = value;
        }
      }
      
      if (parameter->valueType() == typeid(void).name())
      {
        ofParameter<void> & casted = parameter->cast<void>();
        
        if (msg.value > 0) casted.trigger();
      }
      
      if (parameter->valueType() == typeid(ofxCortex::core::types::Select<int>).name())
      {
        ofParameter<ofxCortex::core::types::Select<int>> & casted = parameter->cast<ofxCortex::core::types::Select<int>>();
        
        auto ref = casted.get();
        ref.setSelectedIndex((int)ofMap(msg.value, 0, 126, 0, ref.size(), true));
        
        casted.set(ref);
      }
    }
  }
  
  bool isParameterLinkable(const ofAbstractParameter & param)
  {
    return true; //param.valueType() == typeid(float).name() || param.valueType() == typeid(int).name() || param.valueType() == typeid(bool).name() || param.valueType() == typeid(void).name() || param.valueType() == typeid(ofxCortex::core::types::Select<int>).name();
  }
  
  std::string getMessageHash(const ofxMidiMessage & msg)
  {
    std::stringstream s;
    s << msg.portName << ":" << msg.status << ":" << msg.channel << ":" << msg.control;
    
    return s.str();
  }
  
  bool unlink(std::shared_ptr<ofAbstractParameter> & param)
  {
    
    std::string removeKey = "";
    bool found = false;
    
    auto hashed = ofxCortex::core::utils::Parameters::hash(*param);
    for (auto & [key, value] : links)
    {
      if (ofxCortex::core::utils::Parameters::hash(*value) == hashed)
      {
        removeKey = key;
        found = true;
      }
    }
    
    if (!found) return false;
    
    links.erase(links.find(removeKey));
    ofxCortex::ui::linkedParameters.erase(hashed);
    
    return found;
  }
  
  std::deque<std::shared_ptr<ofAbstractParameter>> queuedParameters;
  std::map<std::string, std::shared_ptr<ofAbstractParameter>> links;
  std::vector<ofxMidiIn> midis;
  
  ofParameterGroup parameters;
  ofParameter<ofxCortex::core::types::Select<int>> deviceDropdown;
  ofEventListener onDeviceChanged;
};

}}}
