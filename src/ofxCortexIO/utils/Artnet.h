#pragma once

#include "ofConstants.h"
#include "ofUtils.h"
#include "ofMath.h"
#include "ofVectorMath.h"
#include "ofColor.h"
#include "ofLog.h"
#include "ofxArtnet.h"
#include "ofxCortexIO/hardware/Devices.h"


namespace ofxCortex::io::utils {

struct DeviceData {
  uint16_t universe;
  uint8_t indexOffset;
  std::vector<uint8_t> data;
};

inline static void sendPackets(ofxArtnetSender & artnet, const std::vector<ofxArtnetMessage> & packets)
{
  for (const auto & packet : packets) artnet.sendArtnet(packet);
}

inline static std::vector<ofxArtnetMessage> colorsToArtnet(const vector<ofColor> & colors, int glFormat = GL_RGB, int universeOffset = 0)
{
  int channelsPrColor = ofGetNumChannelsFromGLFormat(glFormat);
  size_t totalBytes = colors.size() * channelsPrColor;
  int universes = ceil(totalBytes / 512.0);
  int colorsPrUniverse = floor(512.0 / channelsPrColor);

  ofLogVerbose("ofxCortex::io::utils::colorsToArtnet()") << "Colors = " << colors.size() << " | Channels pr. color = " << channelsPrColor << " | Total Bytes = " << totalBytes << " | Universes needed = " << universes << " | Colors pr universe = " << colorsPrUniverse;

  std::map<int, std::vector<uint8_t>> universeData;
  for (int i = 0; i < colors.size(); i++)
  {
    const ofColor & c = colors[i];
    uint16_t currentUniverse = floor(i / colorsPrUniverse);

    for (int ch = 0; ch < channelsPrColor; ch++) universeData[currentUniverse].push_back(c[ch]);
  }

  std::vector<ofxArtnetMessage> messages;
  for (auto universe : universeData)
  {
    ofxArtnetMessage msg;
    msg.setUniverse(0, channelsPrColor, universe.first + universeOffset);
    msg.setData(universe.second);
    messages.push_back(msg);
  }

  return messages;
}

inline static std::vector<ofxArtnetMessage> devicesToArtnet(const std::vector<std::shared_ptr<ofxCortex::io::hardware::ArtnetDevice>> & devices, int universeOffset = 0)
{
  unsigned int bytes = 0;
  unsigned int currentDataIndex = 0;
  uint16_t currentUniverse = universeOffset;
  
  std::map<uint16_t, DeviceData> universeData;
  unsigned int indexOffset = 0;
  for (int i = 0; i < devices.size(); i++)
  {
    const auto & device = devices[i];
    const std::vector<uint8_t> & data = device->getData();
    
    DeviceData deviceData;
    deviceData.universe = currentUniverse;
    deviceData.indexOffset = indexOffset;
    
//    ofLogNotice("ArtnetUtils::devicesToArtnet()") << "Device Index = " << i << " | Universe = " << currentUniverse << " | Universe Offset = " << universeOffset << " | Index Offset = " << indexOffset << " | Data Size = " << data.size() << " (" << data.size() / device->getChannelCount() << " units)";

    for (int ch = 0; ch < data.size(); ch++)
    {
      if (currentDataIndex >= 512 - 1) {
        universeData[currentUniverse] = deviceData; // Push current data
        
        currentUniverse++; // Increment to next universe
        currentDataIndex = 0; // Reset current data entry index
        indexOffset += 512 / device->getChannelCount();
        
        deviceData.universe = currentUniverse;
        deviceData.indexOffset = indexOffset;
        deviceData.data.clear();
        
//        ofLogNotice("ArtnetUtils::devicesToArtnet()") << "Universe packed (" << bytes << " bytes)! Skipping to next universe (" << currentUniverse << ")";
      }
      
      deviceData.data.push_back(data[ch]);
      
      currentDataIndex++;
      bytes++;
    }
    
    currentUniverse++;
    currentDataIndex = 0;
    indexOffset += data.size() / device->getChannelCount();
    
    universeData[currentUniverse] = deviceData;
  }

  std::vector<ofxArtnetMessage> messages;
  for (auto [universe, deviceData] : universeData)
  {
    ofxArtnetMessage msg;
    msg.setUniverse(0, deviceData.indexOffset, deviceData.universe);
    msg.setData(deviceData.data);
    messages.push_back(msg);
  }

  return messages;
}

inline static void sendColorsOverArtnet(ofxArtnetSender & artnet, const std::vector<ofColor> & colors, int format = GL_RGB, int universeOffset = 0)
{
  ofLogVerbose("ofxCortex::io::utils::sendColorsOverArtnet()") << "IP = " << artnet.getIP() << " | Colors = " << colors.size();
  sendPackets(artnet, colorsToArtnet(colors, format, universeOffset));
}

}
