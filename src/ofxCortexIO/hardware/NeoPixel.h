#pragma once

#include "ofPolyline.h"
#include "ofxTweenzor.h"
#include "ofxCortex/utils/ContainerUtils.h"
#include "Devices.h"

namespace ofxCortex { namespace io { namespace hardware {

struct NeoPixel : public LED {
public:
  enum class DataType : int { RGB = 3, RGBW = 4 };
  
public:
  template<typename ... T>
  static std::shared_ptr<NeoPixel> create(T&& ... t) {
    struct EnableMakeShared : public NeoPixel { EnableMakeShared(T&&... arg) : NeoPixel(std::forward<T>(arg)...) {} };
    return std::make_shared<EnableMakeShared>(std::forward<T>(t)...);
  }
  
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
        ofSetColor(16, 64);
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
    if (dataType == DataType::RGB) return std::vector<uint8_t>({ c.g, c.r, c.b });
    else return std::vector<uint8_t>({ c.g, c.r, c.b, c.a });
  }
  
  virtual unsigned int getChannelCount() override { return (int) dataType; }
  
protected:
  NeoPixel(const ofFloatColor & color = ofFloatColor(0, 0), unsigned int index = 0, DataType type = DataType::RGBW) : LED(color, index), dataType(type) {};
  DataType dataType;
};

class NeoPixelStrip : public ofxCortex::io::hardware::ArtnetDevice {
public:
  template<typename ... T>
  inline static std::shared_ptr<NeoPixelStrip> create(T&& ... t) {
    struct EnableMakeShared : public NeoPixelStrip { EnableMakeShared(T&&... arg) : NeoPixelStrip(std::forward<T>(arg)...) {} };
    
    return std::make_shared<EnableMakeShared>(std::forward<T>(t)...);
  }
  
  static std::shared_ptr<NeoPixelStrip> fromPolyline(const ofPolyline & line, unsigned int ledsPerMeter = 144, NeoPixel::DataType type = NeoPixel::DataType::RGBW, const glm::vec3 & normal = glm::vec3(0, 1, 0), size_t indexOffset = 0)
  {
    int ledCount = (line.getPerimeter() / 1000.0) * ledsPerMeter;
    ofPolyline spacedLine = line.getResampledByCount(ledCount * 2);
    
    auto instance = NeoPixelStrip::create(ledsPerMeter, type);
    for (int i = 1; i < spacedLine.size(); i += 2)
    {
      const auto & v = spacedLine[i];
      auto led = ofxCortex::io::hardware::NeoPixel::create(ofFloatColor(0, 0), i, type);
      led->index = i + indexOffset;
      led->setParent(*instance);
      led->setPosition(v);
      led->lookAt(led->getPosition() + normal);
      
      instance->leds.push_back(led);
    }
    
    return instance;
  }
  
  static std::shared_ptr<NeoPixelStrip> fromDirection(const glm::vec3 & direction, size_t count, const glm::vec3 & normal = glm::vec3(0, 1, 0), unsigned int ledsPerMeter = 144, NeoPixel::DataType type = NeoPixel::DataType::RGBW, size_t indexOffset = 0)
  {
    auto instance = NeoPixelStrip::create(ledsPerMeter, type);
    for (int i = 0; i < count; i++)
    {
      auto led = ofxCortex::io::hardware::NeoPixel::create(ofFloatColor(0, 0), i + indexOffset, type);
      led->setParent(*instance);
      led->setPosition(direction * instance->getSpacing() * i);
      led->lookAt(led->getPosition() + normal);
      
      instance->leds.push_back(led);
    }
    
    return instance;
  }
  
  inline virtual void draw() override
  {
    for (auto & led : leds) led->draw();
  }
  
  inline std::shared_ptr<NeoPixel> & operator[](unsigned int i) { return leds[i]; }
  inline const std::shared_ptr<NeoPixel> & operator[](unsigned int i) const { return leds[i]; }
  
  inline const std::vector<std::shared_ptr<NeoPixel>> & getLeds() const { return leds; }
  inline std::vector<std::shared_ptr<NeoPixel>> & getLeds() { return leds; }
  inline size_t size() const { return leds.size(); }
  
  virtual std::vector<uint8_t> getData() override
  {
    return ofxCortex::core::utils::Array::accumulate<std::vector<uint8_t>>(leds, [](std::vector<uint8_t> carry, std::shared_ptr<ofxCortex::io::hardware::NeoPixel> led){
      const auto & data = led->getData();
      carry.insert(carry.end(), data.begin(), data.end());
      return carry;
    });
  }
  
//  inline virtual unsigned int getChannelCount() override { return ofxCortex::core::utils::Array::accumulate<unsigned int>(leds, [](unsigned int carry, const std::shared_ptr<ofxCortex::io::hardware::NeoPixel> & led){ return carry + led->getChannelCount(); }); };
  inline virtual unsigned int getChannelCount() override { return (int) dataType; };
  
  void setStripColor(const ofFloatColor & c, float animationTime = 0.0f)
  {
    if (ofIsFloatEqual(animationTime, 0.0f)) for (auto & led : leds) led->color = c;
    else for (auto & led : leds) Tweenzor::add(&led->color, led->color, c, 0.0f, animationTime, EASE_IN_OUT_QUINT);
  }
  
protected:
  NeoPixelStrip(unsigned int ledsPerMeter = 144, NeoPixel::DataType type = NeoPixel::DataType::RGBW) : ledsPerMeter(ledsPerMeter), dataType(type) {};
  
  inline float getSpacing() const { return 1000.0 / ledsPerMeter; }
  
  unsigned int ledsPerMeter;
  NeoPixel::DataType dataType;
  std::vector<std::shared_ptr<NeoPixel>> leds;
};

}}}
