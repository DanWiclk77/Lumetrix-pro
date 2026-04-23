// ============================================================
//  LUMETRIX PRO – PluginEditor.h
// ============================================================
#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// ── Custom look and feel ──────────────────────────────────────
class LumetrixLookAndFeel : public juce::LookAndFeel_V4
{
public:
    LumetrixLookAndFeel();

    void drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h,
                          float sliderPos, float minPos, float maxPos,
                          juce::Slider::SliderStyle style,
                          juce::Slider& slider) override;

    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                          float sliderPos, float startAngle, float endAngle,
                          juce::Slider& slider) override;
};

// ── VU bar component ──────────────────────────────────────────
class VUBar : public juce::Component
{
public:
    void setLevel(float dbLevel, float holdDb);
    void paint(juce::Graphics& g) override;

    float warnDb  = -6.f;
    float clipDb  =  0.f;

private:
    float level  = -70.f;
    float holdDb = -70.f;
};

// ── Horizontal LUFS bar ───────────────────────────────────────
class LUFSBar : public juce::Component
{
public:
    LUFSBar(const juce::String& label, juce::Colour barColor);
    void setValue(float lufs);
    void paint(juce::Graphics& g) override;

private:
    juce::String  label;
    juce::Colour  color;
    float value = -70.f;
    float min   = -40.f;
    float max   =   0.f;
};

// ── Numeric readout ───────────────────────────────────────────
class ValueReadout : public juce::Component
{
public:
    ValueReadout(const juce::String& label, const juce::String& unit, juce::Colour c);
    void setValue(float v, bool showPlus = false);
    void paint(juce::Graphics& g) override;

private:
    juce::String label, unit;
    juce::Colour color;
    float value = -70.f;
    bool plus   = false;
};

// ── Main editor ───────────────────────────────────────────────
class LumetrixAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    explicit LumetrixAudioProcessorEditor(LumetrixAudioProcessor&);
    ~LumetrixAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    LumetrixAudioProcessor& audioProcessor;
    LumetrixLookAndFeel     lookAndFeel;

    // Meters
    VUBar   vuL, vuR;
    LUFSBar lufsIntBar  { "INTEGRATED",  juce::Colour(0xff00e5a0) };
    LUFSBar lufsSTPBar  { "SHORT TERM",  juce::Colour(0xff4da6ff) };
    LUFSBar lufsMomBar  { "MOMENTARY",   juce::Colour(0xffc084fc) };

    // Readouts
    ValueReadout readPeak    { "PEAK",      "dBTP",   juce::Colour(0xffffb800) };
    ValueReadout readRMS     { "RMS",       "dBRMS",  juce::Colour(0xff4da6ff) };
    ValueReadout readLUFS    { "LUFS INT",  "LUFS",   juce::Colour(0xff00e5a0) };
    ValueReadout readDyn     { "DYN RANGE", "LU",     juce::Colour(0xffc084fc) };

    // Gain knob + label
    juce::Slider gainKnob;
    juce::Label  gainLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;

    // Reset button
    juce::TextButton resetBtn { "RESET" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LumetrixAudioProcessorEditor)
};
