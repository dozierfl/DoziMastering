#include "MasteringChainView.h"

#include <algorithm>

namespace {
using Type=dozi::core::MasteringModuleType;
juce::String name(Type type){switch(type){case Type::linearPhaseEq:return "CORRECTIVE EQ";case Type::dynamicEq:return "DYNAMIC EQ";case Type::deEsser:return "DE-ESSER";case Type::broadbandCompressor:return "GLUE COMPRESSOR";case Type::multibandCompressor:return "MULTIBAND";case Type::saturation:return "SATURATION";case Type::stereoWidthMonoBass:return "WIDTH / MONO BASS";case Type::truePeakLimiter:return "TRUE PEAK LIMITER";}return "PROCESSOR";}
}

class MasteringChainView::ProcessorCard final : public juce::Component {
public:
    ProcessorCard(std::size_t value,std::function<void(std::size_t)> callback,std::function<void()> changed):index(value),open(std::move(callback)),change(std::move(changed)){addMouseListener(this,true);}
    ~ProcessorCard()override{removeMouseListener(this);panel.reset();}
    void setModule(dozi::core::MasteringModulePlan* value){const auto nextType=value?std::optional<Type>{value->type}:std::nullopt;const auto changedType=currentType!=nextType;module=value;currentType=nextType;if(changedType){panel.reset();if(module){if(module->type==Type::linearPhaseEq)panel=std::make_unique<CorrectiveEqHardwarePanel>();else if(module->type==Type::broadbandCompressor)panel=std::make_unique<GlueCompressorHardwarePanel>();else panel=std::make_unique<SkinnedProcessorHardwarePanel>();if(auto*eq=dynamic_cast<CorrectiveEqHardwarePanel*>(panel.get())){eq->onModuleChanged=change;eq->setModule(module);}else if(auto*glue=dynamic_cast<GlueCompressorHardwarePanel*>(panel.get())){glue->onModuleChanged=change;glue->setModule(module);}else if(auto*processor=dynamic_cast<SkinnedProcessorHardwarePanel*>(panel.get())){processor->onModuleChanged=change;processor->setModule(module);}addAndMakeVisible(*panel);}}else refresh();resized();repaint();}
    void refresh(){if(auto*eq=dynamic_cast<CorrectiveEqHardwarePanel*>(panel.get()))eq->refreshFromModule();else if(auto*glue=dynamic_cast<GlueCompressorHardwarePanel*>(panel.get()))glue->refreshFromModule();else if(auto*processor=dynamic_cast<SkinnedProcessorHardwarePanel*>(panel.get()))processor->refreshFromModule();}
    void setTelemetry(const std::vector<dozi::core::ProcessorTelemetry>& values){if(auto*processor=dynamic_cast<SkinnedProcessorHardwarePanel*>(panel.get()))processor->setAvailableTelemetry(values);if(auto*glue=dynamic_cast<GlueCompressorHardwarePanel*>(panel.get())){const auto found=module?std::ranges::find(values,module->type,&dozi::core::ProcessorTelemetry::type):values.end();glue->setTelemetry(found==values.end()?std::optional<dozi::core::ProcessorTelemetry>{}:std::optional<dozi::core::ProcessorTelemetry>{*found});}}
    void updateGlue(double in,double out,double gr,bool active){if(auto*glue=dynamic_cast<GlueCompressorHardwarePanel*>(panel.get()))glue->setLiveMetering(in,out,gr,active);}
    [[nodiscard]] juce::Component* hardware()const{return panel.get();}
    void paint(juce::Graphics&g)override{g.fillAll(juce::Colour(0xff080d12));g.setColour(module&&!module->bypassed?juce::Colour(0xff38dfc9):juce::Colour(0xff4d5960));g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1),7,module&&!module->bypassed?2.0f:1.0f);}
    void resized()override{if(!panel)return;const auto corrective=module&&module->type==Type::linearPhaseEq;const float designWidth=1200,designHeight=corrective?577:650;const auto scale=juce::jmin((getWidth()-4)/designWidth,(getHeight()-4)/designHeight);const auto x=(getWidth()-designWidth*scale)*.5f,y=(getHeight()-designHeight*scale)*.5f;panel->setBounds(0,0,static_cast<int>(designWidth),static_cast<int>(designHeight));panel->setTransform(juce::AffineTransform(scale,0,x,0,scale,y));}
    void mouseDoubleClick(const juce::MouseEvent&)override{if(module&&open)open(index);}
private:
    std::size_t index;std::function<void(std::size_t)> open;std::function<void()> change;dozi::core::MasteringModulePlan* module=nullptr;std::optional<Type> currentType;std::unique_ptr<juce::Component> panel;
};

class MasteringChainView::ProcessorWindow final : public juce::DocumentWindow {
public:
    ProcessorWindow(dozi::core::MasteringPlan* value,Type type,std::function<void()> changed,std::function<bool(const juce::KeyPress&)> shortcut):juce::DocumentWindow("DOZI / "+name(type),juce::Colour(0xff10151b),juce::DocumentWindow::allButtons),transportShortcut(std::move(shortcut)){setUsingNativeTitleBar(true);setResizable(true,true);setResizeLimits(620,430,1800,1200);inspector=std::make_unique<MasteringModuleInspector>();inspector->onPlanChanged=std::move(changed);inspector->setPlan(value);inspector->selectModule(type);setContentOwned(inspector.release(),true);centreWithSize(1100,720);setVisible(true);}
    void setPlan(dozi::core::MasteringPlan* value,const std::vector<dozi::core::ProcessorTelemetry>& measurements){auto*view=dynamic_cast<MasteringModuleInspector*>(getContentComponent());if(view){view->setPlan(value);view->setTelemetry(measurements);}}
    void refresh(const std::vector<dozi::core::ProcessorTelemetry>& measurements){auto*view=dynamic_cast<MasteringModuleInspector*>(getContentComponent());if(view){view->refreshFromPlan();view->setTelemetry(measurements);}}
    void update(const dozi::core::ProcessorTelemetry& value){if(auto*view=dynamic_cast<MasteringModuleInspector*>(getContentComponent()))view->updateTelemetry(value);}
    void updateGlue(double in,double out,double gr,bool active){if(auto*view=dynamic_cast<MasteringModuleInspector*>(getContentComponent()))view->updateGlueLiveMetering(in,out,gr,active);}
    void closeButtonPressed()override{setVisible(false);}
    bool keyPressed(const juce::KeyPress& key)override{return transportShortcut&&transportShortcut(key);}
private:std::unique_ptr<MasteringModuleInspector> inspector;std::function<bool(const juce::KeyPress&)> transportShortcut;
};

MasteringChainView::MasteringChainView(){for(std::size_t i=0;i<cards.size();++i){cards[i]=std::make_unique<ProcessorCard>(i,[this](std::size_t index){openProcessor(index);},[this]{refreshFromPlan();if(onPlanChanged)onPlanChanged();});addAndMakeVisible(*cards[i]);}}
MasteringChainView::~MasteringChainView(){for(auto&window:windows)window.reset();}
void MasteringChainView::setPlan(dozi::core::MasteringPlan* value){plan=value;for(std::size_t i=0;i<cards.size();++i)cards[i]->setModule(plan&&i<plan->modules.size()?&plan->modules[i]:nullptr);for(auto&window:windows)if(window)window->setPlan(plan,telemetry);repaint();}
void MasteringChainView::refreshFromPlan(){for(auto&card:cards)card->refresh();for(auto&window:windows)if(window)window->refresh(telemetry);repaint();}
void MasteringChainView::setTelemetry(const std::vector<dozi::core::ProcessorTelemetry>& value){telemetry=value;for(auto&card:cards)card->setTelemetry(telemetry);for(auto&window:windows)if(window)window->refresh(telemetry);}
void MasteringChainView::updateTelemetry(const dozi::core::ProcessorTelemetry& value){const auto found=std::ranges::find(telemetry,value.type,&dozi::core::ProcessorTelemetry::type);if(found==telemetry.end())telemetry.push_back(value);else*found=value;for(auto&card:cards)card->setTelemetry(telemetry);for(auto&window:windows)if(window)window->update(value);}
void MasteringChainView::updateGlueLiveMetering(double in,double out,double gr,bool active){for(auto&card:cards)card->updateGlue(in,out,gr,active);for(auto&window:windows)if(window)window->updateGlue(in,out,gr,active);}
void MasteringChainView::openProcessorEditor(dozi::core::MasteringModuleType type){if(!plan)return;const auto found=std::ranges::find(plan->modules,type,&dozi::core::MasteringModulePlan::type);if(found!=plan->modules.end())openProcessor(static_cast<std::size_t>(std::distance(plan->modules.begin(),found)));}
void MasteringChainView::closeAllEditors(){for(auto&window:windows)if(window)window->setVisible(false);}
std::size_t MasteringChainView::openEditorCount()const{return static_cast<std::size_t>(std::ranges::count_if(windows,[](const auto&window){return window&&window->isVisible();}));}
juce::Rectangle<int> MasteringChainView::processorBounds(std::size_t index)const{return index<cards.size()?cards[index]->getBounds():juce::Rectangle<int>{};}
juce::Component* MasteringChainView::processorPanel(std::size_t index)const{return index<cards.size()?cards[index]->hardware():nullptr;}
bool MasteringChainView::handleProcessorWindowShortcut(const juce::KeyPress&key){return onTransportShortcut&&onTransportShortcut(key);}
void MasteringChainView::openProcessor(std::size_t index){if(!plan||index>=plan->modules.size())return;if(!windows[index])windows[index]=std::make_unique<ProcessorWindow>(plan,plan->modules[index].type,[this]{refreshFromPlan();if(onPlanChanged)onPlanChanged();},[this](const juce::KeyPress& key){return handleProcessorWindowShortcut(key);});else{windows[index]->refresh(telemetry);windows[index]->setVisible(true);windows[index]->toFront(true);}}
void MasteringChainView::paint(juce::Graphics&g){g.fillAll(juce::Colour(0xff0d1319));g.setColour(juce::Colour(0xff8799a3));g.setFont(juce::FontOptions(13).withStyle("Bold"));g.drawText("MASTERING SIGNAL CHAIN  /  DOUBLE-CLICK ANY PROCESSOR TO OPEN AN INDEPENDENT EDITOR",getLocalBounds().removeFromTop(28),juce::Justification::centred);if(cards[0]->getWidth()==0)return;juce::Path route;const auto centre=[](const auto&card){return card->getBounds().toFloat().getCentre();};for(std::size_t i=0;i<3;++i){route.startNewSubPath(cards[i]->getRight(),centre(cards[i]).y);route.lineTo(cards[i+1]->getX(),centre(cards[i+1]).y);}const auto gutter=static_cast<float>(getWidth()-8);route.startNewSubPath(cards[3]->getRight(),centre(cards[3]).y);route.lineTo(gutter,centre(cards[3]).y);route.lineTo(gutter,centre(cards[4]).y);route.lineTo(cards[4]->getX(),centre(cards[4]).y);for(std::size_t i=4;i<7;++i){route.startNewSubPath(cards[i]->getRight(),centre(cards[i]).y);route.lineTo(cards[i+1]->getX(),centre(cards[i+1]).y);}g.setColour(juce::Colour(0xff38dfc9).withAlpha(.7f));g.strokePath(route,juce::PathStrokeType(3));}
void MasteringChainView::resized(){auto area=getLocalBounds().reduced(18);area.removeFromTop(30);const auto gap=18;const auto cardWidth=(area.getWidth()-3*gap)/4;const auto cardHeight=(area.getHeight()-gap)/2;for(int row=0;row<2;++row)for(int column=0;column<4;++column){const auto index=static_cast<std::size_t>(row*4+column);cards[index]->setBounds(area.getX()+column*(cardWidth+gap),area.getY()+row*(cardHeight+gap),cardWidth,cardHeight);}}
