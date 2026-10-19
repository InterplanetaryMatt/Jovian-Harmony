#include "PluginEditor.h"
#include <BinaryData.h>
#include <cmath>
namespace {
constexpr double pi=3.14159265358979323846;
constexpr double periods[6]={1.769137786,3.551181,7.154553,16.6890184,0.498179,250.56};
constexpr double eccentricity[6]={.0041,.0094,.0013,.0074,.003,.16};
constexpr const char* names[6]={"Io","Europa","Ganymede","Callisto","Amalthea","Himalia"};
const juce::Colour colours[6]={juce::Colour(0xffffc86a),juce::Colour(0xffd0b992),juce::Colour(0xff8db9db),juce::Colour(0xff929fae),juce::Colour(0xffff805f),juce::Colour(0xffbc93e5)};
double raw(const PlanetProcessor& p,const juce::String& id){return p.parameters.getRawParameterValue(id)->load();}
double anomaly(double turns,double e){const double M=2*pi*(turns-std::floor(turns));double E=M;for(int j=0;j<9;++j)E-=(E-e*std::sin(E)-M)/(1-e*std::cos(E));return E;}
}
void OrbitDisplay::paint(juce::Graphics& g){
 auto b=getLocalBounds().toFloat().reduced(8);g.setColour(juce::Colour(0xff0c1526));g.fillRoundedRectangle(b,15);
 const float cx=b.getCentreX(),cy=b.getCentreY()+8,maxR=juce::jmin(b.getWidth(),b.getHeight())*.455f;
 g.setColour(juce::Colour(0xffe8a968).withAlpha(.16f));g.fillEllipse(cx-57,cy-57,114,114);
 g.setColour(juce::Colour(0xffd9b28a));g.fillEllipse(cx-44,cy-44,88,88);
 g.setColour(juce::Colour(0xff876044));for(int j=0;j<5;++j)g.fillRect(cx-42,cy-29+j*13,84.f,3.f);
 const double sec=juce::jmax(.5,raw(processor,"year")),time=processor.getOrbitTime(),ex=raw(processor,"ellipse");
 // Orbit radii are compressed for legibility, not proportional to distance.
 constexpr int order[6]={4,0,1,2,3,5};
 for(int k=5;k>=0;--k){const int i=order[k];const float a=maxR*(.34f+.128f*k);const double e=juce::jmin(.75,eccentricity[i]*ex);const float bb=a*(float)std::sqrt(1-e*e),focus=cx+(float)(a*e);
  g.setColour(colours[i].withAlpha(.35f));g.drawEllipse(focus-a,cy-bb,2*a,2*bb,1.3f);
  const double E=anomaly(time/sec*periods[0]/periods[i],e);
  const float x=focus-a*(float)std::cos(E),y=cy-bb*(float)std::sin(E);const bool active=raw(processor,"on"+juce::String(i))>.5;
  const float r=(i==2||i==3)?6.f:4.6f;g.setColour((active?colours[i]:juce::Colours::grey).withAlpha(.22f));g.fillEllipse(x-r*2,y-r*2,4*r,4*r);g.setColour(active?colours[i]:juce::Colours::grey);g.fillEllipse(x-r,y-r,2*r,2*r);
 }
 g.setColour(juce::Colour(0xffc2ddff));g.setFont(14.f);const int note=processor.getReferenceMidiNote();
 const juce::String status=note<0?"PLAY A MIDI NOTE TO START":"REFERENCE: "+juce::MidiMessage::getMidiNoteName(note,true,true,3)+" / "+juce::String(juce::MidiMessage::getMidiNoteInHertz(note),1)+" Hz";
 g.drawFittedText(status,getLocalBounds().removeFromTop(35),juce::Justification::centred,1);
 g.setColour(juce::Colours::white.withAlpha(.6f));g.setFont(12.f);
 g.drawFittedText("Jupiter is the reference - Io plays the input pitch",getLocalBounds().removeFromBottom(30),juce::Justification::centred,1);
}
PlanetEditor::PlanetEditor(PlanetProcessor& p):AudioProcessorEditor(p),processor(p),orbits(p){
 setOpaque(true);setSize(1150,800);logo=juce::ImageCache::getFromMemory(BinaryData::interplanetary_logo_jpg,BinaryData::interplanetary_logo_jpgSize);
 addAndMakeVisible(orbits);
 title.setText("JOVIAN HARMONY",juce::dontSendNotification);title.setFont(juce::Font(25.f,juce::Font::bold));title.setColour(juce::Label::textColourId,juce::Colours::white);addAndMakeVisible(title);
 subtitle.setText("SIX MOONS OF JUPITER  |  PLAY A KEY TO START  |  RELEASE TO STOP",juce::dontSendNotification);subtitle.setColour(juce::Label::textColourId,juce::Colour(0xff9db9d7));addAndMakeVisible(subtitle);
 auto& state=p.parameters;
 auto choice=[&](juce::ComboBox& c,const juce::String& id,const juce::StringArray& labels){for(int i=0;i<labels.size();++i)c.addItem(labels[i],i+1);addAndMakeVisible(c);choices.push_back(std::make_unique<CA>(state,id,c));};
 choice(mode,"mode",{"True ratios","Octave folded","Compressed"});choice(wave,"wave",{"Sine","Triangle","Soft saw"});
 auto slider=[&](juce::Slider& s,const juce::String& id){s.setSliderStyle(juce::Slider::LinearHorizontal);s.setTextBoxStyle(juce::Slider::TextBoxRight,false,60,22);addAndMakeVisible(s);sliders.push_back(std::make_unique<SA>(state,id,s));};
 slider(year,"year");slider(depth,"depth");slider(ellipse,"ellipse");slider(gain,"gain");slider(duty,"duty");slider(distanceNum,"distanceccnum");slider(positionNum,"positionccnum");slider(rotationNum,"spinccnum");
 auto toggle=[&](juce::ToggleButton& b,const juce::String& id,const juce::String& label){b.setButtonText(label);addAndMakeVisible(b);buttons.push_back(std::make_unique<BA>(state,id,b));};
 toggle(audio,"audio","Internal audio");toggle(midi,"midi","MIDI notes out");toggle(bend,"bend","Pitch bend");toggle(gate,"gate","4 beats per orbit");
 toggle(distanceOn,"orbitcc11","Orbital distance");toggle(positionOn,"orbitcc10","Orbital position");toggle(rotationOn,"spincc","Moon rotation");
 for(int i=0;i<6;++i){auto id=juce::String(i);moonNames[i].setText(names[i],juce::dontSendNotification);moonNames[i].setColour(juce::Label::textColourId,colours[i]);addAndMakeVisible(moonNames[i]);toggle(enabled[i],"on"+id,"On");choice(channels[i],"ch"+id,{"1","2","3","4","5","6","7","8","9","10","11","12","13","14","15","16"});slider(levels[i],"vol"+id);}
}
void PlanetEditor::paint(juce::Graphics& g){
 g.fillAll(juce::Colour(0xff070d18));if(logo.isValid())g.drawImageWithin(logo,1035,6,100,100,juce::RectanglePlacement::centred);
 g.setColour(juce::Colour(0xff9db9d7));g.setFont(14.f);
 g.drawText("MOON",592,123,125,22,juce::Justification::left);g.drawText("ON",727,123,50,22,juce::Justification::left);g.drawText("MIDI CH",805,123,120,22,juce::Justification::left);g.drawText("LEVEL",946,123,120,22,juce::Justification::left);
 g.drawText("Orbital modulation - independent CC number for each parameter",590,473,550,24,juce::Justification::left);
 g.drawText("CC NUMBER",958,505,160,22,juce::Justification::left);
 g.drawText("Io orbit (seconds)",22,612,220,23,juce::Justification::left);
 g.drawText("Orbit CC depth",22,655,220,23,juce::Justification::left);
 g.drawText("Ellipse exaggeration",22,698,220,23,juce::Justification::left);
 g.drawText("Internal volume",592,655,220,23,juce::Justification::left);
 g.drawText("Gate length (%)",592,698,220,23,juce::Justification::left);
 g.setColour(juce::Colour(0xff657995));g.setFont(12.f);
 g.drawText("Moon rotations use sidereal periods; most Galilean moons are tidally locked.",592,758,550,20,juce::Justification::left);
}
void PlanetEditor::resized(){
 title.setBounds(20,12,600,37);subtitle.setBounds(21,52,725,26);orbits.setBounds(10,105,567,480);
 mode.setBounds(20,586,267,29);wave.setBounds(300,586,255,29);
 audio.setBounds(25,742,137,28);midi.setBounds(170,742,149,28);bend.setBounds(325,742,125,28);gate.setBounds(453,742,150,28);
 year.setBounds(216,612,341,29);depth.setBounds(216,655,341,29);ellipse.setBounds(216,698,341,29);
 gain.setBounds(766,655,350,29);duty.setBounds(766,698,350,29);
 for(int i=0;i<6;++i){int y=154+i*50;moonNames[i].setBounds(592,y,129,29);enabled[i].setBounds(726,y,66,29);channels[i].setBounds(803,y,108,28);levels[i].setBounds(928,y,190,29);}
 distanceOn.setBounds(596,508,275,28);positionOn.setBounds(596,554,275,28);rotationOn.setBounds(596,600,275,28);
 distanceNum.setBounds(886,507,231,29);positionNum.setBounds(886,553,231,29);rotationNum.setBounds(886,599,231,29);
}
