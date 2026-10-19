#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
namespace {
constexpr double pi=3.14159265358979323846;
// Io, Europa, Ganymede, Callisto, Amalthea, Himalia. Mean periods in Earth days.
constexpr double periods[6]={1.769137786,3.551181,7.154553,16.6890184,0.498179,250.56};
constexpr double ecc[6]={0.0041,0.0094,0.0013,0.0074,0.003,0.16};
// The inner four and Amalthea are tidally locked; Himalia's spin ~7.8h.
constexpr double spins[6]={1.769137786,3.551181,7.154553,16.6890184,0.498179,0.325};
constexpr const char* names[6]={"Io","Europa","Ganymede","Callisto","Amalthea","Himalia"};
float raw(const juce::AudioProcessorValueTreeState& p,const juce::String& id){return p.getRawParameterValue(id)->load();}
double anomaly(double turns,double e){const double M=2*pi*(turns-std::floor(turns)); double E=M;for(int i=0;i<9;++i) E-=(E-e*std::sin(E)-M)/(1-e*std::cos(E));return 2*std::atan2(std::sqrt(1+e)*std::sin(E/2),std::sqrt(1-e)*std::cos(E/2));}
int value7(double v){return juce::jlimit(0,127,(int)std::lround(v));}
}
juce::AudioProcessorValueTreeState::ParameterLayout PlanetProcessor::makeParameters(){
 using F=juce::AudioParameterFloat;using B=juce::AudioParameterBool;using C=juce::AudioParameterChoice;using I=juce::AudioParameterInt;
 std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
 p.push_back(std::make_unique<C>(juce::ParameterID{"mode",1},"Pitch mapping",juce::StringArray{"True ratios","Octave folded","Compressed"},1));
 p.push_back(std::make_unique<F>(juce::ParameterID{"year",1},"One Io orbit (seconds)",juce::NormalisableRange<float>(0.5f,120.f,0.1f,0.5f),8.f));
 p.push_back(std::make_unique<F>(juce::ParameterID{"depth",1},"Orbit CC depth",0.f,1.f,0.8f));
 p.push_back(std::make_unique<F>(juce::ParameterID{"ellipse",1},"Ellipse exaggeration",1.f,3.f,1.f));
 p.push_back(std::make_unique<F>(juce::ParameterID{"gain",1},"Internal volume",0.f,0.3f,0.09f));
 p.push_back(std::make_unique<C>(juce::ParameterID{"wave",1},"Waveform",juce::StringArray{"Sine","Triangle","Soft saw"},0));
 p.push_back(std::make_unique<B>(juce::ParameterID{"audio",1},"Internal audio",true));
 p.push_back(std::make_unique<B>(juce::ParameterID{"midi",1},"MIDI note output",true));
 p.push_back(std::make_unique<B>(juce::ParameterID{"gate",1},"Four beats per orbit",true));
 p.push_back(std::make_unique<F>(juce::ParameterID{"duty",1},"Note gate length (%)",juce::NormalisableRange<float>(5.f,95.f,1.f),45.f));
 p.push_back(std::make_unique<B>(juce::ParameterID{"bend",1},"Microtonal pitch bend",false));
 p.push_back(std::make_unique<B>(juce::ParameterID{"orbitcc11",1},"Orbital distance CC enabled",true));
 p.push_back(std::make_unique<I>(juce::ParameterID{"distanceccnum",1},"Orbital distance CC number",1,119,11));
 p.push_back(std::make_unique<B>(juce::ParameterID{"orbitcc10",1},"Orbital position CC enabled",true));
 p.push_back(std::make_unique<I>(juce::ParameterID{"positionccnum",1},"Orbital position CC number",1,119,10));
 p.push_back(std::make_unique<B>(juce::ParameterID{"spincc",1},"Sidereal rotation CC enabled",true));
 p.push_back(std::make_unique<I>(juce::ParameterID{"spinccnum",1},"Sidereal rotation CC number",1,119,74));
 for(int i=0;i<6;++i){auto id=juce::String(i);p.push_back(std::make_unique<B>(juce::ParameterID{"on"+id,1},juce::String(names[i])+" enabled",true));p.push_back(std::make_unique<F>(juce::ParameterID{"vol"+id,1},juce::String(names[i])+" level",0.f,1.f,0.7f));p.push_back(std::make_unique<C>(juce::ParameterID{"ch"+id,1},juce::String(names[i])+" MIDI channel",juce::StringArray{"1","2","3","4","5","6","7","8","9","10","11","12","13","14","15","16"},i));}
 return {p.begin(),p.end()};
}
PlanetProcessor::PlanetProcessor():AudioProcessor(BusesProperties().withOutput("Output",juce::AudioChannelSet::stereo(),true)),parameters(*this,nullptr,"PARAMETERS",makeParameters()){activeNote.fill(-1);activeChannel.fill(1);lastDistance.fill(-1);lastPosition.fill(-1);lastRotation.fill(-1);}
void PlanetProcessor::prepareToPlay(double sr,int){sampleRateHz=sr;phase.fill(0);smoothLevel.fill(0);activeNote.fill(-1);activeChannel.fill(1);heldNotes.fill(false);noteOrder.fill(0);lastDistance.fill(-1);lastPosition.fill(-1);lastRotation.fill(-1);nextNoteOrder=0;referenceNote=-1;displayedNote.store(-1);ccElapsed=0;elapsedSeconds=0;orbitTime.store(0);}
void PlanetProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer& midi){
 juce::ScopedNoDenormals noDenormals;
 for(const auto metadata:midi){const auto msg=metadata.getMessage();if(msg.isNoteOn()){const int note=msg.getNoteNumber();heldNotes[(size_t)note]=true;noteOrder[(size_t)note]=++nextNoteOrder;}else if(msg.isNoteOff())heldNotes[(size_t)msg.getNoteNumber()]=false;else if(msg.isAllNotesOff()||msg.isAllSoundOff())heldNotes.fill(false);}
 referenceNote=-1;unsigned long long latest=0;for(int note=0;note<128;++note)if(heldNotes[(size_t)note]&&noteOrder[(size_t)note]>latest){latest=noteOrder[(size_t)note];referenceNote=note;}
 displayedNote.store(referenceNote);midi.clear();buffer.clear();const int n=buffer.getNumSamples();if(n<=0||sampleRateHz<=0)return;
 const double root=referenceNote>=0?440.0*std::pow(2.0,(referenceNote-69)/12.0):440.0;
 const double ioSeconds=raw(parameters,"year"),depth=raw(parameters,"depth"),exaggeration=raw(parameters,"ellipse"),gain=raw(parameters,"gain");
 const int mode=(int)raw(parameters,"mode"),wave=(int)raw(parameters,"wave");
 const bool audio=raw(parameters,"audio")>0.5f,sendMidi=raw(parameters,"midi")>0.5f,bend=raw(parameters,"bend")>0.5f,rhythm=raw(parameters,"gate")>0.5f;
 const bool distanceOn=raw(parameters,"orbitcc11")>0.5f,positionOn=raw(parameters,"orbitcc10")>0.5f,rotationOn=raw(parameters,"spincc")>0.5f;
 const int distanceCC=(int)raw(parameters,"distanceccnum"),positionCC=(int)raw(parameters,"positionccnum"),rotationCC=(int)raw(parameters,"spinccnum");
 const double duty=raw(parameters,"duty")/100.0;
 ccElapsed+=(double)n/sampleRateHz;const bool emitCC=ccElapsed>=0.02;if(emitCC)ccElapsed=std::fmod(ccElapsed,0.02);
 std::array<double,6> freq{},amp{},pan{};
 for(int i=0;i<6;++i){const auto id=juce::String(i);const bool enabled=raw(parameters,"on"+id)>0.5f;const double level=raw(parameters,"vol"+id);
  const double ratio=periods[0]/periods[i];double f=root*ratio;if(mode==1){while(f<90)f*=2;while(f>1200)f*=0.5;}if(mode==2)f=root*std::pow(ratio,0.4);
  const double e=juce::jmin(.8,ecc[i]*exaggeration);const double turns=elapsedSeconds/ioSeconds*ratio;const double theta=anomaly(turns,e);
  const double distance=(1-e*e)/(1+e*std::cos(theta));const double proximity=juce::jlimit(0.,1.,(1+e-distance)/(2*juce::jmax(.001,e)));
  const double beat=4*turns-std::floor(4*turns);const bool pulse=!rhythm||beat<duty;
  freq[i]=f;amp[i]=(enabled&&referenceNote>=0&&pulse?level:0);pan[i]=depth*.7*std::sin(theta);
  const int channel=1+(int)raw(parameters,"ch"+id);
  const int note=juce::jlimit(0,127,(int)std::lround(69+12*std::log2(f/440.0)));
  const bool wants=sendMidi&&enabled&&referenceNote>=0&&pulse;
  if(activeNote[i]>=0&&(!wants||activeNote[i]!=note||activeChannel[i]!=channel)){midi.addEvent(juce::MidiMessage::noteOff(activeChannel[i],activeNote[i]),0);activeNote[i]=-1;}
  if(sendMidi&&enabled&&referenceNote>=0&&emitCC){
   const int dist=value7(127*(1-depth+depth*proximity));
   const int pos=value7(64+63*depth*std::sin(theta));
   // Sidereal rotation: positive spin periods, one CC sine cycle per rotation.
   const double rotationTurns=elapsedSeconds/ioSeconds*(periods[0]/spins[i]);
   const int rot=value7(63.5+63.5*std::sin(2*pi*rotationTurns));
   if(distanceOn&&dist!=lastDistance[i]){midi.addEvent(juce::MidiMessage::controllerEvent(channel,distanceCC,dist),0);lastDistance[i]=dist;}
   if(positionOn&&pos!=lastPosition[i]){midi.addEvent(juce::MidiMessage::controllerEvent(channel,positionCC,pos),0);lastPosition[i]=pos;}
   if(rotationOn&&rot!=lastRotation[i]){midi.addEvent(juce::MidiMessage::controllerEvent(channel,rotationCC,rot),0);lastRotation[i]=rot;}
  }
  if(!distanceOn||!sendMidi||!enabled||referenceNote<0)lastDistance[i]=-1;
  if(!positionOn||!sendMidi||!enabled||referenceNote<0)lastPosition[i]=-1;
  if(!rotationOn||!sendMidi||!enabled||referenceNote<0)lastRotation[i]=-1;
  if(wants&&activeNote[i]<0){if(bend){const double semis=69+12*std::log2(f/440.0);const int pb=juce::jlimit(0,16383,(int)std::lround(8192+(semis-note)*4096));midi.addEvent(juce::MidiMessage::pitchWheel(channel,pb),0);}midi.addEvent(juce::MidiMessage::noteOn(channel,note,(juce::uint8)96),0);activeNote[i]=note;activeChannel[i]=channel;}
 }
 if(audio){for(int s=0;s<n;++s){float left=0,right=0;for(int i=0;i<6;++i){smoothLevel[i]+=0.001*(amp[i]-smoothLevel[i]);phase[i]+=freq[i]/sampleRateHz;phase[i]-=std::floor(phase[i]);const double t=phase[i];const double osc=wave==0?std::sin(2*pi*t):(wave==1?4*std::abs(t-.5)-1:.7*(2*t-1));const double v=osc*smoothLevel[i]*gain;left+=(float)(v*std::sqrt((1-pan[i])*.5));right+=(float)(v*std::sqrt((1+pan[i])*.5));}if(buffer.getNumChannels()>0)buffer.setSample(0,s,juce::jlimit(-1.f,1.f,left));if(buffer.getNumChannels()>1)buffer.setSample(1,s,juce::jlimit(-1.f,1.f,right));}}
 elapsedSeconds+=(double)n/sampleRateHz;orbitTime.store(elapsedSeconds);
}
juce::AudioProcessorEditor* PlanetProcessor::createEditor(){return new PlanetEditor(*this);}
void PlanetProcessor::getStateInformation(juce::MemoryBlock& b){if(auto xml=parameters.copyState().createXml())copyXmlToBinary(*xml,b);}
void PlanetProcessor::setStateInformation(const void* data,int size){if(auto xml=getXmlFromBinary(data,size))if(xml->hasTagName(parameters.state.getType()))parameters.replaceState(juce::ValueTree::fromXml(*xml));}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new PlanetProcessor();}
