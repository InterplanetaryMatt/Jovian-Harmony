#pragma once
#include <JuceHeader.h>
#include <array>
class PlanetProcessor final : public juce::AudioProcessor {
public:
 PlanetProcessor();
 void prepareToPlay(double,int) override;
 void releaseResources() override {}
 void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
 juce::AudioProcessorEditor* createEditor() override;
 bool hasEditor() const override { return true; }
 const juce::String getName() const override { return "Jovian Harmony"; }
 bool acceptsMidi() const override { return true; }
 bool producesMidi() const override { return true; }
 bool isMidiEffect() const override { return false; }
 double getTailLengthSeconds() const override { return 0; }
 int getNumPrograms() override { return 1; }
 int getCurrentProgram() override { return 0; }
 void setCurrentProgram(int) override {}
 const juce::String getProgramName(int) override { return {}; }
 void changeProgramName(int,const juce::String&) override {}
 void getStateInformation(juce::MemoryBlock&) override;
 void setStateInformation(const void*,int) override;
 bool isBusesLayoutSupported(const BusesLayout& l) const override { return l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo(); }
 juce::AudioProcessorValueTreeState parameters;
 double getOrbitTime() const noexcept { return orbitTime.load(); }
 int getReferenceMidiNote() const noexcept { return displayedNote.load(); }
private:
 static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters();
 std::array<double,6> phase{};
 std::array<double,6> smoothLevel{};
 std::array<int,6> activeNote{},activeChannel{},lastDistance{},lastPosition{},lastRotation{};
 std::array<bool,128> heldNotes{};
 std::array<unsigned long long,128> noteOrder{};
 unsigned long long nextNoteOrder=0;
 int referenceNote=-1;
 double elapsedSeconds=0,sampleRateHz=44100,ccElapsed=0;
 std::atomic<double> orbitTime{0};
 std::atomic<int> displayedNote{-1};
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlanetProcessor)
};
