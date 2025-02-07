#include "NeoPixel.h"

namespace ofxCortex::io::hardware {

//void NeoPixel::draw() override
//{
//  ofPushMatrix();
//  {
//    ofMultMatrix(getGlobalTransformMatrix());
//    ofRotateXDeg(-90);
//    ofScale(1.0);
//    
//    ofPushStyle();
//    {
//      ofSetColor(16);
//      ofFill();
//      ofDrawBox(0, 1, 0, 7, 2, 7);
//      
//      ofSetColor(255, 64);
//      ofNoFill();
//      ofDrawBox(0, 1, 0, 7, 2, 7);
//      
//      ofSetColor(getDisplayColor());
//      ofFill();
//      ofDrawSphere(0, 2, 1.5);
//    }
//    ofPopStyle();
//  }
//  ofPopMatrix();
//}

//std::shared_ptr<NeoPixelStrip> NeoPixelStrip::fromPolyline(const ofPolyline & line, unsigned int ledsPerMeter)
//{
//  int ledCount = (line.getPerimeter() / 1000.0) * ledsPerMeter;
//  ofPolyline spacedLine = line.getResampledByCount(ledCount * 2);
//  
//  auto instance = NeoPixelStrip::create(ledsPerMeter);
//  for (int i = 1; i < spacedLine.size(); i += 2)
//  {
//    const auto & v = spacedLine[i];
//    auto led = ofxCortex::io::hardware::NeoPixel::create();
//    led->index = i;
//    led->setParent(*instance);
//    led->setPosition(v);
////    led->setOrientation(glm::vec3(90, 0, 0));
//    
//    instance->leds.push_back(led);
//  }
//  
//  return instance;
//}

//std::shared_ptr<NeoPixelStrip> NeoPixelStrip::fromDirection(const glm::vec3 & direction, size_t count, unsigned int ledsPerMeter)
//{
//  auto instance = NeoPixelStrip::create(ledsPerMeter);
//  for (int i = 0; i < count; i++)
//  {
//    auto led = ofxCortex::io::hardware::NeoPixel::create();
//    led->index = i;
//    led->setParent(*instance);
//    led->setPosition(direction * instance->getSpacing() * i);
//    
//    instance->leds.push_back(led);
//  }
//  
//  return instance;
//}

//std::vector<uint8_t> NeoPixelStrip::getData() override {
//  return utils::Array::accumulate<std::vector<uint8_t>>(leds, [](std::vector<uint8_t> carry, std::shared_ptr<ofxCortex::io::hardware::NeoPixel> led){
//    const auto & data = led->getData();
//    carry.insert(carry.end(), data.begin(), data.end());
//    return carry;
//  });
//}

//void NeoPixelStrip::setStripColor(const ofFloatColor & c, float animationTime)
//{
//  if (ofIsFloatEqual(animationTime, 0.0f)) for (auto & led : leds) led->color = c;
//  else for (auto & led : leds) Tweenzor::add(&led->color, led->color, c, 0.0f, animationTime, EASE_IN_OUT_QUINT);
//}

}
