#include "PluginProcessor.h"

namespace
{
constexpr double kPi = 3.14159265358979323846;

juce::AudioProcessorValueTreeState::ParameterLayout makeLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "master_volume", 1 }, "Master Volume",
        juce::NormalisableRange<float> (-96.0f, 0.0f, 0.1f), -6.0f));

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "inst1_attack", 1 }, "Attack", 0, 15, 12));

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "inst1_decay", 1 }, "Decay", 0, 7, 4));

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "inst1_sustain_level", 1 }, "Sustain Level", 0, 7, 5));

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "inst1_release", 1 }, "Release", 0, 31, 12));

    juce::StringArray interp { "GAUSS", "LINEAR", "OFF" };
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "interpolation", 1 }, "Interpolation", interp, 0));

    return { params.begin(), params.end() };
}
} // namespace

SPC8AudioProcessor::SPC8AudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", makeLayout())
{
    tableStorage.resize ((size_t) kTableLength);
    for (int i = 0; i < kTableLength; ++i)
    {
        const double phase = 2.0 * kPi * (double) i / (double) kTableLength;
        tableStorage[(size_t) i] = (juce::int16) std::lround (std::sin (phase) * 16384.0);
    }

    sample.pcm       = tableStorage.data();
    sample.length    = kTableLength;
    sample.loopStart = 0;
    sample.loop      = true;
    sample.rootHz    = (float) (spc8::kEngineRate / (double) kTableLength);

    masterParam  = apvts.getRawParameterValue ("master_volume");
    attackParam  = apvts.getRawParameterValue ("inst1_attack");
    decayParam   = apvts.getRawParameterValue ("inst1_decay");
    sustainParam = apvts.getRawParameterValue ("inst1_sustain_level");
    releaseParam = apvts.getRawParameterValue ("inst1_release");
    interpParam  = apvts.getRawParameterValue ("interpolation");

    applyEnvelopeSettings();
}

void SPC8AudioProcessor::applyEnvelopeSettings()
{
    const int a  = (int) attackParam->load();
    const int d  = (int) decayParam->load();
    const int sl = (int) sustainParam->load();
    const int rr = (int) releaseParam->load();
    const int mode = (int) interpParam->load();

    for (auto& v : pool.voices)
    {
        v.env.setAdsr (a, d, sl, 8, rr);
        v.interp = mode == 0 ? 2 : (mode == 1 ? 1 : 0);
    }
}

void SPC8AudioProcessor::prepareToPlay (double sampleRate, int)
{
    hostRate = sampleRate > 0.0 ? sampleRate : 48000.0;
    pool.allNotesOff();
    applyEnvelopeSettings();
}

bool SPC8AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void SPC8AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    applyEnvelopeSettings();

    for (const auto metadata : midi)
    {
        const auto msg = metadata.getMessage();

        if (msg.isNoteOn())
            pool.noteOn (msg.getNoteNumber(), msg.getFloatVelocity(), hostRate, sample);
        else if (msg.isNoteOff())
            pool.noteOff (msg.getNoteNumber());
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            pool.allNotesOff();
    }

    const float masterGain = juce::Decibels::decibelsToGain (masterParam->load(), -96.0f);
    const int   numSamples = buffer.getNumSamples();
    const int   numCh      = buffer.getNumChannels();

    auto* left  = numCh > 0 ? buffer.getWritePointer (0) : nullptr;
    auto* right = numCh > 1 ? buffer.getWritePointer (1) : nullptr;

    // 8 voices summing; scale headroom so a full 8-voice chord doesn't clip.
    constexpr float voiceTrim = 0.22f;

    for (int i = 0; i < numSamples; ++i)
    {
        const float s = pool.render (sample) * voiceTrim * masterGain;
        if (left  != nullptr) left[i]  = s;
        if (right != nullptr) right[i] = s;
    }
}

juce::AudioProcessorEditor* SPC8AudioProcessor::createEditor()
{
    // Phase 2.1.1 placeholder: the S-DSP strip arrives in 2.1.4 (WebView).
    return new juce::GenericAudioProcessorEditor (*this);
}

void SPC8AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void SPC8AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SPC8AudioProcessor();
}
