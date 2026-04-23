#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class LumetrixAudioProcessor : public juce::AudioProcessor,
                               public juce::AudioProcessorValueTreeState::Listener
{
public:
    LumetrixAudioProcessor();
    ~LumetrixAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "LUMETRIX PRO"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void parameterChanged(const juce::String& paramID, float newValue) override;

    struct MeterData {
        float lufsIntegrated  = -70.f;
        float lufsShortTerm   = -70.f;
        float lufsMomentary   = -70.f;
        float truePeak        = -70.f;
        float rms             = -70.f;
        float dynamicRange    = 0.f;
        float peakHoldL       = -70.f;
        float peakHoldR       = -70.f;
        float levelL          = -70.f;
        float levelR          = -70.f;
    };

    MeterData getMeterData() const;
    void resetMeters();

    juce::AudioProcessorValueTreeState apvts;

private:
    std::atomic<float> gainLinear { 1.f };

    struct KFilter {
        double xPreL[3] = {}, yPreL[3] = {};
        double xPreR[3] = {}, yPreR[3] = {};
        double xRlbL[3] = {}, yRlbL[3] = {};
        double xRlbR[3] = {}, yRlbR[3] = {};
        double bPre[3], aPre[3], bRlb[3], aRlb[3];

        void prepare(double sampleRate);
        float processSampleL(float x);
        float processSampleR(float x);
    } kFilter;

    static constexpr int kMomentaryMs = 400;
    static constexpr int kShortTermMs = 3000;
    std::vector<double> momentaryBuf, shortTermBuf;
    int momentaryPos = 0, shortTermPos = 0;
    int momentaryLen = 0, shortTermLen = 0;

    struct Gate {
        double sumPower       = 0.0;
        int    blockCount     = 0;
        double absThreshold   = -70.0;
        double integratedPower= 0.0;
        int    gatedBlocks    = 0;
    } gate;

    juce::dsp::Oversampling<float> oversampler { 2, 2,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR };

    std::vector<float> rmsBufL, rmsBufR;
    int   rmsPos = 0, rmsLen = 0;
    float rmsSumL = 0.f, rmsSumR = 0.f;

    float peakHoldL = -70.f, peakHoldR = -70.f;
    int   peakHoldTimerL = 0, peakHoldTimerR = 0;
    static constexpr int kPeakHoldSamples = 48000 * 2;

    float instantL = -70.f, instantR = -70.f;
    float truePeakVal = -70.f;

    double currentSampleRate = 48000.0;

    mutable juce::SpinLock meterLock;
    MeterData meterSnapshot;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParams();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LumetrixAudioProcessor)
};
