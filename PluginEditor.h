#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include "PluginProcessor.h"

static const juce::Colour kBg      { 0xff0d0f14 };
static const juce::Colour kSurface { 0xff13161c };
static const juce::Colour kBorder  { 0xff1e2128 };
static const juce::Colour kGreen   { 0xff00e5a0 };
static const juce::Colour kAmber   { 0xffffb800 };
static const juce::Colour kRed     { 0xffff2a2a };
static const juce::Colour kBlue    { 0xff4da6ff };
static const juce::Colour kPurple  { 0xffc084fc };
static const juce::Colour kText    { 0xffcccccc };
static const juce::Colour kSubText { 0xff666666 };

// ── Custom LookAndFeel ────────────────────────────────────────
class LumetrixLookAndFeel : public juce::LookAndFeel_V4
{
public:
    LumetrixLookAndFeel();
    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h,
                          float sliderPos, float startAngle, float endAngle,
                          juce::Slider&) override;
    void drawLinearSlider(juce::Graphics&, int x, int y, int w, int h,
                          float sliderPos, float minPos, float maxPos,
                          juce::Slider::SliderStyle, juce::Slider&) override;
};

// ── VU bar ────────────────────────────────────────────────────
class VUBar : public juce::Component
{
public:
    void setLevel(float dbLevel, float holdDb);
    void paint(juce::Graphics&) override;
private:
    float level = -70.f, holdDb = -70.f;
};

// ── Horizontal LUFS bar ───────────────────────────────────────
class LUFSBar : public juce::Component
{
public:
    LUFSBar(const juce::String& label, juce::Colour color);
    void setValue(float lufs);
    void paint(juce::Graphics&) override;
private:
    juce::String label;
    juce::Colour color;
    float value = -70.f;
};

// ── Numeric readout ───────────────────────────────────────────
class ValueReadout : public juce::Component
{
public:
    ValueReadout(const juce::String& label, const juce::String& unit, juce::Colour c);
    void setValue(float v, bool showPlus = false);
    void paint(juce::Graphics&) override;
private:
    juce::String label, unit;
    juce::Colour color;
    float value = -70.f;
    bool  plus  = false;
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
    LumetrixLookAndFeel     laf;

    VUBar   vuL, vuR;
    LUFSBar lufsIntBar  { "INTEGRATED", kGreen  };
    LUFSBar lufsSTPBar  { "SHORT TERM", kBlue   };
    LUFSBar lufsMomBar  { "MOMENTARY",  kPurple };

    ValueReadout readPeak { "PEAK",      "dBTP",  kAmber  };
    ValueReadout readRMS  { "RMS",       "dBRMS", kBlue   };
    ValueReadout readLUFS { "LUFS INT",  "LUFS",  kGreen  };
    ValueReadout readDyn  { "DYN RANGE", "LU",    kPurple };

    juce::Slider gainKnob;
    juce::Label  gainLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;

    juce::TextButton resetBtn { "RESET" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LumetrixAudioProcessorEditor)
};
