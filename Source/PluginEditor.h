#pragma once
#include "PluginProcessor.h"
#include <array>
#include <memory>
#include <vector>
class OrbitDisplay : public juce::Component, private juce::Timer {
public: explicit OrbitDisplay(PlanetProcessor& p):processor(p){startTimerHz(30);} void paint(juce::Graphics&) override;
private: void timerCallback() override{repaint();} PlanetProcessor& processor;
};
class PlanetEditor : public juce::AudioProcessorEditor {
public: explicit PlanetEditor(PlanetProcessor&);void paint(juce::Graphics&) override;void resized() override;
private:
 using CA=juce::AudioProcessorValueTreeState::ComboBoxAttachment;
 using SA=juce::AudioProcessorValueTreeState::SliderAttachment;
 using BA=juce::AudioProcessorValueTreeState::ButtonAttachment;
 PlanetProcessor& processor;OrbitDisplay orbits;juce::Image logo;
 juce::Label title,subtitle;
 juce::ComboBox mode,wave;
 juce::Slider year,depth,ellipse,gain,duty,distanceNum,positionNum,rotationNum;
 juce::ToggleButton audio,midi,bend,gate,distanceOn,positionOn,rotationOn;
 std::array<juce::Label,6> moonNames;
 std::array<juce::ToggleButton,6> enabled;
 std::array<juce::ComboBox,6> channels;
 std::array<juce::Slider,6> levels;
 std::vector<std::unique_ptr<CA>> choices;
 std::vector<std::unique_ptr<SA>> sliders;
 std::vector<std::unique_ptr<BA>> buttons;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlanetEditor)
};
