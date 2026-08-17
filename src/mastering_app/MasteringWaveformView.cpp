#include "MasteringWaveformView.h"

#include <algorithm>
#include <cmath>

void MasteringWaveformView::setWaveform(std::vector<float> value,double seconds){waveform=std::move(value);duration=std::max(0.0,seconds);playhead=0;clearSelection();repaint();}
void MasteringWaveformView::setPlayhead(double seconds){playhead=std::clamp(seconds,0.0,duration);repaint();}
void MasteringWaveformView::setHorizontalInset(float pixels){horizontalInset=std::max(0.0f,pixels);repaint();}
void MasteringWaveformView::setSelection(double start,double end){selectionStart=std::clamp(std::min(start,end),0.0,duration);selectionEnd=std::clamp(std::max(start,end),0.0,duration);repaint();}
void MasteringWaveformView::clearSelection(){selectionStart=selectionEnd=0;repaint();}
bool MasteringWaveformView::hasSelection()const{return duration>0&&selectionEnd-selectionStart>=0.02;}
double MasteringWaveformView::selectionStartSeconds()const{return selectionStart;}
double MasteringWaveformView::selectionEndSeconds()const{return selectionEnd;}
double MasteringWaveformView::secondsAt(int x)const{const auto travel=std::max(1.0f,static_cast<float>(getWidth())-2.0f*horizontalInset);return duration*std::clamp((static_cast<float>(x)-horizontalInset)/travel,0.0f,1.0f);}
void MasteringWaveformView::paint(juce::Graphics&g){auto area=getLocalBounds();g.setColour(juce::Colour(0xff202936));g.fillRoundedRectangle(area.toFloat(),14);g.setColour(juce::Colour(0xff34485a));g.drawRoundedRectangle(area.toFloat(),14,1);const auto travel=std::max(1.0f,static_cast<float>(getWidth())-2.0f*horizontalInset);const auto xAt=[this,travel](double seconds){return horizontalInset+static_cast<float>(seconds/duration)*travel;};if(hasSelection()){const auto x1=xAt(selectionStart),x2=xAt(selectionEnd);g.setColour(juce::Colour(0x5531e0bd));g.fillRect(x1,0.0f,x2-x1,static_cast<float>(getHeight()));}if(!waveform.empty()){juce::Path path;for(int x=0;x<getWidth();++x){const auto index=static_cast<std::size_t>(x)*waveform.size()/static_cast<std::size_t>(std::max(1,getWidth()));const auto y=area.getCentreY()-waveform[std::min(index,waveform.size()-1)]*area.getHeight()*.42f;if(x==0)path.startNewSubPath(0.0f,y);else path.lineTo(static_cast<float>(x),y);}g.setColour(juce::Colour(0xff42dfc2));g.strokePath(path,juce::PathStrokeType(1.7f));}if(duration>0){const auto x=xAt(playhead);g.setColour(juce::Colours::white);g.fillRect(x-1.0f,0.0f,2.0f,static_cast<float>(getHeight()));}}
void MasteringWaveformView::mouseDown(const juce::MouseEvent&e){dragging=true;anchor=secondsAt(e.x);selectionStart=selectionEnd=anchor;if(onSeek)onSeek(anchor);repaint();}
void MasteringWaveformView::mouseDrag(const juce::MouseEvent&e){if(!dragging)return;const auto current=secondsAt(e.x);selectionStart=std::min(anchor,current);selectionEnd=std::max(anchor,current);repaint();}
void MasteringWaveformView::mouseUp(const juce::MouseEvent&e){if(!dragging)return;dragging=false;const auto current=secondsAt(e.x);selectionStart=std::min(anchor,current);selectionEnd=std::max(anchor,current);if(!hasSelection()){selectionStart=selectionEnd=0;if(onSeek)onSeek(current);}if(onSelectionChanged)onSelectionChanged(selectionStart,selectionEnd);repaint();}
