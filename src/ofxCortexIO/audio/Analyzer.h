#pragma once

#include "ofEvents.h"
#include "ofxAudioAnalyzer.h"

#include "ofxCortex/utils/ContainerUtils.h"
#include "ofxCortex/types/Select.h"

namespace ofxCortex::io::audio {

/// \class Audio Analyzer
/// \brief Remember to run with Intel/Rosette-destination.
class AudioAnalyzer : public ofBaseSoundInput {
public:
  template<typename ... T>
  static std::shared_ptr<AudioAnalyzer> create(T&& ... t) {
    struct EnableMakeShared : public AudioAnalyzer { EnableMakeShared(T&&... arg) : AudioAnalyzer(std::forward<T>(arg)...) {} };
    return std::make_shared<EnableMakeShared>(std::forward<T>(t)...);
  }
  
  void setup(int deviceID);
  void audioIn(ofSoundBuffer &inBuffer) { analyzer.analyze(inBuffer); }
  
  float getRMS(size_t channel = 0) { return analyzer.getValue(RMS, channel % analyzer.getChannelsNum(), smoothing, normalized); }
  float getPower(size_t channel = 0) { return analyzer.getValue(POWER, channel % analyzer.getChannelsNum(), smoothing, normalized); }
  float getPitchFrequency(size_t channel = 0) { return analyzer.getValue(PITCH_FREQ, channel % analyzer.getChannelsNum(), smoothing, normalized); }
  float getPitchConfidence(size_t channel = 0) { return analyzer.getValue(PITCH_CONFIDENCE, channel % analyzer.getChannelsNum(), smoothing, normalized); }
  float getPitchSalience(size_t channel = 0) { return analyzer.getValue(PITCH_SALIENCE, channel % analyzer.getChannelsNum(), smoothing, normalized); }
  float getInharmonicity(size_t channel = 0) { return analyzer.getValue(INHARMONICITY, channel % analyzer.getChannelsNum(), smoothing, normalized); }
  float getHFC(size_t channel = 0) { return analyzer.getValue(HFC, channel % analyzer.getChannelsNum(), smoothing, normalized); }
  float getSpectralCompleity(size_t channel = 0) { return analyzer.getValue(SPECTRAL_COMPLEXITY, channel % analyzer.getChannelsNum(), smoothing, normalized); }
  float getCentroid(size_t channel = 0) { return analyzer.getValue(CENTROID, channel % analyzer.getChannelsNum(), smoothing, normalized); }
  float getRollOff(size_t channel = 0) { return analyzer.getValue(ROLL_OFF, channel % analyzer.getChannelsNum(), smoothing, normalized); }
  float getOddToEven(size_t channel = 0) { return analyzer.getValue(ODD_TO_EVEN, channel % analyzer.getChannelsNum(), smoothing, normalized); }
  float getStrongPeak(size_t channel = 0) { return analyzer.getValue(STRONG_PEAK, channel % analyzer.getChannelsNum(), smoothing, normalized); }
  float getStrongDecay(size_t channel = 0) { return analyzer.getValue(STRONG_DECAY, channel % analyzer.getChannelsNum(), smoothing, normalized); }
  float getDissonance(size_t channel = 0) { return analyzer.getValue(DISSONANCE, channel % analyzer.getChannelsNum(), smoothing, normalized); }
  
  const std::vector<float> & getSpectrum(size_t channel = 0) { return analyzer.getValues(SPECTRUM, channel % analyzer.getChannelsNum(), smoothing); }
  const std::vector<float> & getMelBands(size_t channel = 0) { return analyzer.getValues(MEL_BANDS, channel % analyzer.getChannelsNum(), smoothing); }
  const std::vector<float> & getMFCC(size_t channel = 0) { return analyzer.getValues(MFCC, channel % analyzer.getChannelsNum(), smoothing); }
  const std::vector<float> & getHPCP(size_t channel = 0) { return analyzer.getValues(HPCP, channel % analyzer.getChannelsNum(), smoothing); }
  const std::vector<float> & getTristimulus(size_t channel = 0) { return analyzer.getValues(TRISTIMULUS, channel % analyzer.getChannelsNum(), smoothing); }
  
  bool isOnset(size_t channel = 0) { return analyzer.getOnsetValue(channel); }
  
  operator ofParameterGroup&() { return parameters; }
  
protected:
  AudioAnalyzer();
  ~AudioAnalyzer() {}
  
  ofSoundStream soundStream;
  ofxAudioAnalyzer analyzer;
  
  ofParameter<ofxCortex::core::types::Select<int>> deviceDropdown { "Device", ofxCortex::core::types::Select<int>() };
  ofEventListener onDeviceSelectedE;
  ofParameter<float> smoothing { "Smoothing", 0.5, 0.0, 1.0 };
  ofParameter<bool> normalized { "Normalized", true };
  ofParameterGroup parameters {
    "Audio Analyzer",
    deviceDropdown,
    smoothing,
    normalized
  };
  
};

}
