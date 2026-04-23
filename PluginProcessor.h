// ============================================================
//  LUMETRIX PRO – VST3 Plugin
//  Loudness Meter (LUFS, RMS, Peak, Dynamic Range) + Gain
//  Built with JUCE framework
//
//  SETUP:
//  1. Create new JUCE Audio Plugin project (Projucer)
//  2. Enable VST3 in Projucer → Plugin Formats
//  3. Replace auto-generated PluginProcessor.h/cpp and
//     PluginEditor.h/cpp with these files
//  4. Add JUCE modules: juce_audio_processors, juce_dsp,
//     juce_gui_basics, juce_graphics
// ============================================================

#pragma once
#include <JuceHeader.h>

// EBU R128 / ITU-R BS.1770 K-weighting coefficients (48kHz)
// Pre-filter (shelf): b0=1.53512485958697, b1=-2.69169618940638, b2=1.19839281085285
//                     a0=1.0,              a1=-1.69065929318241, a2=0.73248077421585
// RLB filter:         b0=1.0,  b1=-2.0, b2=1.0
//                     a0=1.0,  a1=-1.99004745483398, a2=0.99007225036621

class LumetrixAudioProcessor : public juce::AudioProcessor,
                               public juce::AudioProcessorValueTreeState::Listener
{
public:
    LumetrixAudioProcessor();
    ~LumetrixAudioProcessor() override;

    // ── AudioProcessor interface ──────────────────────────────
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

    // ── Metering data (read by editor on timer) ───────────────
    struct MeterData {
        float lufsIntegrated  = -70.f;
        float lufsShortTerm   = -70.f;
        float lufsMomentary   = -70.f;
        float truePeak        = -70.f;   // dBTP
        float rms             = -70.f;   // dBFS
        float dynamicRange    = 0.f;     // LU
        float peakHoldL       = -70.f;
        float peakHoldR       = -70.f;
        float levelL          = -70.f;
        float levelR          = -70.f;
    };

    MeterData getMeterData() const;
    void resetMeters();

    // APVTS for gain parameter
    juce::AudioProcessorValueTreeState apvts;

private:
    // ── Gain ─────────────────────────────────────────────────
    std::atomic<float> gainLinear { 1.f };

    // ── K-weighting IIR (per channel) ────────────────────────
    struct KFilter {
        // Pre-filter (high shelf)
        double xPreL[3] = {}, yPreL[3] = {};
        double xPreR[3] = {}, yPreR[3] = {};
        // RLB (high-pass)
        double xRlbL[3] = {}, yRlbL[3] = {};
        double xRlbR[3] = {}, yRlbR[3] = {};

        // Coefficients
        double bPre[3], aPre[3], bRlb[3], aRlb[3];

        void prepare(double sampleRate);
        float processSampleL(float x);
        float processSampleR(float x);
    } kFilter;

    // ── Momentary LUFS (400ms sliding window) ────────────────
    static constexpr int kMomentaryMs = 400;
    static constexpr int kShortTermMs = 3000;
    std::vector<double> momentaryBuf, shortTermBuf;
    int momentaryPos = 0, shortTermPos  = 0;
    int momentaryLen = 0, shortTermLen  = 0;

    // ── Integrated LUFS gating ────────────────────────────────
    struct Gate {
        double sumPower     = 0.0;
        int    blockCount   = 0;
        double absThreshold = -70.0;   // -70 LUFS absolute gate
        double relThreshold = -10.0;   // relative: set after first pass
        // For simplicity: single-stage gating (close enough for display)
        double integratedPower = 0.0;
        int    gatedBlocks     = 0;
    } gate;

    // ── True peak (4× oversample) ─────────────────────────────
    juce::dsp::Oversampling<float> oversampler { 2, 2,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR };

    // ── RMS (300ms) ───────────────────────────────────────────
    std::vector<float> rmsBufL, rmsBufR;
    int  rmsPos = 0, rmsLen = 0;
    float rmsSumL = 0.f, rmsSumR = 0.f;

    // ── Peak hold ─────────────────────────────────────────────
    float peakHoldL = -70.f, peakHoldR = -70.f;
    int   peakHoldTimerL = 0, peakHoldTimerR = 0;
    static constexpr int kPeakHoldSamples = 48000 * 2; // 2 sec

    // ── Instant levels ────────────────────────────────────────
    float instantL = -70.f, instantR = -70.f;
    float truePeakVal = -70.f;

    double currentSampleRate = 48000.0;

    // ── Cached meter snapshot (lock-free) ─────────────────────
    mutable juce::SpinLock meterLock;
    MeterData meterSnapshot;

    // ── APVTS layout ──────────────────────────────────────────
    static juce::AudioProcessorValueTreeState::ParameterLayout createParams();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LumetrixAudioProcessor)
};
