#include "MasteringHardwareControls.h"
#include "BinaryData.h"

#include <algorithm>
#include <cmath>

HardwareKnobLookAndFeel::HardwareKnobLookAndFeel(){setColour(juce::Slider::textBoxTextColourId,juce::Colour(0xff38d8be));setColour(juce::Slider::textBoxBackgroundColourId,juce::Colour(0xff080b10));setColour(juce::Slider::textBoxOutlineColourId,juce::Colour(0xff2b3a49));}
void HardwareKnobLookAndFeel::drawButtonText(juce::Graphics&,juce::TextButton&,bool,bool){}
void HardwareKnobLookAndFeel::drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool){}
juce::Font HardwareKnobLookAndFeel::getLabelFont(juce::Label&){return juce::Font(juce::FontOptions(20.0f));}
void HardwareKnobLookAndFeel::drawRotarySlider(juce::Graphics&g,int x,int y,int width,int height,float position,float start,float end,juce::Slider&){const auto diameter=static_cast<float>(std::min(width,height));const auto area=juce::Rectangle<float>(static_cast<float>(x)+(width-diameter)*0.5f,static_cast<float>(y)+(height-diameter)*0.5f,diameter,diameter);const auto centre=area.getCentre();const auto angle=start+position*(end-start);const auto radius=area.getWidth()*0.31f;const auto endPoint=centre+juce::Point<float>(std::sin(angle),-std::cos(angle))*radius;g.setColour(juce::Colours::black.withAlpha(0.7f));g.drawLine(centre.x+1,centre.y+2,endPoint.x+1,endPoint.y+2,4.0f);g.setColour(juce::Colour(0xfff2eee8));g.drawLine(centre.x,centre.y,endPoint.x,endPoint.y,3.0f);g.setColour(juce::Colour(0xff38e3d0));g.fillEllipse(endPoint.x-3,endPoint.y-3,6,6);}

CorrectiveEqHardwarePanel::CorrectiveEqHardwarePanel(){faceplateSkin=juce::ImageFileFormat::loadFrom(BinaryData::correctiveeqskin_png,BinaryData::correctiveeqskin_pngSize);for(auto*component:std::array<juce::Component*,8>{&inButton,&autoButton,&resetButton,&bandInButton,&resetBandButton,&title,&status,&selectedBandLabel})addAndMakeVisible(component);for(auto*button:{&inButton,&autoButton,&resetButton,&bandInButton,&resetBandButton})button->setLookAndFeel(&knobLookAndFeel);title.setText("CORRECTIVE EQ",juce::dontSendNotification);title.setFont(juce::FontOptions(34.0f).withStyle("Bold"));title.setJustificationType(juce::Justification::centred);status.setJustificationType(juce::Justification::centredLeft);status.setFont(juce::FontOptions(22.0f).withStyle("Bold"));status.setColour(juce::Label::textColourId,juce::Colour(0xffe6a85c));selectedBandLabel.setColour(juce::Label::textColourId,juce::Colour(0xffd9c6b2));selectedBandLabel.setFont(juce::FontOptions(22.0f).withStyle("Bold"));inButton.setClickingTogglesState(true);inButton.onClick=[this]{updateBypass();};autoButton.setTooltip("Reapply the analysis recommendation");autoButton.onClick=[this]{restoreAnalysisSettings();};resetButton.onClick=[this]{if(!module)return;for(auto&[key,value]:module->parameters)if(key.ends_with("gainDb"))value=0.0;refresh();if(onModuleChanged)onModuleChanged();};bandInButton.setClickingTogglesState(true);bandInButton.onClick=[this]{if(!module||bands[selectedBand].gainKey.empty())return;auto&band=bands[selectedBand];if(bandInButton.getToggleState()){const auto saved=analysisSettings.contains(band.gainKey)?analysisSettings.at(band.gainKey):-1.0;module->parameters[band.gainKey]=std::min(-0.01,saved);}else module->parameters[band.gainKey]=0.0;refresh();if(onModuleChanged)onModuleChanged();};resetBandButton.onClick=[this]{if(!module)return;auto&band=bands[selectedBand];for(const auto*key:{&band.gainKey,&band.frequencyKey,&band.qKey})if(!key->empty()&&analysisSettings.contains(*key))module->parameters[*key]=analysisSettings.at(*key);refresh();if(onModuleChanged)onModuleChanged();};static constexpr std::array detailNames{"FREQUENCY","GAIN","Q"};for(std::size_t index=0;index<detailControls.size();++index){addAndMakeVisible(detailLabels[index]);addAndMakeVisible(detailControls[index]);detailLabels[index].setText(detailNames[index],juce::dontSendNotification);detailLabels[index].setJustificationType(juce::Justification::centred);detailLabels[index].setFont(juce::FontOptions(20.0f).withStyle("Bold"));detailControls[index].setLookAndFeel(&knobLookAndFeel);detailControls[index].setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);detailControls[index].setTextBoxStyle(juce::Slider::TextBoxBelow,false,104,23);}detailControls[0].setRange(20.0,20000.0,1.0);detailControls[1].setRange(-6.0,0.0,0.01);detailControls[2].setRange(0.1,20.0,0.01);detailControls[0].onValueChange=[this]{if(updatingControls||!module)return;auto&band=bands[selectedBand];if(!band.frequencyKey.empty()){module->parameters[band.frequencyKey]=detailControls[0].getValue();refresh();if(onModuleChanged)onModuleChanged();}};detailControls[1].onValueChange=[this]{if(updatingControls||!module)return;auto&band=bands[selectedBand];if(!band.gainKey.empty()){module->parameters[band.gainKey]=detailControls[1].getValue();refresh();if(onModuleChanged)onModuleChanged();}};detailControls[2].onValueChange=[this]{if(updatingControls||!module)return;auto&band=bands[selectedBand];if(!band.qKey.empty()){module->parameters[band.qKey]=detailControls[2].getValue();refresh();if(onModuleChanged)onModuleChanged();}};for(std::size_t index=0;index<bands.size();++index){auto&band=bands[index];addAndMakeVisible(band.gain);addAndMakeVisible(band.name);addAndMakeVisible(band.frequency);band.gain.setLookAndFeel(&knobLookAndFeel);band.gain.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);band.gain.setTextBoxStyle(juce::Slider::TextBoxBelow,false,88,24);band.gain.setRange(-6.0,0.0,0.01);band.gain.setDoubleClickReturnValue(true,0.0);band.name.setJustificationType(juce::Justification::centred);band.name.setFont(juce::FontOptions(22.0f).withStyle("Bold"));band.frequency.setJustificationType(juce::Justification::centred);band.frequency.setColour(juce::Label::textColourId,juce::Colour(0xff9ba8b5));auto*slider=&band.gain;band.gain.onDragStart=[this,index]{selectBand(index);};band.gain.onValueChange=[this,index,slider]{auto&current=bands[index];if(updatingControls||!module||current.gainKey.empty())return;selectedBand=index;module->parameters[current.gainKey]=slider->getValue();refreshSelectedBand();if(onModuleChanged)onModuleChanged();repaint();};}}
CorrectiveEqHardwarePanel::~CorrectiveEqHardwarePanel(){for(auto&band:bands)band.gain.setLookAndFeel(nullptr);for(auto&control:detailControls)control.setLookAndFeel(nullptr);for(auto*button:{&inButton,&autoButton,&resetButton,&bandInButton,&resetBandButton})button->setLookAndFeel(nullptr);}
void CorrectiveEqHardwarePanel::setModule(dozi::core::MasteringModulePlan*value){inButton.setComponentID("in");autoButton.setComponentID("auto");resetButton.setComponentID("reset");bandInButton.setComponentID("bandIn");resetBandButton.setComponentID("resetBand");for(std::size_t index=0;index<detailControls.size();++index)detailControls[index].setComponentID("detail"+juce::String(static_cast<int>(index)));for(std::size_t index=0;index<bands.size();++index)bands[index].gain.setComponentID("bandGain"+juce::String(static_cast<int>(index)));module=value;if(module&&module!=lastModule){analysisSettings=module->recommendedParameters.empty()?module->parameters:module->recommendedParameters;styleSettings=module->styleParameters.empty()?module->parameters:module->styleParameters;lastModule=module;}resetButton.onClick=[this]{if(!module)return;module->parameters=styleSettings;refresh();if(onModuleChanged)onModuleChanged();};resetBandButton.onClick=[this]{if(!module)return;auto&band=bands[selectedBand];for(const auto*key:{&band.gainKey,&band.frequencyKey,&band.qKey})if(!key->empty()&&styleSettings.contains(*key))module->parameters[*key]=styleSettings.at(*key);refresh();if(onModuleChanged)onModuleChanged();};if(!module)lastModule=nullptr;refresh();}
void CorrectiveEqHardwarePanel::refreshFromModule(){refresh();}
void CorrectiveEqHardwarePanel::refresh(){static constexpr std::array names{"LOW","LOW MID","HIGH MID","AIR"};updatingControls=true;for(std::size_t index=0;index<bands.size();++index){auto&band=bands[index];band.gainKey.clear();band.frequencyKey.clear();band.qKey.clear();band.name.setText(names[index],juce::dontSendNotification);band.frequency.setText("No cut",juce::dontSendNotification);band.gain.setValue(0.0,juce::dontSendNotification);band.gain.setEnabled(false);}if(!module){inButton.setEnabled(false);autoButton.setEnabled(false);resetButton.setEnabled(false);status.setText("ANALYZE A MIX",juce::dontSendNotification);updatingControls=false;refreshSelectedBand();resized();repaint();return;}inButton.setEnabled(true);autoButton.setEnabled(!analysisSettings.empty());resetButton.setEnabled(true);inButton.setToggleState(!module->bypassed,juce::dontSendNotification);struct Candidate{std::string gainKey,frequencyKey,qKey;double frequency;};std::vector<Candidate>candidates;for(const auto&[key,value]:module->parameters)if(key.size()>=6&&key.ends_with("gainDb")){const auto prefix=key.substr(0,key.size()-6);const auto frequencyKey=prefix+"frequencyHz";candidates.push_back({key,frequencyKey,prefix+"q",module->parameters.contains(frequencyKey)?module->parameters.at(frequencyKey):1000.0});}std::ranges::sort(candidates,{},&Candidate::frequency);std::array<bool,4>occupied{};for(const auto&candidate:candidates){std::size_t preferred=candidate.frequency<160.0?0:candidate.frequency<1000.0?1:candidate.frequency<6000.0?2:3;if(occupied[preferred]){auto empty=std::ranges::find(occupied,false);if(empty==occupied.end())continue;preferred=static_cast<std::size_t>(std::distance(occupied.begin(),empty));}occupied[preferred]=true;auto&band=bands[preferred];band.gainKey=candidate.gainKey;band.frequencyKey=candidate.frequencyKey;band.qKey=candidate.qKey;band.gain.setEnabled(true);band.gain.setValue(module->parameters.at(candidate.gainKey),juce::dontSendNotification);band.frequency.setText(candidate.frequency>=1000?juce::String(candidate.frequency/1000.0,2)+" kHz":juce::String(candidate.frequency,0)+" Hz",juce::dontSendNotification);}const auto activeCount=static_cast<int>(std::ranges::count(occupied,true));status.setText(activeCount==0?"NO CUTS ACTIVE":juce::String(activeCount)+" CUTS ACTIVE",juce::dontSendNotification);if(bands[selectedBand].gainKey.empty()){const auto found=std::ranges::find_if(bands,[](const auto&band){return !band.gainKey.empty();});if(found!=bands.end())selectedBand=static_cast<std::size_t>(std::distance(bands.begin(),found));}updatingControls=false;refreshSelectedBand();resized();repaint();}
void CorrectiveEqHardwarePanel::selectBand(std::size_t index){if(index>=bands.size()||bands[index].gainKey.empty())return;selectedBand=index;refreshSelectedBand();repaint();}
void CorrectiveEqHardwarePanel::refreshSelectedBand(){static constexpr std::array names{"LOW","LOW MID","HIGH MID","AIR"};const auto available=module&&!bands[selectedBand].gainKey.empty();selectedBandLabel.setText(names[selectedBand]+juce::String(" - SELECTED BAND"),juce::dontSendNotification);bandInButton.setEnabled(available);resetBandButton.setEnabled(available);for(auto&control:detailControls)control.setEnabled(available);if(!available)return;const auto&band=bands[selectedBand];updatingControls=true;const auto frequency=module->parameters.contains(band.frequencyKey)?module->parameters.at(band.frequencyKey):1000.0;const auto gain=module->parameters.at(band.gainKey);const auto q=module->parameters.contains(band.qKey)?module->parameters.at(band.qKey):1.0;detailControls[0].setValue(frequency,juce::dontSendNotification);detailControls[0].setTextValueSuffix(frequency>=1000.0?" Hz":" Hz");detailControls[1].setValue(gain,juce::dontSendNotification);detailControls[1].setTextValueSuffix(" dB");detailControls[2].setValue(q,juce::dontSendNotification);bandInButton.setToggleState(gain<-0.005,juce::dontSendNotification);updatingControls=false;}
void CorrectiveEqHardwarePanel::updateBypass(){if(!module)return;module->bypassed=!inButton.getToggleState();if(onModuleChanged)onModuleChanged();repaint();}
void CorrectiveEqHardwarePanel::restoreAnalysisSettings(){if(!module||analysisSettings.empty())return;module->parameters=analysisSettings;refresh();if(onModuleChanged)onModuleChanged();}
void CorrectiveEqHardwarePanel::paint(juce::Graphics&g){
    const auto bounds=getLocalBounds().toFloat();
    if(faceplateSkin.isValid())g.drawImage(faceplateSkin,bounds);else g.fillAll(juce::Colour(0xff111820));
    const auto processorEnabled=module&&!module->bypassed;
    if(!processorEnabled){
        g.setColour(juce::Colour(0xff071016).withAlpha(0.94f));g.fillEllipse(143,243,54,54);
        g.setColour(juce::Colour(0xff28323a));g.drawEllipse(143,243,54,54,3.0f);
        g.setColour(juce::Colour(0xff111820).withAlpha(0.94f));g.fillEllipse(1022,112,18,18);
    }else{
        g.setColour(juce::Colour(0xff42f2e5).withAlpha(0.22f));g.drawEllipse(140,240,60,60,5.0f);
    }
    if(bandInButton.getToggleState()){
        g.setColour(juce::Colour(0xff42e2d2));g.fillRoundedRectangle(876.0f,547.0f,102.0f,3.0f,1.5f);
    }
    const juce::Rectangle<float> plot(315.0f,86.0f,568.0f,148.0f);
    juce::Path response;std::array<juce::Point<float>,4>nodes{};std::size_t nodeCount=0;
    for(int px=0;px<juce::roundToInt(plot.getWidth());++px){
        const auto proportion=static_cast<double>(px)/std::max(1.0,static_cast<double>(plot.getWidth()));
        const auto frequency=20.0*std::pow(1000.0,proportion);double gain=0.0;
        if(module)for(const auto&band:bands)if(!band.gainKey.empty()){
            const auto centre=module->parameters.contains(band.frequencyKey)?module->parameters.at(band.frequencyKey):1000.0;
            const auto q=module->parameters.contains(band.qKey)?module->parameters.at(band.qKey):1.0;
            const auto distance=std::log2(frequency/std::max(20.0,centre));
            gain+=module->parameters.at(band.gainKey)*std::exp(-distance*distance*juce::jlimit(0.5,6.0,q*1.8));
        }
        const auto point=juce::Point<float>(plot.getX()+static_cast<float>(px),juce::jmap(static_cast<float>(gain),-18.0f,12.0f,plot.getBottom(),plot.getY()));
        if(px==0)response.startNewSubPath(point);else response.lineTo(point);
    }
    const auto enabled=processorEnabled;
    g.setColour(juce::Colour(0xff3ee4d3).withAlpha(enabled?0.95f:0.30f));
    g.strokePath(response,juce::PathStrokeType(2.2f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
    if(module)for(const auto&band:bands)if(!band.gainKey.empty()&&nodeCount<nodes.size()){
        const auto frequency=module->parameters.contains(band.frequencyKey)?module->parameters.at(band.frequencyKey):1000.0;
        const auto gain=module->parameters.at(band.gainKey);
        const auto proportion=std::log(frequency/20.0)/std::log(1000.0);
        nodes[nodeCount++]={plot.getX()+static_cast<float>(proportion)*plot.getWidth(),juce::jmap(static_cast<float>(gain),-18.0f,12.0f,plot.getBottom(),plot.getY())};
    }
    for(std::size_t index=0;index<nodeCount;++index){
        const auto selected=!bands[selectedBand].gainKey.empty()&&index==selectedBand;
        if(selected){g.setColour(juce::Colour(0xff43e9dc).withAlpha(0.18f));g.fillEllipse(nodes[index].x-14,nodes[index].y-14,28,28);}
        g.setColour(juce::Colour(0xff07151a));g.fillEllipse(nodes[index].x-6,nodes[index].y-6,12,12);
        g.setColour(juce::Colour(0xff3ee4d3).withAlpha(enabled?1.0f:0.35f));g.drawEllipse(nodes[index].x-6,nodes[index].y-6,12,12,2.0f);
    }
    g.setColour(juce::Colour(0xff111820));g.fillRect(112,494,230,36);
}
void CorrectiveEqHardwarePanel::resized(){
    title.setVisible(false);
    status.setBounds(958,128,150,36);
    inButton.setBounds(118,220,104,104);
    autoButton.setBounds(972,178,108,62);
    resetButton.setBounds(985,254,92,48);
    static constexpr std::array<int,4> mainCentres{330,509,689,867};
    for(std::size_t index=0;index<bands.size();++index){
        auto&band=bands[index];band.name.setVisible(false);band.frequency.setVisible(false);
        band.gain.setBounds(mainCentres[index]-62,292,124,139);
    }
    selectedBandLabel.setVisible(true);selectedBandLabel.setBounds(112,494,230,36);
    static constexpr std::array<int,3> detailCentres{442,615,772};
    for(std::size_t index=0;index<detailControls.size();++index){
        detailLabels[index].setVisible(false);detailControls[index].setVisible(true);
        detailControls[index].setBounds(detailCentres[index]-52,482,104,92);
    }
    bandInButton.setVisible(true);resetBandButton.setVisible(true);
    bandInButton.setBounds(872,496,110,54);resetBandButton.setBounds(1000,496,112,54);
}

SkinnedProcessorHardwarePanel::SkinnedProcessorHardwarePanel(){
    inButton.setComponentID("in");autoButton.setComponentID("auto");resetButton.setComponentID("reset");
    globalListenButton.setComponentID("globalListen");hqButton.setComponentID("hq");
    for(std::size_t index=0;index<characterButtons.size();++index)characterButtons[index].setComponentID("character"+juce::String(static_cast<int>(index)));
    for(auto*button:{&inButton,&autoButton,&resetButton}){addAndMakeVisible(button);button->setLookAndFeel(&lookAndFeel);}
    for(auto*button:{&globalListenButton,&hqButton,&characterButtons[0],&characterButtons[1],&characterButtons[2]}){addAndMakeVisible(button);button->setLookAndFeel(&lookAndFeel);button->setClickingTogglesState(true);button->setVisible(false);}
    inButton.setClickingTogglesState(true);
    inButton.onClick=[this]{if(!module)return;module->bypassed=!inButton.getToggleState();repaint();if(onModuleChanged)onModuleChanged();};
    autoButton.onClick=[this]{applyAnalysisSettings();};resetButton.onClick=[this]{applyStyleSettings();};
    globalListenButton.onClick=[this]{if(!module)return;const auto state=globalListenButton.getToggleState()?1.0:0.0;for(auto&[key,value]:module->parameters)if(key.ends_with(".listen"))value=state;repaint();if(onModuleChanged)onModuleChanged();};
    hqButton.onClick=[this]{if(!module)return;module->parameters["oversamplingFactor"]=hqButton.getToggleState()?8.0:2.0;rebuild();if(onModuleChanged)onModuleChanged();};
    for(std::size_t index=0;index<characterButtons.size();++index)characterButtons[index].onClick=[this,index]{if(!module)return;module->parameters["character"]=static_cast<double>(index);rebuild();if(onModuleChanged)onModuleChanged();};
}
SkinnedProcessorHardwarePanel::~SkinnedProcessorHardwarePanel(){for(auto&control:controls)control.slider->setLookAndFeel(nullptr);for(auto&item:switches)item.button->setLookAndFeel(nullptr);for(auto*button:{&inButton,&autoButton,&resetButton,&globalListenButton,&hqButton,&characterButtons[0],&characterButtons[1],&characterButtons[2]})button->setLookAndFeel(nullptr);}
void SkinnedProcessorHardwarePanel::setModule(dozi::core::MasteringModulePlan*value){module=value;if(module){const auto add=[this](const std::string&key,double v){module->parameters.try_emplace(key,v);};using Type=dozi::core::MasteringModuleType;switch(module->type){case Type::dynamicEq:for(int n=1;n<=4;++n){const auto p="band"+std::to_string(n)+".";if(module->parameters.contains(p+"frequencyHz")){add(p+"thresholdDbfs",-26);add(p+"attackMs",10);add(p+"releaseMs",120);add(p+"enabled",1);add(p+"listen",0);}}break;case Type::deEsser:add("listen",0);break;case Type::broadbandCompressor:add("sidechainHpfHz",80);add("automaticRelease",0);break;case Type::multibandCompressor:add("lowCrossoverHz",120);add("midCrossoverHz",700);add("highCrossoverHz",4000);for(const auto*name:{"low","lowMid","highMid","high"}){const auto p=std::string(name)+".";add(p+"thresholdDbfs",-18);add(p+"maximumReductionDb",module->parameters.contains("maximumGainReductionDb")?module->parameters.at("maximumGainReductionDb"):3);add(p+"solo",0);}break;case Type::saturation:add("character",1);add("oversamplingFactor",4);break;case Type::stereoWidthMonoBass:add("sideTrimDb",0);add("balance",0);add("monoCheck",0);add("swapLeftRight",0);break;case Type::truePeakLimiter:add("linked",1);add("truePeakEnabled",1);break;case Type::linearPhaseEq:break;}}analysisSettings=module?(module->recommendedParameters.empty()?module->parameters:module->recommendedParameters):std::map<std::string,double>{};styleSettings=module?(module->styleParameters.empty()?module->parameters:module->styleParameters):std::map<std::string,double>{};const auto measured=module?std::ranges::find(availableTelemetry,module->type,&dozi::core::ProcessorTelemetry::type):availableTelemetry.end();telemetry=measured==availableTelemetry.end()?std::optional<dozi::core::ProcessorTelemetry>{}:std::optional<dozi::core::ProcessorTelemetry>{*measured};rebuild();}
void SkinnedProcessorHardwarePanel::refreshFromModule(){rebuild();}
void SkinnedProcessorHardwarePanel::setTelemetry(std::optional<dozi::core::ProcessorTelemetry>value){telemetry=std::move(value);repaint();}
void SkinnedProcessorHardwarePanel::setAvailableTelemetry(const std::vector<dozi::core::ProcessorTelemetry>&value){availableTelemetry=value;const auto measured=module?std::ranges::find(availableTelemetry,module->type,&dozi::core::ProcessorTelemetry::type):availableTelemetry.end();setTelemetry(measured==availableTelemetry.end()?std::optional<dozi::core::ProcessorTelemetry>{}:std::optional<dozi::core::ProcessorTelemetry>{*measured});}
void SkinnedProcessorHardwarePanel::applyAnalysisSettings(){if(!module)return;module->parameters=analysisSettings;rebuild();if(onModuleChanged)onModuleChanged();}
void SkinnedProcessorHardwarePanel::applyStyleSettings(){if(!module)return;module->parameters=styleSettings;rebuild();if(onModuleChanged)onModuleChanged();}
void SkinnedProcessorHardwarePanel::rebuild(){
    for(auto&control:controls)control.slider->setLookAndFeel(nullptr);controls.clear();for(auto&item:switches)item.button->setLookAndFeel(nullptr);switches.clear();
    if(!module){skin={};repaint();return;}
    using Type=dozi::core::MasteringModuleType;
    switch(module->type){
        case Type::linearPhaseEq:skin={};break;
        case Type::dynamicEq:skin=juce::ImageFileFormat::loadFrom(BinaryData::dynamiceqskin_png,BinaryData::dynamiceqskin_pngSize);break;
        case Type::deEsser:skin=juce::ImageFileFormat::loadFrom(BinaryData::deesserskin_png,BinaryData::deesserskin_pngSize);break;
        case Type::broadbandCompressor:skin=juce::ImageFileFormat::loadFrom(BinaryData::gluecompressorskin_png,BinaryData::gluecompressorskin_pngSize);break;
        case Type::multibandCompressor:skin=juce::ImageFileFormat::loadFrom(BinaryData::multibandcompressorskin_png,BinaryData::multibandcompressorskin_pngSize);break;
        case Type::saturation:skin=juce::ImageFileFormat::loadFrom(BinaryData::saturationskin_png,BinaryData::saturationskin_pngSize);break;
        case Type::stereoWidthMonoBass:skin=juce::ImageFileFormat::loadFrom(BinaryData::widthmonobassskin_png,BinaryData::widthmonobassskin_pngSize);break;
        case Type::truePeakLimiter:skin=juce::ImageFileFormat::loadFrom(BinaryData::truepeaklimiterskin_png,BinaryData::truepeaklimiterskin_pngSize);break;
        default:skin={};break;
    }
    inButton.setToggleState(!module->bypassed,juce::dontSendNotification);autoButton.setEnabled(!analysisSettings.empty());resetButton.setEnabled(!analysisSettings.empty());
    globalListenButton.setToggleState(std::ranges::any_of(module->parameters,[](const auto&entry){return entry.first.ends_with(".listen")&&entry.second>.5;}),juce::dontSendNotification);
    const auto character=static_cast<int>(module->parameters.contains("character")?module->parameters.at("character"):1);for(std::size_t i=0;i<characterButtons.size();++i)characterButtons[i].setToggleState(static_cast<int>(i)==character,juce::dontSendNotification);
    hqButton.setToggleState(module->parameters.contains("oversamplingFactor")&&module->parameters.at("oversamplingFactor")>=4,juce::dontSendNotification);
    const auto isSwitch=[](const std::string&key){return key=="automaticRelease"||key=="linked"||key=="truePeakEnabled"||key=="monoCheck"||key=="swapLeftRight"||key=="automaticLevelCompensation"||key=="listen"||key.ends_with(".listen")||key.ends_with(".solo")||key.ends_with(".enabled");};
    for(const auto&[key,value]:module->parameters){
        if(isSwitch(key)){Switch item;item.key=key;item.button=std::make_unique<juce::TextButton>(juce::String(key));item.button->setComponentID(juce::String(key));item.button->setTitle(juce::String(key));item.button->setLookAndFeel(&lookAndFeel);item.button->setClickingTogglesState(true);item.button->setToggleState(value>.5,juce::dontSendNotification);auto*button=item.button.get();button->onClick=[this,button,key]{if(!module)return;module->parameters[key]=button->getToggleState()?1.0:0.0;repaint();if(onModuleChanged)onModuleChanged();};addAndMakeVisible(*item.button);switches.push_back(std::move(item));continue;}
        Control control;control.key=key;control.slider=std::make_unique<juce::Slider>();auto&slider=*control.slider;slider.setComponentID(juce::String(key));slider.setTitle(juce::String(key));
        slider.setLookAndFeel(&lookAndFeel);slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,112,24);
        if(key.find("frequencyHz")!=std::string::npos||key.find("CrossoverHz")!=std::string::npos||key.find("HpfHz")!=std::string::npos){slider.setRange(20,20000,1);slider.setTextValueSuffix(" Hz");}
        else if(key.find("threshold")!=std::string::npos){slider.setRange(-60,0,.1);slider.setTextValueSuffix(" dB");}
        else if(key.find("attackMs")!=std::string::npos||key.find("releaseMs")!=std::string::npos||key.find("lookaheadMs")!=std::string::npos){slider.setRange(.1,1000,.1);slider.setTextValueSuffix(" ms");}
        else if(key.find("ratio")!=std::string::npos){slider.setRange(1,20,.1);slider.setTextValueSuffix(":1");}
        else if(key=="q"){slider.setRange(.1,20,.1);}
        else if(key.find("maximumReductionDb")!=std::string::npos||key.find("maximumGainReductionDb")!=std::string::npos){slider.setRange(0,24,.1);slider.setTextValueSuffix(" dB");}
        else if(key.find("ceilingDb")!=std::string::npos){slider.setRange(-6,0,.1);slider.setTextValueSuffix(" dBTP");}
        else if(key=="mix"||key.find("widthFactor")!=std::string::npos){slider.setRange(0,2,.01);}
        else if(key=="mode"||key=="character"){slider.setRange(0,2,1);}
        else if(key=="oversamplingFactor"){slider.setRange(1,8,1);}
        else if(key=="balance"){slider.setRange(-1,1,.01);}
        else {slider.setRange(-24,24,.1);slider.setTextValueSuffix(" dB");}
        slider.setValue(value,juce::dontSendNotification);auto*sliderPtr=&slider;const auto parameterKey=key;
        slider.onValueChange=[this,sliderPtr,parameterKey]{if(!module)return;module->parameters[parameterKey]=sliderPtr->getValue();repaint();if(onModuleChanged)onModuleChanged();};
        addAndMakeVisible(slider);controls.push_back(std::move(control));
    }
    resized();repaint();
}
void SkinnedProcessorHardwarePanel::paint(juce::Graphics&g){
    if(skin.isValid())g.drawImage(skin,getLocalBounds().toFloat());else g.fillAll(juce::Colour(0xff111820));
    paintProcessorDisplay(g);
    if(module&&module->bypassed){const auto area=inButton.getBounds().toFloat().reduced(inButton.getWidth()*.18f);g.setColour(juce::Colour(0xff071016).withAlpha(.94f));g.fillEllipse(area);g.setColour(juce::Colour(0xff323b42));g.drawEllipse(area,3.0f);}
    for(const auto&item:switches)if(item.button->isVisible()){
        const auto area=item.button->getBounds().toFloat();
        if(item.button->getToggleState()){g.setColour(juce::Colour(0xff43e4d5).withAlpha(.85f));g.drawRoundedRectangle(area.reduced(2),5,3);}
        else{g.setColour(juce::Colour(0xff071016).withAlpha(.46f));g.fillRoundedRectangle(area.reduced(4),4);}
    }
    for(const auto*button:{&globalListenButton,&hqButton,&characterButtons[0],&characterButtons[1],&characterButtons[2]})if(button->isVisible()){
        const auto area=button->getBounds().toFloat();if(button->getToggleState()){g.setColour(juce::Colour(0xffffb23e).withAlpha(.82f));g.drawRoundedRectangle(area.reduced(2),5,3);}else{g.setColour(juce::Colour(0xff071016).withAlpha(.30f));g.fillRoundedRectangle(area.reduced(4),4);}
    }
}

double SkinnedProcessorHardwarePanel::parameter(const std::string& key,double fallback)const{
    if(!module)return fallback;const auto found=module->parameters.find(key);return found==module->parameters.end()?fallback:found->second;
}

void SkinnedProcessorHardwarePanel::paintProcessorDisplay(juce::Graphics&g){
    if(!module)return;
    using Type=dozi::core::MasteringModuleType;
    juce::Rectangle<float> area;
    switch(module->type){
        case Type::dynamicEq:area={268,138,798,188};break;
        case Type::deEsser:area={388,125,420,305};break;
        case Type::broadbandCompressor:area={360,142,480,170};break;
        case Type::multibandCompressor:area={425,107,395,346};break;
        case Type::saturation:area={365,158,462,233};break;
        case Type::stereoWidthMonoBass:area={405,110,390,334};break;
        case Type::truePeakLimiter:area={285,77,632,357};break;
        case Type::linearPhaseEq:return;
    }
    g.setColour(juce::Colour(0xff030b0f));g.fillRoundedRectangle(area,8.0f);
    g.setColour(juce::Colour(0xff536874));g.drawRoundedRectangle(area,8.0f,2.0f);
    auto plot=area.reduced(18.0f,28.0f);
    g.setColour(juce::Colour(0xff193038));
    for(int n=1;n<6;++n){const auto x=plot.getX()+plot.getWidth()*n/6.0f;g.drawVerticalLine(juce::roundToInt(x),plot.getY(),plot.getBottom());}
    for(int n=1;n<4;++n){const auto y=plot.getY()+plot.getHeight()*n/4.0f;g.drawHorizontalLine(juce::roundToInt(y),plot.getX(),plot.getRight());}
    const auto cyan=juce::Colour(0xff40dfd1),amber=juce::Colour(0xffffad32),muted=juce::Colour(0xff91a6b0);
    const auto drawSpectrum=[&](juce::Rectangle<float>bounds){if(!telemetry)return;const auto width=bounds.getWidth()/telemetry->spectrumDbfs.size();for(std::size_t index=0;index<telemetry->spectrumDbfs.size();++index){const auto normalized=juce::jlimit(0.0,1.0,(telemetry->spectrumDbfs[index]+72.0)/72.0);g.setColour(cyan.withAlpha(.32f));g.fillRect(bounds.getX()+static_cast<float>(index)*width,bounds.getBottom()-static_cast<float>(normalized)*bounds.getHeight(),juce::jmax(1.0f,width-1),static_cast<float>(normalized)*bounds.getHeight());}};
    g.setFont(juce::FontOptions(14.0f).withStyle("Bold"));
    if(module->type==Type::dynamicEq){
        g.setColour(cyan);g.drawText("PARAMETRIC RESPONSE",area.removeFromTop(28),juce::Justification::centred);
        juce::Path curve;const auto mid=plot.getCentreY();
        for(int px=0;px<=juce::roundToInt(plot.getWidth());++px){const auto frequency=20.0*std::pow(1000.0,px/plot.getWidth());double db=0;
            for(int n=1;n<=4;++n){const auto p="band"+std::to_string(n)+".";if(parameter(p+"enabled",1)<.5)continue;const auto f=parameter(p+"frequencyHz",1000),gain=parameter(p+"gainDb",0),q=std::max(.1,parameter(p+"q",1));const auto oct=std::log2(frequency/f);db+=gain*std::exp(-.5*std::pow(oct*q*1.35,2));}
            const auto point=juce::Point<float>(plot.getX()+px,mid-static_cast<float>(db)*plot.getHeight()/24.0f);if(px==0)curve.startNewSubPath(point);else curve.lineTo(point);}
        drawSpectrum(plot);g.setColour(cyan);g.strokePath(curve,juce::PathStrokeType(2.5f));
    }else if(module->type==Type::deEsser){
        g.setColour(cyan);g.drawText("SIBILANCE CONTROL - PARAMETER VIEW",area.removeFromTop(28),juce::Justification::centred);
        const auto f=parameter("frequencyHz",6500);const auto range=static_cast<float>(parameter("maximumReductionDb",6));const auto centre=plot.getX()+plot.getWidth()*static_cast<float>(std::log(f/1000.0)/std::log(20.0));
        juce::Path response;response.startNewSubPath(plot.getX(),plot.getCentreY());response.cubicTo(centre-80,plot.getCentreY(),centre-45,plot.getCentreY()+range*5,centre,plot.getCentreY()+range*5);response.cubicTo(centre+45,plot.getCentreY()+range*5,centre+80,plot.getCentreY(),plot.getRight(),plot.getCentreY());g.setColour(amber);g.strokePath(response,juce::PathStrokeType(2.5f));
        drawSpectrum(plot);g.setColour(telemetry?amber:muted);g.drawText(telemetry?"MEASURED REDUCTION "+juce::String(telemetry->maximumGainReductionDb,1)+" dB":"WAITING FOR AUDIO TELEMETRY",plot.removeFromBottom(24),juce::Justification::centred);
    }else if(module->type==Type::broadbandCompressor){
        g.setColour(cyan);g.drawText("COMPRESSION CURVE",area.removeFromTop(28),juce::Justification::centred);
        const auto threshold=parameter("thresholdDbfs",-18),ratio=std::max(1.0,parameter("ratio",2));juce::Path curve;
        for(int n=0;n<=100;++n){const auto input=-60.0+n*.6;const auto output=input<=threshold?input:threshold+(input-threshold)/ratio;const auto p=juce::Point<float>(plot.getX()+n*plot.getWidth()/100.0f,plot.getBottom()-static_cast<float>((output+60)/60)*plot.getHeight());if(n==0)curve.startNewSubPath(p);else curve.lineTo(p);}g.setColour(cyan);g.strokePath(curve,juce::PathStrokeType(2.5f));g.setColour(telemetry?cyan:muted);g.drawText(telemetry?"IN "+juce::String(telemetry->inputPeakDbfs,1)+" dBFS   OUT "+juce::String(telemetry->outputPeakDbfs,1)+" dBFS   GR "+juce::String(telemetry->maximumGainReductionDb,1)+" dB":"WAITING FOR AUDIO TELEMETRY",plot.removeFromBottom(22),juce::Justification::centred);
    }else if(module->type==Type::multibandCompressor){
        g.setColour(cyan);g.drawText("FOUR-BAND CONTROL",area.removeFromTop(28),juce::Justification::centred);
        const std::array<double,3> cross{parameter("lowCrossoverHz",120),parameter("midCrossoverHz",700),parameter("highCrossoverHz",4000)};for(std::size_t n=0;n<cross.size();++n){const auto x=plot.getX()+plot.getWidth()*static_cast<float>(std::log(cross[n]/20.0)/std::log(1000.0));g.setColour(n==2?amber:cyan);g.drawVerticalLine(juce::roundToInt(x),plot.getY(),plot.getBottom());}
        const std::array<const char*,4> names{"LOW","LOW MID","HIGH MID","HIGH"};for(int n=0;n<4;++n){auto column=plot.withX(plot.getX()+plot.getWidth()*n/4).withWidth(plot.getWidth()/4);g.setColour(cyan);g.drawText(names[static_cast<std::size_t>(n)],column.removeFromTop(24),juce::Justification::centred);g.setColour(telemetry?amber:muted);g.drawText(telemetry?"GR "+juce::String(telemetry->maximumGainReductionDb,1):"GR --",column.removeFromBottom(28),juce::Justification::centred);}
    }else if(module->type==Type::saturation){
        const auto drive=parameter("driveDb",0);const auto character=juce::jlimit(0,2,static_cast<int>(parameter("character",1)));const std::array<const char*,3> names{"CLEAN","WARM","RICH"};g.setColour(amber);g.drawText(juce::String(names[static_cast<std::size_t>(character)])+" TRANSFER",area.removeFromTop(28),juce::Justification::centred);juce::Path curve;
        const auto amount=1.0+std::max(0.0,drive)/8.0+character*.35;for(int n=0;n<=100;++n){const auto x=-1.0+n*.02;const auto y=std::tanh(x*amount)/std::tanh(amount);const auto p=juce::Point<float>(plot.getX()+n*plot.getWidth()/100.0f,plot.getCentreY()-static_cast<float>(y)*plot.getHeight()*.45f);if(n==0)curve.startNewSubPath(p);else curve.lineTo(p);}drawSpectrum(plot.withTrimmedLeft(plot.getWidth()*.55f));g.setColour(amber);g.strokePath(curve,juce::PathStrokeType(3.0f));g.setColour(cyan.withAlpha(.65f));g.drawLine(plot.getX(),plot.getBottom(),plot.getRight(),plot.getY(),1.5f);
    }else if(module->type==Type::stereoWidthMonoBass){
        g.setColour(cyan);g.drawText("STEREO FIELD - MEASURED",area.removeFromTop(28),juce::Justification::centred);const auto width=static_cast<float>(parameter("widthFactor",1));const auto scope=plot.withWidth(plot.getHeight()).withCentre(plot.getCentre());g.setColour(juce::Colour(0xff244852));g.drawEllipse(scope,1.5f);g.drawLine(scope.getCentreX(),scope.getY(),scope.getCentreX(),scope.getBottom());g.drawLine(scope.getX(),scope.getCentreY(),scope.getRight(),scope.getCentreY());g.setColour(cyan.withAlpha(.75f));if(telemetry&&!telemetry->vectorscope.empty())for(const auto&point:telemetry->vectorscope){const auto mid=(point[0]+point[1])*.5,side=(point[0]-point[1])*.5;g.fillEllipse(scope.getCentreX()+static_cast<float>(side)*scope.getWidth()*.45f-1.5f,scope.getCentreY()-static_cast<float>(mid)*scope.getHeight()*.45f-1.5f,3,3);}else for(int n=0;n<45;++n){const auto t=n/44.0f;const auto y=scope.getBottom()-t*scope.getHeight();const auto spread=std::sin(t*31.0f)*scope.getWidth()*.12f*width;g.fillEllipse(scope.getCentreX()+spread-1.5f,y-1.5f,3,3);}g.setColour(telemetry?cyan:muted);g.drawText(telemetry?"PHASE CORRELATION "+juce::String(telemetry->phaseCorrelation,2):"WAITING FOR AUDIO TELEMETRY",plot.removeFromBottom(22),juce::Justification::centred);
    }else if(module->type==Type::truePeakLimiter){
        g.setColour(cyan);g.drawText("TRUE-PEAK CONTROL",area.removeFromTop(28),juce::Justification::centred);const auto ceiling=parameter("ceilingDbtp",-1),input=parameter("inputGainDb",0);g.setFont(juce::FontOptions(30.0f).withStyle("Bold"));g.setColour(cyan);g.drawText(juce::String(ceiling,1)+" dBTP",plot.removeFromTop(65),juce::Justification::centred);g.setFont(juce::FontOptions(14.0f));g.setColour(amber);g.drawText("INPUT "+juce::String(input,1)+" dB",plot.removeFromTop(28),juce::Justification::centred);g.setColour(telemetry?amber:muted);g.drawText(telemetry?"OUTPUT "+juce::String(telemetry->outputPeakDbfs,1)+" dBFS   GR "+juce::String(telemetry->maximumGainReductionDb,1)+" dB   PEAK EVENTS "+juce::String(static_cast<int>(telemetry->peakEventCount)):"WAITING FOR AUDIO TELEMETRY",plot,juce::Justification::centred);
    }
}
void SkinnedProcessorHardwarePanel::resized(){
    for(auto&control:controls)control.slider->setVisible(false);for(auto&item:switches)item.button->setVisible(false);globalListenButton.setVisible(false);hqButton.setVisible(false);for(auto&button:characterButtons)button.setVisible(false);
    if(!module)return;
    const auto place=[this](const std::string&key,int x,int y,int w=106,int h=116){for(auto&control:controls)if(control.key==key){control.slider->setBounds(x,y,w,h);control.slider->setVisible(true);}};
    const auto placeSwitch=[this](const std::string&key,int x,int y,int w=80,int h=54){for(auto&item:switches)if(item.key==key){item.button->setBounds(x,y,w,h);item.button->setVisible(true);}};
    using Type=dozi::core::MasteringModuleType;
    switch(module->type){
        case Type::dynamicEq:{inButton.setBounds(116,190,92,92);autoButton.setBounds(950,64,64,48);resetButton.setBounds(1026,64,64,48);globalListenButton.setBounds(117,330,72,44);globalListenButton.setVisible(true);for(int n=1;n<=4;++n){const auto prefix="band"+std::to_string(n)+".";const auto base=128+(n-1)*246;place(prefix+"frequencyHz",base,444);place(prefix+"thresholdDbfs",base+76,444);place(prefix+"gainDb",base+152,444);placeSwitch(prefix+"enabled",base+180,365,34,34);}break;}
        case Type::deEsser:inButton.setBounds(92,282,92,92);autoButton.setBounds(472,482,76,52);resetButton.setBounds(640,482,76,52);place("frequencyHz",235,176,112,130);place("thresholdDbfs",235,400,112,130);place("maximumReductionDb",970,178,112,130);place("releaseMs",970,400,112,130);placeSwitch("listen",852,452,76,52);break;
        case Type::broadbandCompressor:inButton.setBounds(126,166,88,88);autoButton.setBounds(958,165,66,54);resetButton.setBounds(1050,165,66,54);place("thresholdDbfs",130,424);place("attackMs",330,424);place("releaseMs",510,424);place("ratio",690,424);place("makeupGainDb",850,424);placeSwitch("automaticRelease",1000,388,76,54);place("sidechainHpfHz",1000,480,92,100);break;
        case Type::multibandCompressor:{inButton.setBounds(57,255,76,76);autoButton.setBounds(930,36,62,52);resetButton.setBounds(1012,36,62,52);const std::array names{"low","lowMid","highMid","high"};const std::array xs{145,315,850,1020};for(std::size_t n=0;n<names.size();++n){place(std::string(names[n])+".thresholdDbfs",xs[n],188,84,94);place(std::string(names[n])+".maximumReductionDb",xs[n],330,84,94);placeSwitch(std::string(names[n])+".solo",xs[n]+5,435,72,48);}place("lowCrossoverHz",250,520,88,100);place("midCrossoverHz",560,520,88,100);place("highCrossoverHz",850,520,88,100);break;}
        case Type::saturation:inButton.setBounds(105,392,88,88);autoButton.setBounds(906,100,64,50);resetButton.setBounds(990,100,64,50);place("driveDb",225,230,116,132);place("character",225,410,116,132);place("mix",880,230,116,132);place("outputTrimDb",880,410,116,132);for(std::size_t i=0;i<characterButtons.size();++i){characterButtons[i].setBounds(390+static_cast<int>(i)*140,430,116,58);characterButtons[i].setVisible(true);}hqButton.setBounds(786,486,70,72);hqButton.setVisible(true);break;
        case Type::stereoWidthMonoBass:inButton.setBounds(105,305,92,92);autoButton.setBounds(1045,192,72,68);resetButton.setBounds(1045,405,72,68);place("monoBassCrossoverHz",245,152,118,136);place("sideTrimDb",245,390,118,136);place("widthFactor",885,152,130,145);place("balance",885,390,118,136);placeSwitch("monoCheck",435,500,140,60);placeSwitch("swapLeftRight",640,500,140,60);break;
        case Type::truePeakLimiter:inButton.setBounds(105,390,96,96);autoButton.setBounds(620,484,76,56);resetButton.setBounds(752,484,76,56);place("inputGainDb",120,200,120,145);place("ceilingDbtp",982,120,116,136);place("releaseMs",982,300,116,130);place("lookaheadMs",982,468,116,120);placeSwitch("linked",330,484,80,56);placeSwitch("truePeakEnabled",470,484,80,56);break;
        case Type::linearPhaseEq:break;
    }
}
