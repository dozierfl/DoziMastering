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
double MasteringWaveformView::secondsAt(int x)const{const auto travel=std::max(1.0f,static_cast<float>(getWidth())-2.0f*horizontalInset);return duration*std::clamp((static_cast<float>(x)-horizontalInset)/travel,0.0f,1.0f);}
void MasteringWaveformView::paint(juce::Graphics&g){
    const auto area=getLocalBounds();g.setColour(juce::Colour(0xff202936));g.fillRoundedRectangle(area.toFloat(),14);g.setColour(juce::Colour(0xff34485a));g.drawRoundedRectangle(area.toFloat(),14,1);
    const auto travel=std::max(1.0f,static_cast<float>(getWidth())-2.0f*horizontalInset);const auto xAt=[this,travel](double seconds){return horizontalInset+static_cast<float>(seconds/duration)*travel;};
    if(hasSelection()){const auto x1=xAt(selectionStart),x2=xAt(selectionEnd);g.setColour(juce::Colour(0x5531e0bd));g.fillRect(x1,0.0f,x2-x1,static_cast<float>(getHeight()));}
    const auto channelCount=static_cast<int>(waveform.size());
    if(channelCount>0){
        for(int channel=0;channel<channelCount;++channel){
            const auto lane=juce::Rectangle<float>(0.0f,static_cast<float>(channel)*getHeight()/channelCount,static_cast<float>(getWidth()),static_cast<float>(getHeight())/channelCount);const auto centre=lane.getCentreY(),amplitudeHeight=lane.getHeight()*.42f;static constexpr std::array<double,5> levels{0,-6,-12,-24,-48};g.setFont(juce::FontOptions(9.5f));for(const auto db:levels){const auto zeroLevel=static_cast<float>(db>-0.5),amplitude=amplitudeForDbfs(db),upper=centre-amplitude*amplitudeHeight,lower=centre+amplitude*amplitudeHeight;g.setColour(juce::Colour(0xff57707c).withAlpha(zeroLevel>.5f?.7f:.38f));g.drawHorizontalLine(juce::roundToInt(upper),42.0f,getWidth()-horizontalInset);g.drawHorizontalLine(juce::roundToInt(lower),42.0f,getWidth()-horizontalInset);g.setColour(juce::Colour(0xffb4c2c8));g.drawText(juce::String(static_cast<int>(db)),2,juce::roundToInt(upper)-7,35,14,juce::Justification::centredRight);if(zeroLevel>.5f)g.drawText("0",2,juce::roundToInt(lower)-7,35,14,juce::Justification::centredRight);}g.setColour(juce::Colour(0xff71838c));g.drawText("-INF",2,juce::roundToInt(centre)-7,35,14,juce::Justification::centredRight);g.setColour(juce::Colour(0xff2b3a48));g.drawHorizontalLine(juce::roundToInt(centre),42.0f,getWidth()-horizontalInset);g.setColour(juce::Colour(0xff71838c));g.drawVerticalLine(40,lane.getY()+5,lane.getBottom()-5);
            const auto& envelope=waveform[static_cast<std::size_t>(channel)];if(!envelope.empty()){juce::Path shape;for(int x=0;x<getWidth();++x){const auto index=std::min(static_cast<std::size_t>(x)*envelope.size()/static_cast<std::size_t>(std::max(1,getWidth())),envelope.size()-1);const auto y=centre-envelope[index].maximum*lane.getHeight()*.42f;if(x==0)shape.startNewSubPath(0.0f,y);else shape.lineTo(static_cast<float>(x),y);}for(int x=getWidth()-1;x>=0;--x){const auto index=std::min(static_cast<std::size_t>(x)*envelope.size()/static_cast<std::size_t>(std::max(1,getWidth())),envelope.size()-1);shape.lineTo(static_cast<float>(x),centre-envelope[index].minimum*lane.getHeight()*.42f);}shape.closeSubPath();g.setColour(channel==0?juce::Colour(0xdd42dfc2):juce::Colour(0xdd45bde8));g.fillPath(shape);}
            g.setColour(juce::Colour(0xbbd7e5e8));g.setFont(12.0f);const auto label=channelCount==1?"MONO":channel==0?"L":"R";g.drawText(label,juce::Rectangle<int>(getWidth()-58,juce::roundToInt(lane.getY())+7,45,18),juce::Justification::centredRight);
        }
        if(channelCount==2){g.setColour(juce::Colour(0xff526575));g.drawHorizontalLine(getHeight()/2,0.0f,static_cast<float>(getWidth()));}
    }
    if(duration>0){const auto x=xAt(playhead);g.setColour(juce::Colours::white);g.fillRect(x-1.0f,0.0f,2.0f,static_cast<float>(getHeight()));}
}
void MasteringWaveformView::mouseDown(const juce::MouseEvent&e){dragging=true;anchor=secondsAt(e.x);selectionStart=selectionEnd=anchor;if(onSeek)onSeek(anchor);repaint();}
void MasteringWaveformView::mouseDrag(const juce::MouseEvent&e){if(!dragging)return;const auto current=secondsAt(e.x);selectionStart=std::min(anchor,current);selectionEnd=std::max(anchor,current);repaint();}
void MasteringWaveformView::mouseUp(const juce::MouseEvent&e){if(!dragging)return;dragging=false;const auto current=secondsAt(e.x);selectionStart=std::min(anchor,current);selectionEnd=std::max(anchor,current);if(!hasSelection()){selectionStart=selectionEnd=0;if(onSeek)onSeek(current);}if(onSelectionChanged)onSelectionChanged(selectionStart,selectionEnd);repaint();}
