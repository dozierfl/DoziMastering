#include "MasteringWaveformView.h"

#include <algorithm>
#include <array>
#include <cmath>

void MasteringWaveformView::setWaveform(std::vector<ChannelEnvelope> value,double seconds){waveform=std::move(value);duration=std::max(0.0,seconds);playhead=0;clearSelection();repaint();}
void MasteringWaveformView::setPlayhead(double seconds){playhead=std::clamp(seconds,0.0,duration);repaint();}
void MasteringWaveformView::setHorizontalInset(float pixels){horizontalInset=std::max(0.0f,pixels);repaint();}
void MasteringWaveformView::setSelection(double start,double end){selectionStart=std::clamp(std::min(start,end),0.0,duration);selectionEnd=std::clamp(std::max(start,end),0.0,duration);repaint();}
void MasteringWaveformView::clearSelection(){selectionStart=selectionEnd=0;repaint();}
bool MasteringWaveformView::hasSelection()const{return duration>0&&selectionEnd-selectionStart>=0.02;}
double MasteringWaveformView::selectionStartSeconds()const{return selectionStart;}
double MasteringWaveformView::selectionEndSeconds()const{return selectionEnd;}
float MasteringWaveformView::amplitudeForDbfs(double dbfs){return dbfs<=-100.0?0.0f:static_cast<float>(std::pow(10.0,std::min(0.0,dbfs)/20.0));}
juce::Rectangle<float> MasteringWaveformView::waveformPlotBounds()const{constexpr float rulerWidth=52.0f;const auto left=std::max(rulerWidth,horizontalInset),right=std::max(left+1.0f,static_cast<float>(getWidth())-horizontalInset);return {left,0.0f,right-left,static_cast<float>(getHeight())};}
double MasteringWaveformView::secondsAt(int x)const{const auto plot=waveformPlotBounds();return duration*std::clamp((static_cast<float>(x)-plot.getX())/plot.getWidth(),0.0f,1.0f);}
void MasteringWaveformView::paint(juce::Graphics&g){
    const auto area=getLocalBounds();const auto plot=waveformPlotBounds();g.setColour(juce::Colour(0xff202936));g.fillRoundedRectangle(area.toFloat(),14);g.setColour(juce::Colour(0xff34485a));g.drawRoundedRectangle(area.toFloat(),14,1);g.setColour(juce::Colour(0xff111920));g.fillRect(0.0f,0.0f,plot.getX(),static_cast<float>(getHeight()));g.setColour(juce::Colour(0xff61747e));g.drawVerticalLine(juce::roundToInt(plot.getX()-1.0f),0.0f,static_cast<float>(getHeight()));
    const auto xAt=[plot,this](double seconds){return plot.getX()+static_cast<float>(seconds/duration)*plot.getWidth();};
    if(hasSelection()){const auto x1=xAt(selectionStart),x2=xAt(selectionEnd);g.setColour(juce::Colour(0x5531e0bd));g.fillRect(x1,0.0f,x2-x1,static_cast<float>(getHeight()));}
    const auto channelCount=static_cast<int>(waveform.size());
    if(channelCount>0){
        for(int channel=0;channel<channelCount;++channel){
            const auto lane=juce::Rectangle<float>(0.0f,static_cast<float>(channel)*getHeight()/channelCount,static_cast<float>(getWidth()),static_cast<float>(getHeight())/channelCount);const auto centre=lane.getCentreY(),amplitudeHeight=lane.getHeight()*.42f;static constexpr std::array<double,5> levels{0,-6,-12,-24,-48};g.setFont(juce::FontOptions(9.5f));for(const auto db:levels){const bool zeroLevel=db>-0.5;const auto amplitude=amplitudeForDbfs(db),upper=centre-amplitude*amplitudeHeight,lower=centre+amplitude*amplitudeHeight;g.setColour(juce::Colour(0xff57707c).withAlpha(zeroLevel?.7f:.38f));g.drawHorizontalLine(juce::roundToInt(upper),plot.getX(),plot.getRight());g.drawHorizontalLine(juce::roundToInt(lower),plot.getX(),plot.getRight());g.setColour(juce::Colour(0xffb4c2c8));g.drawText(juce::String(static_cast<int>(db)),5,juce::roundToInt(upper)-7,38,14,juce::Justification::centredRight);if(zeroLevel)g.drawText("0",5,juce::roundToInt(lower)-7,38,14,juce::Justification::centredRight);}g.setColour(juce::Colour(0xff71838c));g.drawText("-INF",5,juce::roundToInt(centre)-7,38,14,juce::Justification::centredRight);g.setColour(juce::Colour(0xff2b3a48));g.drawHorizontalLine(juce::roundToInt(centre),plot.getX(),plot.getRight());
            const auto& envelope=waveform[static_cast<std::size_t>(channel)];if(!envelope.empty()){juce::Path shape;const auto pixels=std::max(1,juce::roundToInt(plot.getWidth()));for(int pixel=0;pixel<=pixels;++pixel){const auto index=std::min(static_cast<std::size_t>(pixel)*envelope.size()/static_cast<std::size_t>(pixels),envelope.size()-1);const auto x=plot.getX()+static_cast<float>(pixel),y=centre-envelope[index].maximum*amplitudeHeight;if(pixel==0)shape.startNewSubPath(x,y);else shape.lineTo(x,y);}for(int pixel=pixels;pixel>=0;--pixel){const auto index=std::min(static_cast<std::size_t>(pixel)*envelope.size()/static_cast<std::size_t>(pixels),envelope.size()-1);shape.lineTo(plot.getX()+static_cast<float>(pixel),centre-envelope[index].minimum*amplitudeHeight);}shape.closeSubPath();g.saveState();g.reduceClipRegion(plot.toNearestInt());g.setColour(channel==0?juce::Colour(0xdd42dfc2):juce::Colour(0xdd45bde8));g.fillPath(shape);g.restoreState();}
            g.setColour(juce::Colour(0xbbd7e5e8));g.setFont(12.0f);const auto label=channelCount==1?"MONO":channel==0?"L":"R";g.drawText(label,juce::Rectangle<int>(getWidth()-58,juce::roundToInt(lane.getY())+7,45,18),juce::Justification::centredRight);
        }
        if(channelCount==2){g.setColour(juce::Colour(0xff526575));g.drawHorizontalLine(getHeight()/2,0.0f,static_cast<float>(getWidth()));}
    }
    if(duration>0){const auto x=xAt(playhead);g.setColour(juce::Colours::white);g.fillRect(x-1.0f,0.0f,2.0f,static_cast<float>(getHeight()));}
}
void MasteringWaveformView::mouseDown(const juce::MouseEvent&e){dragging=true;anchor=secondsAt(e.x);selectionStart=selectionEnd=anchor;if(onSeek)onSeek(anchor);repaint();}
void MasteringWaveformView::mouseDrag(const juce::MouseEvent&e){if(!dragging)return;const auto current=secondsAt(e.x);selectionStart=std::min(anchor,current);selectionEnd=std::max(anchor,current);repaint();}
void MasteringWaveformView::mouseUp(const juce::MouseEvent&e){if(!dragging)return;dragging=false;const auto current=secondsAt(e.x);selectionStart=std::min(anchor,current);selectionEnd=std::max(anchor,current);if(!hasSelection()){selectionStart=selectionEnd=0;if(onSeek)onSeek(current);}if(onSelectionChanged)onSelectionChanged(selectionStart,selectionEnd);repaint();}
