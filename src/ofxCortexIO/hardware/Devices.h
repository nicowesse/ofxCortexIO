#pragma once

#include <memory>
#include "ofNode.h"
#include "ofColor.h"
#include "ofGraphics.h"
#include "of3dGraphics.h"

namespace ofxCortex { namespace io { namespace hardware {


//template<typename T>
struct Device : public ofNode {
public:
  static std::string deviceName() { return "Device"; };
  virtual std::string getDeviceName() { return deviceName(); }
  
protected:
  Device() = default;
  virtual ~Device() = default;
};

struct ArtnetDevice : public Device {
  virtual std::vector<uint8_t> getData() = 0;
  virtual unsigned int getChannelCount() = 0;
  
  virtual void draw() = 0;
  
protected:
  ArtnetDevice() = default;
};

struct LED : public ArtnetDevice {
public:
  ofFloatColor color { 0, 0 };
  uint_fast64_t index;
  
public:
  template<typename ... T>
  static std::shared_ptr<LED> create(T&& ... t) {
    struct EnableMakeShared : public LED { EnableMakeShared(T&&... arg) : LED(std::forward<T>(arg)...) {} };
    return std::make_shared<EnableMakeShared>(std::forward<T>(t)...);
  }
  
  static std::string deviceName() { return "LED"; }
  
  virtual void draw() override
  {
    ofPushMatrix();
    {
      ofMultMatrix(getGlobalTransformMatrix());
      
      ofPushStyle();
      {
        ofSetColor(getDisplayColor());
        ofDrawSphere(2.0);
      }
      ofPopStyle();
    }
    ofPopMatrix();
  }
  
  ofColor getRGBW() const { return ofColor(color); }
  
  virtual ofColor getDisplayColor() const
  {
    ofColor display = color;
    display += display.a;
    display.a = ofColor::limit();
    
    return display;
  }
  
  void clear()
  {
    this->color = ofFloatColor(0.0, 0.0);
  }
  
  virtual std::vector<uint8_t> getData() override
  {
    const ofColor & c = getRGBW();
    return std::vector<uint8_t>({ c.r, c.g, c.b, c.a });
  }
  
  virtual unsigned int getChannelCount() override { return 4; }
  
protected:
  LED(const ofFloatColor & color = ofFloatColor(0, 0), unsigned int index = 0) : color(color), index(index) {};
};

}}}
