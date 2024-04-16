#pragma once

#include "ofParameter.h"
#include "ofLog.h"
#include "ofMath.h"
#include "ofxMidi.h"
#include "ofxCortexUI.h"

namespace ofxCortex { namespace io { namespace midi {

class ParameterLinker : public ofxMidiListener {
public:
  ParameterLinker() {
    ofAddListener(ofEvents().update, this, &ParameterLinker::update);
    ofAddListener(ofEvents().keyPressed, this, &ParameterLinker::keyPressed);
    
    midiIn.listInPorts();
  };
  
  ~ParameterLinker() {
    ofRemoveListener(ofEvents().update, this, &ParameterLinker::update);
    ofRemoveListener(ofEvents().keyPressed, this, &ParameterLinker::keyPressed);
    
    midiIn.closePort();
    midiIn.removeListener(this);
  };
  
  void setup(int port)
  {
    midiIn.openPort(port);
    midiIn.ignoreTypes(false, false, false);
    midiIn.addListener(this);
    midiIn.setVerbose(false);
  }
  
  void link(const std::shared_ptr<ofAbstractParameter> & parameterRef)
  {
    queuedParameters.push_back(parameterRef);
  }
  
  bool saveLinks(string path = "links.json")
  {
    ofJson json;
    
    for (auto & [hash, parameter] : links)
    {
      json[hash] = ofxCortex::core::utils::Parameters::serializeName(*parameter);
    }
    
    bool isSaved = ofSavePrettyJson(path, json);
    if (isSaved) ofLogNotice("ParameterLinker::saveLinks()") << "💾 Links saved to '" << path << "'!";
    return isSaved;
  }
  
  bool loadLinks(const ofParameterGroup & parameters, string path = "links.json")
  {
    ofFile file(path);
    
    if (file.exists())
    {
      ofJson json;
      file >> json;
      
      for (auto & [hash, parameterName] : json.items()) { links[hash] = ofxCortex::core::utils::Parameters::getParameter(parameterName, parameters).newReference(); }
      
      ofLogNotice("ParameterLinker::loadLinks()") << "💾 Links loaded from '" << path << "'!";
      return true;
    }
    else
    {
      ofLogNotice("ParameterLinker::loadLinks()") << "💾 File could not be loaded from '" << path << "'! Is it there?";
      
      return false;
    }
  }
  
  void enableMultilink() { multilinkEnabled = true; }
  void disableMultilink() { multilinkEnabled = false; }
  
protected:
  ofxMidiIn midiIn;
  std::deque<ofxMidiMessage> messages;
  
  void newMidiMessage(ofxMidiMessage& msg)
  {
    messages.push_back(msg);
  }
  
  void processMessage(const ofxMidiMessage & msg)
  {
    std::string messageHash = getMessageHash(msg);
    
    if (queuedParameters.size() && queuedParameters.front() != nullptr)
    {
      bool linkIsTaken = links.count(messageHash);
      
      if (!linkIsTaken)
      {
        ofLogNotice("ParameterLinker") << "Link: " << messageHash << " <=> '" << queuedParameters.front()->getName() << "'" << std::endl;
        links[messageHash] = queuedParameters.front();
        queuedParameters.pop_front();
      }
    }
    
    if (links.count(messageHash))
    {
      const std::shared_ptr<ofAbstractParameter> & parameter = links[messageHash];
      
      if (parameter->valueType() == typeid(float).name())
      {
        std::shared_ptr<ofxCortex::UnitParameter<float>> unitP = static_pointer_cast<ofxCortex::UnitParameter<float>>(parameter);
        ofParameter<float> & casted = (unitP) ? unitP->getParameter() : parameter->cast<float>();
        
        casted = ofMap(msg.value, 0, 126, casted.getMin(), casted.getMax(), true);
      }
      
      if (parameter->valueType() == typeid(int).name())
      {
        std::shared_ptr<ofxCortex::UnitParameter<int>> unitP = static_pointer_cast<ofxCortex::UnitParameter<int>>(parameter);
        ofParameter<int> & casted = (unitP) ? unitP->getParameter() : parameter->cast<int>();
        
        casted = ofMap(msg.value, 0, 126, casted.getMin(), casted.getMax(), true);
      }
      
      if (parameter->valueType() == typeid(bool).name())
      {
        std::shared_ptr<ofxCortex::UnitParameter<bool>> unitP = static_pointer_cast<ofxCortex::UnitParameter<bool>>(parameter);
        ofParameter<bool> & casted = (unitP) ? unitP->getParameter() : parameter->cast<bool>();
        
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
        std::shared_ptr<ofxCortex::UnitParameter<void>> unitP = static_pointer_cast<ofxCortex::UnitParameter<void>>(parameter);
        ofParameter<void> & casted = (unitP) ? unitP->getParameter() : parameter->cast<void>();
        
        if (msg.value > 0) casted.trigger();
      }
    }
  }
  
  void update(ofEventArgs & e)
  {
    while (messages.size() > 0)
    {
      this->processMessage(messages.front());
      messages.pop_front();
    }
  }
  
  void keyPressed(ofKeyEventArgs & e)
  {
    bool shouldLink = e.key == 'l' && e.hasModifier(OF_KEY_COMMAND);
    
    if (shouldLink) {
      auto focusedView = ofxCortex::ui::View::getFocused();
      auto parameterView = dynamic_pointer_cast<ofxCortex::ui::ParameterView>(focusedView);
      
      if (parameterView) {
        if (isParameterLinkable(parameterView->getParameterReference())) {
          parameterView->setLinkStatus(true);
          link(parameterView->getParameterReference());
        }
        else {
          ofLogNotice("⚠️ ParameterLinker") << "Parameter '" << parameterView->getParameterName() << "' has already been linked! Enable multi-link or pick another parameter.";
        }
      }
    }
  }
  
  bool isParameterLinkable(const std::shared_ptr<ofAbstractParameter> & paramRef)
  {
    bool linkExists = ofxCortex::core::utils::Array::accumulate<bool>(links, [&](bool carry, const std::map<std::string, std::shared_ptr<ofAbstractParameter>>::value_type & link){
      return carry || link.second == paramRef;
    }, false);
    
    bool isLinkable = multilinkEnabled || !linkExists;
    
    return isLinkable && (paramRef->valueType() == typeid(float).name() || paramRef->valueType() == typeid(int).name() || paramRef->valueType() == typeid(bool).name() || paramRef->valueType() == typeid(void).name());
  }
  
  std::string getMessageHash(const ofxMidiMessage & msg)
  {
    std::stringstream s;
    s << msg.portName << ":" << msg.status << ":" << msg.channel << ":" << msg.control;
    
    return s.str();
  }
  
  std::deque<std::shared_ptr<ofAbstractParameter>> queuedParameters;
  std::map<std::string, std::shared_ptr<ofAbstractParameter>> links;
  std::vector<ofxMidiIn> midis;
  
  bool multilinkEnabled { true };
};

}}}
