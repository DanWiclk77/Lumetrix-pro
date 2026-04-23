#include "PluginProcessor.h"
#include "PluginEditor.h"

static const double kPreB[3] = { 1.53512485958697, -2.69169618940638, 1.19839281085285 };
static const double kPreA[3] = { 1.0,              -1.69065929318241, 0.73248077421585 };
static const double kRlbB[3] = { 1.0,  -2.0,  1.0 };
static const double kRlbA[3] = { 1.0,  -1.99004745483398, 0.99007225036621 };

void LumetrixAudioProcessor::KFilter::prepare(double /*sampleRate*/)
{
    std::copy(kPreB, kPreB+3, bPre);
    std::copy(kPreA, kPreA+3, aPre);
    std::copy(kRlbB, kRlbB+3, bRlb);
    std::copy(kRlbA, kRlbA+3, aRlb);
    std::fill(xPreL, xPreL+3, 0.0); std::fill(yPreL, yPreL+3, 0.0);
    std::fill(xPreR, xPreR+3, 0.0); std::fill(yPreR, yPreR+3, 0.0);
    std::fill(xRlbL, xRlbL+3, 0.0); std::fill(yRlbL, yRlbL+3, 0.0);
    std::fill(xRlbR, xRlbR+3, 0.0); std::fill(yRlbR, yRlbR+3, 0.0);
}

static inline double applyBiquad(double x, double* xd, double* yd,
                                  const double* b, const double* a)
{
    xd[2] = xd[1]; xd[1] = xd[0]; xd[0] = x;
    double y = b[0]*xd[0] + b[1]*xd[1] + b[2]*xd[2]
             - a[1]*yd[1] - a[2]*yd[2];
    yd[2] = yd[1]; yd[1] = yd[0]; yd[0] = y;
    return y;
}

float LumetrixAudioProcessor::KFilter::processSampleL(float x)
{
    double y = applyBiquad(x, xPreL, yPreL, bPre, aPre);
    return (float)applyBiquad(y, xRlbL, yRlbL, bRlb, aRlb);
}

float LumetrixAudioProcessor::KFilter::processSampleR(float x)
{
    double y = applyBiquad(x, xPreR, yPreR, bPre, aPre);
    return (float)applyBiquad(y, xRlbR, yRlbR, bRlb, aRlb);
}

juce::AudioProcessorValueTreeState::ParameterLayout
LumetrixAudioProcessor::createParams()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "GAIN", 1 }, "Gain",
        juce::NormalisableRange<float>(-60.f, 60.f, 0.01f, 1.f),
        0.f));
    return layout;
}

LumetrixAudioProcessor::LumetrixAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParams())
{
    apvts.addParameterListener("GAIN", this);
}

LumetrixAudioProcessor::~LumetrixAudioProcessor()
{
    apvts.removeParameterListener("GAIN", this);
}

void LumetrixAudioProcessor::parameterChanged(const juce::String& paramID, float newValue)
{
    if (paramID == "GAIN")
        gainLinear.store(juce::Decibels::decibelsToGain(newValue));
}

void LumetrixAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    kFilter.prepare(sampleRate);

    momentaryLen = juce::roundToInt(sampleRate * kMomentaryMs / 1000.0);
    shortTermLen  = juce::roundToInt(sampleRate * kShortTermMs / 1000.0);
    momentaryBuf.assign(momentaryLen, 0.0);
    shortTermBuf.assign(shortTermLen, 0.0);
    momentaryPos = shortTermPos = 0;

    rmsLen = juce::roundToInt(sampleRate * 0.3);
    rmsBufL.assign(rmsLen, 0.f);
    rmsBufR.assign(rmsLen, 0.f);
    rmsPos = 0; rmsSumL = rmsSumR = 0.f;

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32)samplesPerBlock, 2 };
    oversampler.initProcessing((size_t)samplesPerBlock);

    peakHoldL = peakHoldR = -70.f;
    peakHoldTimerL = peakHoldTimerR = 0;
    instantL = instantR = truePeakVal = -70.f;
    resetMeters();
}

void LumetrixAudioProcessor::releaseResources() {}

bool LumetrixAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) return false;
    if (layouts.getMainInputChannelSet()  != juce::AudioChannelSet::stereo()) return false;
    return true;
}

void LumetrixAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const float gain = gainLinear.load();
    const int numSamples = buffer.getNumSamples();

    buffer.applyGain(gain);

    const float* chanL = buffer.getReadPointer(0);
    const float* chanR = buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : chanL;

    // True peak via 4x oversampling
    {
        juce::dsp::AudioBlock<float> block(buffer);
        auto osBlock = oversampler.processSamplesUp(block);
        float tp = -70.f;
        for (int ch = 0; ch < (int)osBlock.getNumChannels(); ++ch)
        {
            const float* ptr = osBlock.getChannelPointer(ch);
            for (int i = 0; i < (int)osBlock.getNumSamples(); ++i)
            {
                float absV = std::abs(ptr[i]);
                if (absV > 0.f)
                {
                    float dbv = juce::Decibels::gainToDecibels(absV);
                    if (dbv > tp) tp = dbv;
                }
            }
        }
        truePeakVal = std::max(truePeakVal, tp);
        oversampler.processSamplesDown(block);
    }

    double momentarySum = 0.0, shortTermSum = 0.0;

    for (int i = 0; i < numSamples; ++i)
    {
        float l = chanL[i];
        float r = chanR[i];

        float absL = std::abs(l);
        float absR = std::abs(r);
        instantL = absL > 0.f ? juce::Decibels::gainToDecibels(absL) : -70.f;
        instantR = absR > 0.f ? juce::Decibels::gainToDecibels(absR) : -70.f;

        if (instantL > peakHoldL) { peakHoldL = instantL; peakHoldTimerL = kPeakHoldSamples; }
        else if (peakHoldTimerL > 0) peakHoldTimerL--;
        else peakHoldL = std::max(peakHoldL - 0.001f, -70.f);

        if (instantR > peakHoldR) { peakHoldR = instantR; peakHoldTimerR = kPeakHoldSamples; }
        else if (peakHoldTimerR > 0) peakHoldTimerR--;
        else peakHoldR = std::max(peakHoldR - 0.001f, -70.f);

        float kL = kFilter.processSampleL(l);
        float kR = kFilter.processSampleR(r);
        double power = 0.5 * ((double)kL*kL + (double)kR*kR);

        momentarySum -= momentaryBuf[momentaryPos];
        momentaryBuf[momentaryPos] = power;
        momentarySum += power;
        momentaryPos = (momentaryPos + 1) % momentaryLen;

        shortTermSum -= shortTermBuf[shortTermPos];
        shortTermBuf[shortTermPos] = power;
        shortTermSum += power;
        shortTermPos = (shortTermPos + 1) % shortTermLen;

        rmsSumL -= rmsBufL[rmsPos];
        rmsBufL[rmsPos] = l*l;
        rmsSumL += rmsBufL[rmsPos];
        rmsSumR -= rmsBufR[rmsPos];
        rmsBufR[rmsPos] = r*r;
        rmsSumR += rmsBufR[rmsPos];
        rmsPos = (rmsPos + 1) % rmsLen;

        gate.sumPower += power;
        gate.blockCount++;
        int blockSize100ms = juce::roundToInt(currentSampleRate * 0.1);
        if (gate.blockCount >= blockSize100ms)
        {
            double blockPower = gate.sumPower / gate.blockCount;
            double blockLufs  = -0.691 + 10.0 * std::log10(std::max(blockPower, 1e-10));
            if (blockLufs > gate.absThreshold)
            {
                gate.integratedPower += blockPower;
                gate.gatedBlocks++;
            }
            gate.sumPower = 0.0;
            gate.blockCount = 0;
        }
    }

    auto powerToLufs = [](double sum, int len) -> float {
        double avg = sum / std::max(len, 1);
        return (float)(-0.691 + 10.0 * std::log10(std::max(avg, 1e-10)));
    };

    float lufsMom = powerToLufs(momentarySum, momentaryLen);
    float lufsST  = powerToLufs(shortTermSum, shortTermLen);
    float lufsInt = gate.gatedBlocks > 0
        ? (float)(-0.691 + 10.0 * std::log10(gate.integratedPower / gate.gatedBlocks))
        : -70.f;

    float rmsVal = -70.f;
    if (rmsLen > 0)
    {
        float rmsL   = std::sqrt(std::max(rmsSumL / rmsLen, 0.f));
        float rmsR   = std::sqrt(std::max(rmsSumR / rmsLen, 0.f));
        float rmsAvg = 0.5f * (rmsL + rmsR);
        rmsVal = rmsAvg > 0.f ? juce::Decibels::gainToDecibels(rmsAvg) : -70.f;
    }

    float dynRange = std::max(truePeakVal - rmsVal, 0.f);

    {
        juce::SpinLock::ScopedLockType lock(meterLock);
        meterSnapshot.lufsMomentary  = lufsMom;
        meterSnapshot.lufsShortTerm  = lufsST;
        meterSnapshot.lufsIntegrated = lufsInt;
        meterSnapshot.truePeak       = truePeakVal;
        meterSnapshot.rms            = rmsVal;
        meterSnapshot.dynamicRange   = dynRange;
        meterSnapshot.peakHoldL      = peakHoldL;
        meterSnapshot.peakHoldR      = peakHoldR;
        meterSnapshot.levelL         = instantL;
        meterSnapshot.levelR         = instantR;
    }
}

LumetrixAudioProcessor::MeterData LumetrixAudioProcessor::getMeterData() const
{
    juce::SpinLock::ScopedLockType lock(meterLock);
    return meterSnapshot;
}

void LumetrixAudioProcessor::resetMeters()
{
    juce::SpinLock::ScopedLockType lock(meterLock);
    gate = Gate{};
    truePeakVal = -70.f;
    peakHoldL = peakHoldR = -70.f;
    peakHoldTimerL = peakHoldTimerR = 0;
    std::fill(momentaryBuf.begin(), momentaryBuf.end(), 0.0);
    std::fill(shortTermBuf.begin(), shortTermBuf.end(), 0.0);
    std::fill(rmsBufL.begin(), rmsBufL.end(), 0.f);
    std::fill(rmsBufR.begin(), rmsBufR.end(), 0.f);
    rmsSumL = rmsSumR = 0.f;
    meterSnapshot = MeterData{};
}

void LumetrixAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void LumetrixAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LumetrixAudioProcessor();
}
