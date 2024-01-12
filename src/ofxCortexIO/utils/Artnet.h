#pragma once

#include "ofConstants.h"
#include "ofUtils.h"
#include "ofMath.h"
#include "ofVectorMath.h"
#include "ofColor.h"
#include "ofLog.h"
#include "ofxArtnet.h"
#include "ofxCortexIO/hardware/Devices.h"


namespace ofxCortex { namespace io { namespace utils {

inline static void sendPackets(ofxArtnetSender & artnet, const std::vector<ofxArtnetMessage> & packets)
{
  ofLogVerbose("ofxCortex::io::utils::sendPackets()") << "Sending data to IP '" << artnet.getIP() << "'";
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

template<typename DeviceType>
inline static std::vector<ofxArtnetMessage> devicesToArtnet(const std::vector<std::shared_ptr<DeviceType>> & devices, int universeOffset = 0)
{
  static_assert(std::is_base_of<ofxCortex::io::hardware::ArtnetDevice, DeviceType>::value, "ofxCortex::io::utils::devicesToArtnet(): The Device needs to be of base type ArtnetDevice");
  
  const size_t dataPrUniverse = 512;
  
  unsigned int bytes = 0;
  unsigned int currentDataIndex = 0;
  uint16_t currentUniverse = 0;
  
  std::map<uint16_t, std::vector<uint8_t>> universeData;
  for (int i = 0; i < devices.size(); i++)
  {
    const auto & device = devices[i];
    const unsigned int channels = device->getChannelCount();
    
    if (currentDataIndex + channels > dataPrUniverse) {
      currentUniverse++;
      currentDataIndex = 0;
    }
    const std::vector<uint8_t> & data = device->getData();

    for (int ch = 0; ch < channels; ch++)
    {
      universeData[currentUniverse].push_back(data[ch]);
      currentDataIndex++;
      bytes++;
    }
  }

  std::vector<ofxArtnetMessage> messages;
  for (auto universe : universeData)
  {
    ofxArtnetMessage msg;
    msg.setUniverse(0, 0, universe.first + universeOffset);
    msg.setData(universe.second);
    messages.push_back(msg);
  }

  return messages;
}

inline static std::vector<ofxArtnetMessage> devicesToArtnet(const std::vector<std::shared_ptr<ofxCortex::io::hardware::ArtnetDevice>> & devices, int universeOffset = 0)
{
//  return devicesToArtnet(devices, universeOffset);
  
  const size_t dataPrUniverse = 512;
  
  unsigned int bytes = 0;
  unsigned int currentDataIndex = 0;
  uint16_t currentUniverse = 0;
  
  std::map<uint16_t, std::vector<uint8_t>> universeData;
  for (int i = 0; i < devices.size(); i++)
  {
    const auto & device = devices[i];
    const unsigned int channels = device->getChannelCount();
    
    if (currentDataIndex + channels > dataPrUniverse) {
      currentUniverse++;
      currentDataIndex = 0;
    }
    const std::vector<uint8_t> & data = device->getData();

    for (int ch = 0; ch < channels; ch++)
    {
      universeData[currentUniverse].push_back(data[ch]);
      currentDataIndex++;
      bytes++;
    }
  }

  std::vector<ofxArtnetMessage> messages;
  for (auto universe : universeData)
  {
    ofxArtnetMessage msg;
    msg.setUniverse(0, 0, universe.first + universeOffset);
    msg.setData(universe.second);
    messages.push_back(msg);
  }

  return messages;
}

inline static void sendColorsOverArtnet(ofxArtnetSender & artnet, const std::vector<ofColor> & colors, int format = GL_RGB, int universeOffset = 0)
{
  ofLogVerbose("ofxCortex::io::utils::sendColorsOverArtnet()") << "IP = " << artnet.getIP() << " | Colors = " << colors.size();
  sendPackets(artnet, colorsToArtnet(colors, format, universeOffset));
}

}}}
