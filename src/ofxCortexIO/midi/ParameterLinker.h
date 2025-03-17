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
  };
  
  ~ParameterLinker() {
    ofRemoveListener(ofEvents().update, this, &ParameterLinker::update);
    ofRemoveListener(ofEvents().keyPressed, this, &ParameterLinker::keyPressed);
    
    midiIn.closePort();
    midiIn.removeListener(this);
  };
  
  void setup(int port)
  {
    midiIn.listInPorts();
    
    midiIn.openPort(port);
    midiIn.ignoreTypes(false, false, false);
    midiIn.addListener(this);
    midiIn.setVerbose(true);
  }
  
  void link(const ofAbstractParameter & parameter)
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
        std::cout << "Link: " << messageHash << " <=> '" << queuedParameters.front()->getName() << "'" << std::endl;
        links[messageHash] = queuedParameters.front();
        queuedParameters.pop_front();
      }
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
      auto sliderFloatView = dynamic_pointer_cast<ofxCortex::ui::Slider<float>>(focusedView);
      auto sliderIntView = dynamic_pointer_cast<ofxCortex::ui::Slider<int>>(focusedView);
      auto checkboxView = dynamic_pointer_cast<ofxCortex::ui::Checkbox>(focusedView);
      auto buttonView = dynamic_pointer_cast<ofxCortex::ui::Button>(focusedView);
      
      if (sliderFloatView && sliderFloatView->hasParameter()) link(sliderFloatView->getParameter());
      if (sliderIntView && sliderIntView->hasParameter()) link(sliderIntView->getParameter());
      if (checkboxView && checkboxView->hasParameter()) link(checkboxView->getParameter());
      if (buttonView && buttonView->hasParameter()) link(buttonView->getParameter());
    }
  }
  
  bool isParameterLinkable(const ofAbstractParameter & param)
  {
    return param.valueType() == typeid(float).name() || param.valueType() == typeid(int).name() || param.valueType() == typeid(bool).name() || param.valueType() == typeid(void).name();
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
};

}}}
