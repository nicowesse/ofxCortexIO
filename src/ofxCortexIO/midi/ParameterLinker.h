#pragma once

#include "ofParameter.h"
#include "ofLog.h"
#include "ofMath.h"
#include "ofxMidi.h"
#include "ofxCortex/utils/ParameterUtils.h"
#include "ofxCortex/utils/ContainerUtils.h"
#include "ofxCortex/utils/TimingUtils.h"
#include "ofxCortexUI.h"

namespace ofxCortex::io::midi {

class ParameterLinker {
public:
  ParameterLinker() {
    ofAddListener(ofEvents().update, this, &ParameterLinker::update);
    ofAddListener(ofEvents().keyPressed, this, &ParameterLinker::keyPressed);
    
    parameters.setName("Parameter Linker");
    parameters.add(devices);
  };
  
  ~ParameterLinker() {
    ofRemoveListener(ofEvents().update, this, &ParameterLinker::update);
    ofRemoveListener(ofEvents().keyPressed, this, &ParameterLinker::keyPressed);
    
    for (auto & [name, input] : midiInputs) input->closePort();
  }
  
  inline static std::shared_ptr<ParameterLinker> create() { return std::make_shared<ParameterLinker>(); }
  
  void queueLink(const ofAbstractParameter & parameter)
  {
    if (!isParameterLinkable(parameter))
    {
      ofLogWarning("ParameterLinker") << "Invalid type for parameter '" << parameter.getName() << "'. Only float, int or bool is accepted.";
      return;
    }
    
    queuedParameters.push_back(parameter.newReference());
  }
  
  bool saveLinks(const std::string & path = "links.json")
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
    
    if (shouldLink && focused) { queueLink(*focused.get()); }
    if (shouldUnlink && focused) { unlink(focused); }
  }
  
  operator ofParameterGroup&() { return parameters; }
  
protected:
//  virtual void newMidiMessage(ofxMidiMessage& msg) override {} // Just a placeholder for now
  
  bool link(const std::string & hash, std::shared_ptr<ofAbstractParameter> parameter)
  {
    bool linkIsTaken = links.count(hash);
    
    if (!linkIsTaken)
    {
      ofLogVerbose("🔗 ParameterLinker") << "Link: " << hash << " <=> '" << ofxCortex::core::utils::Parameters::serializeName(*parameter) << "'";
      
      links[hash] = parameter;
      ofxCortex::ui::linkedParameters.insert(ofxCortex::core::utils::Parameters::hash(*parameter));
      
      return true;
    }
    else
    {
      ofLogWarning("⚠️ ParameterLinker") << "Link for '" << hash << "' is already linked to '" << links[hash]->getName() << "'! Cmd+U to unlink.";
    }
    
    return false;
  }
  
  uint64_t lastPollingTime { 0 };
  const uint64_t pollingInterval { 2000 };
  void update(ofEventArgs & e)
  {
    for (auto & [name, input] : midiInputs)
    {
      if (input->hasWaitingMessages())
      {
        ofxMidiMessage message;
        while (input->getNextMessage(message))
        {
          processMessage(message);
        }
      }
    }
    
    static std::vector<std::string> ignoreDevices = {
      "IAC Driver Bus 1"
    };

    auto currentTime = ofGetElapsedTimeMillis();
    if (currentTime - lastPollingTime > pollingInterval)
    {
      for (auto & deviceName : ofxMidiIn::getMidiDevices())
      {
        if (ofContains(ignoreDevices, deviceName)) continue;
        
        auto input = std::make_shared<ofxMidiIn>();
        input->openPort(deviceName);
        input->ignoreTypes(true, true, true);
        
        midiInputs.insert({ deviceName, input });
        ignoreDevices.push_back(deviceName);
        
        ofParameter<ofxCortex::types::Status> device { deviceName, ofxCortex::types::Status::CONNECTED };
        devices.add(device);
      }
      
      lastPollingTime = currentTime;
    }
  }
  
  void processMessage(ofxMidiMessage & msg)
  {
    std::string messageHash = getMessageHash(msg);
    
    auto deviceParameter = devices[msg.portName].cast<ofxCortex::types::Status>();
    auto ref = deviceParameter.get();
    ref = ofxCortex::types::Status::RECEIVING;
    deviceParameter.set(ref);
    
    if (queuedParameters.size() && queuedParameters.front() != nullptr && link(messageHash, queuedParameters.front())) { queuedParameters.pop_front(); }
    
    if (links.count(messageHash))
    {
      const auto & parameter = links[messageHash];
      
      ofLogNotice("ParameterLinker::processMessage()") << "[" << messageHash << "] Value = " << msg.value << " => '" << ofxCortex::core::utils::Parameters::serializeName(*parameter) << "'";
      
      if (parameter->valueType() == typeid(float).name())
      {
        ofParameter<float> & casted = parameter->cast<float>();
        casted = ofMap(msg.value, 0, 127, casted.getMin(), casted.getMax());
      }
      
      if (parameter->valueType() == typeid(int).name())
      {
        ofParameter<int> & casted = parameter->cast<int>();
        casted = ofMap(msg.value, 0, 127, casted.getMin(), casted.getMax());
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
      
      if (parameter->valueType() == typeid(ofFloatColor).name())
      {
        auto & casted = parameter->cast<ofFloatColor>();
        auto ref = casted.get();
        ref.setHue(ofMap(msg.value, 0, 127, 0.0, 1.0));
        casted.set(ref);
      }
      
      if (parameter->valueType() == typeid(ofxCortex::core::types::Select<int>).name())
      {
        auto & casted = parameter->cast<ofxCortex::core::types::Select<int>>();
        
        auto ref = casted.get();
        ref.setSelectedIndex((int)ofMap(msg.value, 0, 126, 0, ref.size(), true));
        
        casted.set(ref);
      }
      
      if (parameter->valueType() == typeid(ofxCortex::core::types::BeatDivision).name())
      {
        auto & casted = parameter->cast<ofxCortex::core::types::BeatDivision>();
        
        bool isSlider = msg.value > 0 && msg.value < 127;
        
        auto ref = casted.get();
        
        if (msg.value == 127 && !isSlider) ref++; // Toggle
        else if (isSlider)
        {
          ref.setSelectedIndex((int) ofMap(msg.value, 1, 126, 0, ref.size(), true));
        }
        
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
    for (auto & [key, parameter] : links)
    {
      if (ofxCortex::core::utils::Parameters::hash(*parameter) == hashed)
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
  
//  ofxMidiIn midiIn;
  std::unordered_map<std::string, std::shared_ptr<ofxMidiIn>> midiInputs;
  
  ofParameterGroup parameters;
//  ofParameter<std::vector<std::string>> devices { "Devices", std::vector<std::string>() };
  ofParameterGroup devices { "Devices" };
  ofEventListener onDeviceChanged;
};

}
