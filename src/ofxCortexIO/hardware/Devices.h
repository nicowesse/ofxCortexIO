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
};

struct ArtnetDevice : public Device {
  virtual std::vector<uint8_t> getData() = 0;
  virtual unsigned int getChannelCount() = 0;
  
  virtual void draw() = 0;
  
protected:
  ArtnetDevice() = default;
};

struct LED : public ArtnetDevice {
  ofFloatColor color { 0, 0 };
  uint_fast64_t index;
  
  static std::shared_ptr<LED> create() { return std::make_shared<LED>(); }
  
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
  
};

struct NeoPixel : public LED {
  static std::shared_ptr<NeoPixel> create() { return std::make_shared<NeoPixel>(); }
  
  static std::string deviceName() { return "NeoPixel"; }
  
  virtual void draw() override
  {
    ofPushMatrix();
    {
      ofMultMatrix(getGlobalTransformMatrix());
      ofRotateXDeg(-90);
      ofScale(1.0);
      
      ofPushStyle();
      {
        ofSetColor(16);
        ofFill();
        ofDrawBox(0, 1, 0, 7, 2, 7);
        
        ofSetColor(255, 64);
        ofNoFill();
        ofDrawBox(0, 1, 0, 7, 2, 7);
        
        ofSetColor(getDisplayColor());
        ofFill();
        ofDrawSphere(0, 2, 1.5);
      }
      ofPopStyle();
    }
    ofPopMatrix();
  }
  
  virtual std::vector<uint8_t> getData() override
  {
    const ofColor & c = getRGBW();
    return std::vector<uint8_t>({ c.g, c.r, c.b, c.a });
  }
  
  virtual unsigned int getChannelCount() override { return 4; }
};

}}}
