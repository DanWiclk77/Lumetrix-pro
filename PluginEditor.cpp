#include "PluginEditor.h"

// ── LookAndFeel ───────────────────────────────────────────────
LumetrixLookAndFeel::LumetrixLookAndFeel()
{
    setColour(juce::Slider::thumbColourId,      kGreen);
    setColour(juce::Slider::trackColourId,      kBorder);
    setColour(juce::Slider::backgroundColourId, kSurface);
    setColour(juce::TextButton::buttonColourId, kBorder);
    setColour(juce::TextButton::textColourOffId,kSubText);
}

void LumetrixLookAndFeel::drawLinearSlider(juce::Graphics& g,
    int x, int y, int w, int h,
    float sliderPos, float, float,
    juce::Slider::SliderStyle, juce::Slider&)
{
    juce::Rectangle<float> track((float)x, y + h*0.5f - 2.f, (float)w, 4.f);
    g.setColour(kBorder);
    g.fillRoundedRectangle(track, 2.f);

    float centre = x + w * 0.5f;
    juce::Colour fillColor = sliderPos < centre ? kBlue : kAmber;
    if (std::abs(sliderPos - centre) < 2.f) fillColor = kGreen;

    float left  = std::min(sliderPos, centre);
    float right = std::max(sliderPos, centre);
    g.setColour(fillColor);
    g.fillRoundedRectangle(left, y + h*0.5f - 2.f, right - left, 4.f, 2.f);

    g.setColour(fillColor);
    g.fillEllipse(sliderPos - 7.f, y + h*0.5f - 7.f, 14.f, 14.f);
    g.setColour(kBg.withAlpha(0.6f));
    g.fillEllipse(sliderPos - 4.f, y + h*0.5f - 4.f, 8.f, 8.f);
}

void LumetrixLookAndFeel::drawRotarySlider(juce::Graphics& g,
    int x, int y, int w, int h,
    float sliderPos, float startAngle, float endAngle, juce::Slider& slider)
{
    float radius  = std::min(w, h) * 0.5f - 6.f;
    float centreX = x + w * 0.5f;
    float centreY = y + h * 0.5f;
    float angle   = startAngle + sliderPos * (endAngle - startAngle);

    float gainDb  = (float)slider.getValue();
    juce::Colour color = gainDb > 0.5f ? kAmber : gainDb < -0.5f ? kBlue : kGreen;

    // Background arc
    juce::Path bgArc;
    bgArc.addArc(centreX - radius, centreY - radius, radius*2, radius*2,
                 startAngle, endAngle, true);
    g.setColour(kBorder);
    g.strokePath(bgArc, juce::PathStrokeType(4.f, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));

    // Foreground arc
    float midAngle = startAngle + 0.5f * (endAngle - startAngle);
    float arcStart = std::min(midAngle, angle);
    float arcEnd   = std::max(midAngle, angle);
    if (arcEnd > arcStart + 0.01f)
    {
        juce::Path fgArc;
        fgArc.addArc(centreX - radius, centreY - radius, radius*2, radius*2,
                     arcStart, arcEnd, true);
        g.setColour(color);
        g.strokePath(fgArc, juce::PathStrokeType(4.f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
    }

    // Knob body
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff3a3a3a), centreX - radius*0.3f, centreY - radius*0.3f,
        juce::Colour(0xff111111), centreX + radius*0.3f, centreY + radius*0.5f, false));
    g.fillEllipse(centreX - radius*0.75f, centreY - radius*0.75f, radius*1.5f, radius*1.5f);

    // Pointer
    juce::Path pointer;
    float pLen = radius * 0.55f;
    pointer.startNewSubPath(0.f, -radius * 0.25f);
    pointer.lineTo(0.f, -pLen);
    g.setColour(color);
    g.strokePath(pointer,
        juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
        juce::AffineTransform::rotation(angle).translated(centreX, centreY));
}

// ── VUBar ─────────────────────────────────────────────────────
void VUBar::setLevel(float db, float hold) { level = db; holdDb = hold; repaint(); }

void VUBar::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const float minDb = -60.f, maxDb = 6.f;
    auto dbToY = [&](float db) {
        float t = juce::jlimit(0.f, 1.f, (db - minDb) / (maxDb - minDb));
        return b.getHeight() * (1.f - t);
    };

    g.setColour(kSurface);
    g.fillRoundedRectangle(b, 3.f);
    g.setColour(kBorder);
    g.drawRoundedRectangle(b, 3.f, 1.f);

    float fillTop = dbToY(level);
    juce::ColourGradient grad(kGreen, b.getCentreX(), b.getBottom(),
                               kRed,  b.getCentreX(), 0.f, false);
    grad.addColour(0.7, kAmber);
    g.setGradientFill(grad);
    g.fillRoundedRectangle(b.getX()+2, fillTop, b.getWidth()-4,
                            b.getBottom() - fillTop - 2, 2.f);

    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.fillRect(b.getX()+2, dbToY(holdDb)-1.f, b.getWidth()-4, 2.f);

    for (float db : { -48.f, -36.f, -24.f, -18.f, -12.f, -6.f, 0.f })
    {
        float my = dbToY(db);
        g.setColour(db == 0.f ? kRed.withAlpha(0.4f) : kBorder);
        g.fillRect(b.getX(), my, b.getWidth(), 1.f);
    }
}

// ── LUFSBar ───────────────────────────────────────────────────
LUFSBar::LUFSBar(const juce::String& l, juce::Colour c) : label(l), color(c) {}

void LUFSBar::setValue(float lufs) { value = lufs; repaint(); }

void LUFSBar::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const float min = -40.f, max = 0.f;
    float fillPct = juce::jlimit(0.f, 1.f, (value - min) / (max - min));

    g.setFont(juce::Font("Courier New", 9.f, juce::Font::plain));
    g.setColour(kSubText);
    g.drawText(label, b.withWidth(90), juce::Justification::centredLeft);

    g.setFont(juce::Font("Courier New", 12.f, juce::Font::bold));
    g.setColour(color);
    juce::String valStr = (value >= 0 ? "+" : "") + juce::String(value, 1) + " LUFS";
    g.drawText(valStr, b.withLeft(b.getWidth()-80), juce::Justification::centredRight);

    juce::Rectangle<float> track(90.f, b.getCentreY()-5.f, b.getWidth()-175.f, 10.f);
    g.setColour(kSurface);
    g.fillRoundedRectangle(track, 4.f);
    g.setColour(kBorder);
    g.drawRoundedRectangle(track, 4.f, 1.f);

    g.setGradientFill(juce::ColourGradient(
        color.withAlpha(0.5f), track.getX(), 0.f,
        color, track.getRight(), 0.f, false));
    g.fillRoundedRectangle(track.getX(), track.getY(),
                            track.getWidth() * fillPct, track.getHeight(), 4.f);

    auto markX = [&](float db) {
        return track.getX() + track.getWidth() * juce::jlimit(0.f,1.f,(db-min)/(max-min));
    };
    g.setColour(kGreen.withAlpha(0.4f));
    g.fillRect(markX(-23.f), track.getY(), 1.5f, track.getHeight());
    g.setColour(kAmber.withAlpha(0.4f));
    g.fillRect(markX(-14.f), track.getY(), 1.5f, track.getHeight());
    g.setColour(kRed.withAlpha(0.4f));
    g.fillRect(markX(-9.f),  track.getY(), 1.5f, track.getHeight());
}

// ── ValueReadout ──────────────────────────────────────────────
ValueReadout::ValueReadout(const juce::String& l, const juce::String& u, juce::Colour c)
    : label(l), unit(u), color(c) {}

void ValueReadout::setValue(float v, bool showPlus) { value = v; plus = showPlus; repaint(); }

void ValueReadout::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour(kSurface);
    g.fillRoundedRectangle(b, 6.f);
    g.setColour(kBorder);
    g.drawRoundedRectangle(b, 6.f, 1.f);

    g.setFont(juce::Font("Courier New", 9.f, juce::Font::plain));
    g.setColour(kSubText);
    g.drawText(label, b.withHeight(20), juce::Justification::centred);

    bool warn = (label == "PEAK" && value >= 0.f);
    g.setColour(warn ? kRed : color);
    g.setFont(juce::Font("Courier New", 22.f, juce::Font::bold));
    juce::String valStr = (plus && value > 0 ? "+" : "") + juce::String(value, 1);
    g.drawText(valStr, b.withTrimmedTop(14).withTrimmedBottom(14),
               juce::Justification::centred);

    g.setFont(juce::Font("Courier New", 9.f, juce::Font::plain));
    g.setColour(kSubText);
    g.drawText(unit, b.withTop(b.getBottom()-18), juce::Justification::centred);
}

// ── Editor ────────────────────────────────────────────────────
LumetrixAudioProcessorEditor::LumetrixAudioProcessorEditor(LumetrixAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&laf);
    setSize(640, 360);

    gainKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    gainKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
    gainKnob.setRange(-60.0, 60.0, 0.01);
    gainKnob.setDoubleClickReturnValue(true, 0.0);
    addAndMakeVisible(gainKnob);

    gainLabel.setText("GAIN STAGING", juce::dontSendNotification);
    gainLabel.setFont(juce::Font("Courier New", 9.f, juce::Font::plain));
    gainLabel.setColour(juce::Label::textColourId, kSubText);
    gainLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(gainLabel);

    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "GAIN", gainKnob);

    resetBtn.onClick = [this] { audioProcessor.resetMeters(); };
    addAndMakeVisible(resetBtn);

    addAndMakeVisible(vuL);   addAndMakeVisible(vuR);
    addAndMakeVisible(lufsIntBar); addAndMakeVisible(lufsSTPBar); addAndMakeVisible(lufsMomBar);
    addAndMakeVisible(readPeak);   addAndMakeVisible(readRMS);
    addAndMakeVisible(readLUFS);   addAndMakeVisible(readDyn);

    startTimerHz(30);
}

LumetrixAudioProcessorEditor::~LumetrixAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void LumetrixAudioProcessorEditor::timerCallback()
{
    auto m = audioProcessor.getMeterData();
    vuL.setLevel(m.levelL, m.peakHoldL);
    vuR.setLevel(m.levelR, m.peakHoldR);
    lufsIntBar.setValue(m.lufsIntegrated);
    lufsSTPBar.setValue(m.lufsShortTerm);
    lufsMomBar.setValue(m.lufsMomentary);
    readPeak.setValue(m.truePeak,      true);
    readRMS .setValue(m.rms,           false);
    readLUFS.setValue(m.lufsIntegrated,false);
    readDyn .setValue(m.dynamicRange,  false);
}

void LumetrixAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff13161c), 0, 0,
        juce::Colour(0xff0a0c10), getWidth(), getHeight(), false));
    g.fillAll();

    g.setColour(juce::Colour(0x11ffffff));
    g.fillRect(0, 0, getWidth(), 30);
    g.setColour(kBorder);
    g.drawLine(0, 30, (float)getWidth(), 30.f, 1.f);

    g.setColour(kGreen);
    g.fillEllipse(14, 11, 8, 8);

    g.setFont(juce::Font("Courier New", 11.f, juce::Font::bold));
    g.setColour(kText);
    g.drawText("LUMETRIX PRO", 30, 0, 200, 30, juce::Justification::centredLeft);

    g.setFont(juce::Font("Courier New", 9.f, juce::Font::plain));
    g.setColour(kSubText);
    g.drawText("v1.0.0  |  VST3", 148, 0, 120, 30, juce::Justification::centredLeft);
}

void LumetrixAudioProcessorEditor::resized()
{
    const int pad = 10, titleH = 32;
    int y = titleH + pad;
    int h = getHeight() - y - pad;

    int barW = 22;
    vuL.setBounds(pad,           y, barW, h);
    vuR.setBounds(pad+barW+4,    y, barW, h);

    int midX = pad + barW*2 + 14;
    int midW = getWidth() - midX - 150 - pad;

    int lufsH = 22, lufsGap = 4, lufsTop = y + 8;
    lufsIntBar.setBounds(midX, lufsTop,                   midW, lufsH);
    lufsSTPBar.setBounds(midX, lufsTop + lufsH + lufsGap, midW, lufsH);
    lufsMomBar.setBounds(midX, lufsTop +(lufsH+lufsGap)*2,midW, lufsH);

    int rdTop = lufsTop + (lufsH+lufsGap)*3 + 8;
    int rdW   = (midW-6)/2, rdH = 58;
    readPeak.setBounds(midX,       rdTop,       rdW, rdH);
    readRMS .setBounds(midX+rdW+6, rdTop,       rdW, rdH);
    readLUFS.setBounds(midX,       rdTop+rdH+6, rdW, rdH);
    readDyn .setBounds(midX+rdW+6, rdTop+rdH+6, rdW, rdH);

    int knobX = getWidth() - 140;
    gainLabel.setBounds(knobX,    y,     130, 14);
    gainKnob .setBounds(knobX+15, y+16,  100, 100);
    resetBtn .setBounds(knobX+20, getHeight()-pad-24, 90, 22);
}

juce::AudioProcessorEditor* LumetrixAudioProcessor::createEditor()
{
    return new LumetrixAudioProcessorEditor(*this);
}
