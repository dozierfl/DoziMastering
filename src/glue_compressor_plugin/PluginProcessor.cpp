#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace {
constexpr int parameterVersion=1;
juce::NormalisableRange<float> skewed(float min,float max,float centre){juce::NormalisableRange<float> range(min,max);range.setSkewForCentre(centre);return range;}
}

DoziGlueCompressorProcessor::DoziGlueCompressorProcessor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),parameters(*this,nullptr,"DOZI_GLUE",createParameterLayout()){}

juce::AudioProcessorValueTreeState::ParameterLayout DoziGlueCompressorProcessor::createParameterLayout(){
    using Float=juce::AudioParameterFloat;using Bool=juce::AudioParameterBool;juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<Float>(juce::ParameterID{"threshold",parameterVersion},"Threshold",juce::NormalisableRange<float>{-40,10,.1f},-11.4f,"dBFS"));
    layout.add(std::make_unique<Float>(juce::ParameterID{"attack",parameterVersion},"Attack",skewed(.1f,100,10),80,"ms"));
    layout.add(std::make_unique<Float>(juce::ParameterID{"release",parameterVersion},"Release",skewed(50,1000,300),300,"ms"));
    layout.add(std::make_unique<Float>(juce::ParameterID{"ratio",parameterVersion},"Ratio",skewed(1,20,4),1.1f,":1"));
    layout.add(std::make_unique<Float>(juce::ParameterID{"makeupGainDb",parameterVersion},"Makeup Gain",juce::NormalisableRange<float>{-18,12,.1f},0,"dB"));
    layout.add(std::make_unique<Bool>(juce::ParameterID{"autoMakeup",parameterVersion},"Auto Makeup",false));
    layout.add(std::make_unique<Bool>(juce::ParameterID{"autoRelease",parameterVersion},"Auto Release",false));
    layout.add(std::make_unique<Bool>(juce::ParameterID{"sidechainHpf",parameterVersion},"Sidechain HPF 80 Hz",false));
    layout.add(std::make_unique<Bool>(juce::ParameterID{"moduleEnabled",parameterVersion},"Module Enabled",true));return layout;
}

void DoziGlueCompressorProcessor::prepareToPlay(double sampleRate,int blockSize){engine.prepare(sampleRate,blockSize,getTotalNumInputChannels());}
bool DoziGlueCompressorProcessor::isBusesLayoutSupported(const BusesLayout&layout)const{return layout.getMainInputChannelSet()==layout.getMainOutputChannelSet()&&(layout.getMainOutputChannelSet()==juce::AudioChannelSet::mono()||layout.getMainOutputChannelSet()==juce::AudioChannelSet::stereo());}
void DoziGlueCompressorProcessor::processBlock(juce::AudioBuffer<float>&buffer,juce::MidiBuffer&){juce::ScopedNoDenormals noDenormals;for(auto channel=getTotalNumInputChannels();channel<getTotalNumOutputChannels();++channel)buffer.clear(channel,0,buffer.getNumSamples());GlueCompressorSettings s;const auto value=[this](const char*id){return parameters.getRawParameterValue(id)->load();};s.thresholdDbfs=value("threshold");s.attackMs=value("attack");s.releaseMs=value("release");s.ratio=value("ratio");s.makeupGainDb=value("makeupGainDb");s.autoMakeup=value("autoMakeup")>.5f;s.autoRelease=value("autoRelease")>.5f;s.sidechainHpf=value("sidechainHpf")>.5f;s.moduleEnabled=value("moduleEnabled")>.5f;const auto levels=engine.process(buffer,s);for(std::size_t n=0;n<2;++n){meters[n].store(levels.inputDbfs[n],std::memory_order_relaxed);meters[n+2].store(levels.outputDbfs[n],std::memory_order_relaxed);meters[n+4].store(levels.gainReductionDb[n],std::memory_order_relaxed);}}
juce::AudioProcessorEditor*DoziGlueCompressorProcessor::createEditor(){return new DoziGlueCompressorEditor(*this);}
void DoziGlueCompressorProcessor::getStateInformation(juce::MemoryBlock&destination){if(auto xml=parameters.copyState().createXml())copyXmlToBinary(*xml,destination);}
void DoziGlueCompressorProcessor::setStateInformation(const void*data,int size){if(auto xml=getXmlFromBinary(data,size);xml&&xml->hasTagName(parameters.state.getType()))parameters.replaceState(juce::ValueTree::fromXml(*xml));}
void DoziGlueCompressorProcessor::resetParametersToDefaults(){for(auto*parameter:getParameters())if(auto*ranged=dynamic_cast<juce::RangedAudioParameter*>(parameter);ranged&&ranged->paramID!="moduleEnabled"){ranged->beginChangeGesture();ranged->setValueNotifyingHost(ranged->getDefaultValue());ranged->endChangeGesture();}}
GlueCompressorLevels DoziGlueCompressorProcessor::meterSnapshot()const{GlueCompressorLevels value;for(std::size_t n=0;n<2;++n){value.inputDbfs[n]=meters[n].load(std::memory_order_relaxed);value.outputDbfs[n]=meters[n+2].load(std::memory_order_relaxed);value.gainReductionDb[n]=meters[n+4].load(std::memory_order_relaxed);}return value;}

juce::AudioProcessor*JUCE_CALLTYPE createPluginFilter(){return new DoziGlueCompressorProcessor;}
