#include "Analyzer.h"

namespace ofxCortex::io::audio {

AudioAnalyzer::AudioAnalyzer()
{
  auto inputDevices = ofxCortex::core::utils::Array::filter(soundStream.getDeviceList(), [](auto & device){ return device.inputChannels > 0; });
  
  auto dropdownDevices = ofxCortex::core::types::Select<int>();
  for (auto & device : inputDevices) { dropdownDevices.add(device.deviceID, device.name); }
  dropdownDevices.setSelectedValue(2);
  
  onDeviceSelectedE = deviceDropdown.newListener([&, this](ofxCortex::core::types::Select<int> & param) { this->setup(param.getSelected()); });
  deviceDropdown.set(dropdownDevices);
}

void AudioAnalyzer::setup(int deviceID)
{
  ofLogVerbose("AudioAnalyzer::setup(" + ofToString(deviceID) + ")");
  
  auto devices = soundStream.getDeviceList();
  auto it = find_if(devices.begin(), devices.end(), [&deviceID](const ofSoundDevice& device) { return device.deviceID == deviceID; });
  
  if (it == devices.end()) return;
  auto device = *it; //soundStream.getDeviceList()[deviceID];
  
  ofSoundStreamSettings settings;
  settings.setInListener(this);
  settings.setInDevice(device);
  settings.sampleRate = 44100;
  settings.bufferSize = 512;
  settings.numInputChannels = device.inputChannels;
  settings.numOutputChannels = device.outputChannels;
  
  soundStream.setup(settings);
  analyzer.reset(settings.sampleRate, settings.bufferSize, settings.numInputChannels);
}

}
