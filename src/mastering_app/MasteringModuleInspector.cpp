#include "MasteringModuleInspector.h"

#include <algorithm>

namespace {
using Type=dozi::core::MasteringModuleType;
juce::String moduleName(Type type){switch(type){case Type::linearPhaseEq:return "Corrective EQ";case Type::dynamicEq:return "Dynamic EQ";case Type::deEsser:return "De-Esser";case Type::broadbandCompressor:return "Glue Compressor";case Type::multibandCompressor:return "Multiband Compressor";case Type::saturation:return "Saturation";case Type::stereoWidthMonoBass:return "Width / Mono Bass";case Type::truePeakLimiter:return "True Peak Limiter";}return "Module";}
struct Spec{double minimum,maximum,interval;juce::String suffix;};
Spec specFor(const std::string&key,double value){
    if(key.find("frequencyHz")!=std::string::npos||key.find("CrossoverHz")!=std::string::npos)return {20,20000,1," Hz"};
    if(key.find("threshold")!=std::string::npos)return {-60,0,.1," dBFS"};
    if(key.find("ceiling")!=std::string::npos)return {-6,0,.1," dBTP"};
    if(key.find("maximumCutDb")!=std::string::npos||key.find("maximumReductionDb")!=std::string::npos||key.find("maximumCompensationDb")!=std::string::npos)return {0,12,.1," dB"};
    if(key=="inputGainDb")return {-24,36,.1," dB"};
    if(key.find("GainDb")!=std::string::npos||key.find("gainDb")!=std::string::npos||key.find("TrimDb")!=std::string::npos)return {-12,12,.1," dB"};
    if(key.find("driveDb")!=std::string::npos)return {0,12,.1," dB"};
    if(key.find("attackMs")!=std::string::npos||key.find("releaseMs")!=std::string::npos||key.find("lookaheadMs")!=std::string::npos)return {.1,1000,.1," ms"};
    if(key.find("ratio")!=std::string::npos)return {1,20,.1," : 1"};
    if(key.find("width")!=std::string::npos||key=="mix")return {0,2,.01,{}};
    if(key=="q")return {.1,20,.1,{}};
    if(key=="mode"||key=="automaticLevelCompensation")return {0,1,1,{}};
    const auto span=std::max(10.0,std::abs(value)*2);return {-span,span,.01,{}};
}
juce::String displayName(std::string key){for(auto&c:key)if(c=='.')c=' ';juce::String text(key);return text.replace("frequencyHz","Frequency").replace("thresholdDbfs","Threshold").replace("inputGainDb","Input Gain").replace("ceilingDbtp","Ceiling").replace("lookaheadMs","Lookahead").replace("releaseMs","Release").replace("attackMs","Attack").replace("gainDb","Gain").replace("maximum","Maximum ").replace("monoBassCrossoverHz","Mono Bass Crossover").replace("automaticLevelCompensation","Automatic Level Compensation (0 Off, 1 On)").replace("mode","Mode (0 Tape, 1 Tube)");}
}

MasteringModuleInspector::MasteringModuleInspector(){addAndMakeVisible(moduleSelector);addAndMakeVisible(enabledButton);addAndMakeVisible(viewport);viewport.setViewedComponent(&parameterContent,false);moduleSelector.onChange=[this]{rebuildParameters();};enabledButton.onClick=[this]{if(!plan)return;const auto index=moduleSelector.getSelectedItemIndex();if(index<0||index>=static_cast<int>(plan->modules.size()))return;plan->modules[static_cast<std::size_t>(index)].bypassed=!enabledButton.getToggleState();if(onPlanChanged)onPlanChanged();};}
void MasteringModuleInspector::setPlan(dozi::core::MasteringPlan*value){plan=value;rebuildModules();}
void MasteringModuleInspector::rebuildModules(){const auto previous=moduleSelector.getSelectedItemIndex();moduleSelector.clear(juce::dontSendNotification);if(plan)for(std::size_t index=0;index<plan->modules.size();++index)moduleSelector.addItem(moduleName(plan->modules[index].type),static_cast<int>(index)+1);if(plan&&!plan->modules.empty())moduleSelector.setSelectedItemIndex(juce::jlimit(0,static_cast<int>(plan->modules.size())-1,std::max(0,previous)),juce::dontSendNotification);rebuildParameters();}
void MasteringModuleInspector::rebuildParameters(){controls.clear();parameterContent.removeAllChildren();if(!plan){enabledButton.setEnabled(false);return;}const auto index=moduleSelector.getSelectedItemIndex();if(index<0||index>=static_cast<int>(plan->modules.size()))return;auto&module=plan->modules[static_cast<std::size_t>(index)];enabledButton.setEnabled(true);enabledButton.setToggleState(!module.bypassed,juce::dontSendNotification);for(const auto&[key,value]:module.parameters){ParameterControl control;control.key=key;control.label=std::make_unique<juce::Label>();control.slider=std::make_unique<juce::Slider>();control.label->setText(displayName(key),juce::dontSendNotification);const auto spec=specFor(key,value);control.slider->setRange(spec.minimum,spec.maximum,spec.interval);control.slider->setValue(value,juce::dontSendNotification);control.slider->setTextValueSuffix(spec.suffix);control.slider->setSliderStyle(juce::Slider::LinearHorizontal);control.slider->setTextBoxStyle(juce::Slider::TextBoxRight,false,100,24);auto*slider=control.slider.get();slider->onValueChange=[this,key,slider]{if(!plan)return;const auto selected=moduleSelector.getSelectedItemIndex();if(selected<0||selected>=static_cast<int>(plan->modules.size()))return;plan->modules[static_cast<std::size_t>(selected)].parameters[key]=slider->getValue();if(onPlanChanged)onPlanChanged();};parameterContent.addAndMakeVisible(*control.label);parameterContent.addAndMakeVisible(*control.slider);controls.push_back(std::move(control));}resized();}
void MasteringModuleInspector::resized(){auto area=getLocalBounds();moduleSelector.setBounds(area.removeFromTop(30));enabledButton.setBounds(area.removeFromTop(30));viewport.setBounds(area);parameterContent.setSize(juce::jmax(100,area.getWidth()-12),static_cast<int>(controls.size())*52);int y=0;for(auto&control:controls){control.label->setBounds(4,y,parameterContent.getWidth()-8,18);control.slider->setBounds(4,y+18,parameterContent.getWidth()-8,30);y+=52;}}
