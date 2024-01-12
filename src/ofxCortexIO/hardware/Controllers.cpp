#include "Controllers.h"

namespace ofxCortex { namespace io { namespace hardware {

void ArtnetController::send()
{
  std::vector<ofxArtnetMessage> packets;
  for (int i = 0; i < outputs.size(); i++)
  {
    const auto & devices = outputs.at(i);
    auto outputPackets = ofxCortex::io::utils::devicesToArtnet(devices, i * 4);
    ofxCortex::core::utils::Array::appendVector(packets, outputPackets);
  }
  ofxCortex::io::utils::sendPackets(artnet, packets);
}

void ArtnetController::clear()
{
  for (auto & [_, devices] : outputs)
  {
    for (auto & device : devices)
    {
      if (auto LED = std::dynamic_pointer_cast<ofxCortex::io::hardware::LED>(device)) LED->clear();
    }
  }
  
  this->send();
}

void ArtnetController::drawStructure(const ofRectangle & bounds)
{
  int structureColumns = 8;
  float spacingX = bounds.width / structureColumns;
  float spacingY = bounds.height / (outputs.size() + 1);
  
  //    float deviceSpacing = 10.0;
  float deviceHeight = 10;
  float deviceByteWidth = 11 * 3;
  float deviceWidth = deviceHeight; //deviceByteWidth * 4;
  float controllerWidth = 11 * artnet.getIP().size() + 0;
  float controllerHeight = 8 + 14;
  
  ofPushStyle();
  {
    ofSetRectMode(OF_RECTMODE_CORNER);
    ofSetColor(0, 32);
    ofDrawRectangle(bounds);
    
    ofSetRectMode(OF_RECTMODE_CENTER);
    ofNoFill();
    ofSetColor(255);
    ofDrawRectRounded(spacingX, bounds.getCenter().y, controllerWidth, controllerHeight, 2);
    
    ofDrawBitmapString(ofToString(artnet.getIP()), spacingX - (9 * 0.5 * artnet.getIP().size()), bounds.getCenter().y + 4);
    
    int columns = (spacingX * (structureColumns - 3.5)) / deviceWidth;
    
    for (auto & [port, devices] : outputs)
    {
      //        if (devices.size() == 0) continue;
      
      std::string label = "Port " + ofToString(port);
      ofRectangle labelBB = ofxCortex::core::utils::getBitmapStringBoundingBox(label);
      int labelPadding = 8;
      
      int rows = devices.size() / columns;
      float rowStart = -(deviceHeight * (rows / 2.0));
      float deviceX = spacingX * 3;
      float deviceY = spacingY + (spacingY * port);
      float connectorStartX = spacingX + controllerWidth * 0.5 + deviceHeight;
      float connectorEndX = deviceX - deviceHeight - labelBB.width - labelPadding;
      
      ofNoFill();
      (devices.size()) ? ofSetColor(255) : ofSetColor(255, 64);
      ofDrawBezier(connectorStartX, bounds.getCenter().y, connectorStartX + (spacingX * 0.5), bounds.getCenter().y, connectorEndX - (spacingX * 0.5), deviceY, connectorEndX - labelPadding, deviceY);
      
      ofSetColor(255, 64);
      ofDrawRectRounded(connectorEndX + labelBB.width * 0.5, deviceY, labelBB.width + labelPadding, labelBB.height + labelPadding, 3);
      
      (devices.size()) ? ofSetColor(255) : ofSetColor(255, 64);
      ofDrawBitmapString(label, connectorEndX, deviceY + (labelBB.height * 0.5) - 1);
      
      ofFill();
      for (int i = 0; i < devices.size(); i++)
      {
        int column = i % columns;
        int row = i / columns;
        
        if (auto LED = dynamic_pointer_cast<ofxCortex::io::hardware::LED>(devices[i]))
        {
          ofSetColor(LED->getDisplayColor());
        }
        else ofSetColor(255);
        
        ofFill();
        ofDrawCircle(deviceX + (deviceWidth * column), deviceY + rowStart + (deviceHeight * row), deviceHeight * 0.3);
        
        ofNoFill();
        ofSetColor(64);
        ofDrawCircle(deviceX + (deviceWidth * column), deviceY + rowStart + (deviceHeight * row), deviceHeight * 0.3 + 1);
        
      }
    }
  }
  ofPopStyle();
}

}}}
