#include "MasteringChainView.h"

#include <algorithm>

namespace {
using Type=dozi::core::MasteringModuleType;
juce::String name(Type type){switch(type){case Type::linearPhaseEq:return "CORRECTIVE EQ";case Type::dynamicEq:return "DYNAMIC EQ";case Type::deEsser:return "DE-ESSER";case Type::broadbandCompressor:return "GLUE COMPRESSOR";case Type::multibandCompressor:return "MULTIBAND";case Type::saturation:return "SATURATION";case Type::stereoWidthMonoBass:return "WIDTH / MONO BASS";case Type::truePeakLimiter:return "TRUE PEAK LIMITER";}return "PROCESSOR";}
juce::Colour accent(Type type){switch(type){case Type::linearPhaseEq:return juce::Colour(0xff38dfc9);case Type::dynamicEq:return juce::Colour(0xff4eb5e9);case Type::deEsser:return juce::Colour(0xffcf79e8);case Type::broadbandCompressor:return juce::Colour(0xffffae3c);case Type::multibandCompressor:return juce::Colour(0xff62d58b);case Type::saturation:return juce::Colour(0xffff795f);case Type::stereoWidthMonoBass:return juce::Colour(0xff6ea4ff);case Type::truePeakLimiter:return juce::Colour(0xffffd15b);}return juce::Colours::white;}
}

class MasteringChainView::ProcessorCard final : public juce::Component {
public:
    ProcessorCard(std::size_t value,std::function<void(std::size_t)> callback):index(value),open(std::move(callback)){setMouseCursor(juce::MouseCursor::PointingHandCursor);}
    void setModule(dozi::core::MasteringModulePlan* value){module=value;repaint();}
    void paint(juce::Graphics&g)override{auto b=getLocalBounds().toFloat().reduced(2);const auto colour=module?accent(module->type):juce::Colour(0xff52606a);g.setGradientFill({juce::Colour(0xff353c43),b.getX(),b.getY(),juce::Colour(0xff11171c),b.getRight(),b.getBottom(),false});g.fillRoundedRectangle(b,9);g.setColour(colour.withAlpha(module&&!module->bypassed?1.0f:.3f));g.drawRoundedRectangle(b,9,module&&!module->bypassed?2.5f:1.2f);for(auto p:{b.getTopLeft()+juce::Point<float>(10,10),b.getTopRight()+juce::Point<float>(-10,10),b.getBottomLeft()+juce::Point<float>(10,-10),b.getBottomRight()+juce::Point<float>(-10,-10)}){g.setColour(juce::Colour(0xff080a0c));g.fillEllipse(p.x-3,p.y-3,6,6);}g.setColour(juce::Colour(0xff0a1014));g.fillRoundedRectangle(b.reduced(20,30),6);g.setColour(colour.withAlpha(.28f));for(int x=0;x<7;++x)g.drawVerticalLine(juce::roundToInt(b.getX()+25+x*(b.getWidth()-50)/6),b.getY()+38,b.getBottom()-40);g.setColour(colour);g.setFont(juce::FontOptions(15).withStyle("Bold"));g.drawText(juce::String(static_cast<int>(index)+1)+"  "+(module?name(module->type):"EMPTY"),b.reduced(14).removeFromTop(24),juce::Justification::centred);g.setFont(juce::FontOptions(12));g.setColour(module&&!module->bypassed?juce::Colour(0xffe9dfcf):juce::Colour(0xff7a858d));g.drawText(module&&!module->bypassed?"IN  -  DOUBLE-CLICK TO OPEN":"BYPASSED  -  DOUBLE-CLICK TO OPEN",b.reduced(14).removeFromBottom(25),juce::Justification::centred);}
    void mouseDoubleClick(const juce::MouseEvent&)override{if(module&&open)open(index);}
private:
    std::size_t index;std::function<void(std::size_t)> open;dozi::core::MasteringModulePlan* module=nullptr;
};

class MasteringChainView::ProcessorWindow final : public juce::DocumentWindow {
public:
    ProcessorWindow(dozi::core::MasteringPlan* value,Type type,std::function<void()> changed):juce::DocumentWindow("DOZI / "+name(type),juce::Colour(0xff10151b),juce::DocumentWindow::allButtons){setUsingNativeTitleBar(true);setResizable(true,true);setResizeLimits(620,430,1800,1200);inspector=std::make_unique<MasteringModuleInspector>();inspector->onPlanChanged=std::move(changed);inspector->setPlan(value);inspector->selectModule(type);setContentOwned(inspector.release(),true);centreWithSize(1100,720);setVisible(true);}
    void refresh(dozi::core::MasteringPlan* value,const std::vector<dozi::core::ProcessorTelemetry>& measurements){auto*view=dynamic_cast<MasteringModuleInspector*>(getContentComponent());if(view){view->setPlan(value);view->setTelemetry(measurements);}}
    void update(const dozi::core::ProcessorTelemetry& value){if(auto*view=dynamic_cast<MasteringModuleInspector*>(getContentComponent()))view->updateTelemetry(value);}
    void updateGlue(double in,double out,double gr,bool active){if(auto*view=dynamic_cast<MasteringModuleInspector*>(getContentComponent()))view->updateGlueLiveMetering(in,out,gr,active);}
    void closeButtonPressed()override{setVisible(false);}
private:std::unique_ptr<MasteringModuleInspector> inspector;
};

MasteringChainView::MasteringChainView(){for(std::size_t i=0;i<cards.size();++i){cards[i]=std::make_unique<ProcessorCard>(i,[this](std::size_t index){openProcessor(index);});addAndMakeVisible(*cards[i]);}}
MasteringChainView::~MasteringChainView(){for(auto&window:windows)window.reset();}
void MasteringChainView::setPlan(dozi::core::MasteringPlan* value){plan=value;for(std::size_t i=0;i<cards.size();++i)cards[i]->setModule(plan&&i<plan->modules.size()?&plan->modules[i]:nullptr);for(auto&window:windows)if(window)window->refresh(plan,telemetry);repaint();}
void MasteringChainView::setTelemetry(const std::vector<dozi::core::ProcessorTelemetry>& value){telemetry=value;for(auto&window:windows)if(window)window->refresh(plan,telemetry);}
void MasteringChainView::updateTelemetry(const dozi::core::ProcessorTelemetry& value){const auto found=std::ranges::find(telemetry,value.type,&dozi::core::ProcessorTelemetry::type);if(found==telemetry.end())telemetry.push_back(value);else*found=value;for(auto&window:windows)if(window)window->update(value);}
void MasteringChainView::updateGlueLiveMetering(double in,double out,double gr,bool active){for(auto&window:windows)if(window)window->updateGlue(in,out,gr,active);}
void MasteringChainView::openProcessorEditor(dozi::core::MasteringModuleType type){if(!plan)return;const auto found=std::ranges::find(plan->modules,type,&dozi::core::MasteringModulePlan::type);if(found!=plan->modules.end())openProcessor(static_cast<std::size_t>(std::distance(plan->modules.begin(),found)));}
void MasteringChainView::closeAllEditors(){for(auto&window:windows)if(window)window->setVisible(false);}
std::size_t MasteringChainView::openEditorCount()const{return static_cast<std::size_t>(std::ranges::count_if(windows,[](const auto&window){return window&&window->isVisible();}));}
juce::Rectangle<int> MasteringChainView::processorBounds(std::size_t index)const{return index<cards.size()?cards[index]->getBounds():juce::Rectangle<int>{};}
void MasteringChainView::openProcessor(std::size_t index){if(!plan||index>=plan->modules.size())return;if(!windows[index])windows[index]=std::make_unique<ProcessorWindow>(plan,plan->modules[index].type,[this]{setPlan(plan);if(onPlanChanged)onPlanChanged();});else{windows[index]->refresh(plan,telemetry);windows[index]->setVisible(true);windows[index]->toFront(true);}}
void MasteringChainView::paint(juce::Graphics&g){g.fillAll(juce::Colour(0xff0d1319));g.setColour(juce::Colour(0xff8799a3));g.setFont(juce::FontOptions(13).withStyle("Bold"));g.drawText("MASTERING SIGNAL CHAIN  /  DOUBLE-CLICK ANY PROCESSOR TO OPEN AN INDEPENDENT EDITOR",getLocalBounds().removeFromTop(28),juce::Justification::centred);if(cards[0]->getWidth()==0)return;juce::Path route;const auto centre=[](const auto&card){return card->getBounds().toFloat().getCentre();};for(std::size_t i=0;i<3;++i){route.startNewSubPath(cards[i]->getRight(),centre(cards[i]).y);route.lineTo(cards[i+1]->getX(),centre(cards[i+1]).y);}const auto gutter=static_cast<float>(getWidth()-8);route.startNewSubPath(cards[3]->getRight(),centre(cards[3]).y);route.lineTo(gutter,centre(cards[3]).y);route.lineTo(gutter,centre(cards[4]).y);route.lineTo(cards[4]->getX(),centre(cards[4]).y);for(std::size_t i=4;i<7;++i){route.startNewSubPath(cards[i]->getRight(),centre(cards[i]).y);route.lineTo(cards[i+1]->getX(),centre(cards[i+1]).y);}g.setColour(juce::Colour(0xff38dfc9).withAlpha(.7f));g.strokePath(route,juce::PathStrokeType(3));}
void MasteringChainView::resized(){auto area=getLocalBounds().reduced(18);area.removeFromTop(30);const auto gap=18;const auto cardWidth=(area.getWidth()-3*gap)/4;const auto cardHeight=(area.getHeight()-gap)/2;for(int row=0;row<2;++row)for(int column=0;column<4;++column){const auto index=static_cast<std::size_t>(row*4+column);cards[index]->setBounds(area.getX()+column*(cardWidth+gap),area.getY()+row*(cardHeight+gap),cardWidth,cardHeight);}}
