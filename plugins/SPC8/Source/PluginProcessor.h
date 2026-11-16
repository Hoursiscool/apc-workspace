#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

#include "dsp/SpcDsp.h"

class SPC8AudioProcessor : public juce::AudioProcessor
{
public:
    SPC8AudioProcessor();
    ~SPC8AudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SPC8"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    void applyEnvelopeSettings();

    // The engine is a 32 kHz sample-playback model: 128-sample single-cycle
    // table at 250 Hz root (kEngineRate / 128), read at a host-rate-scaled step.
    static constexpr int kTableLength = 128;

    std::vector<juce::int16> tableStorage;
    spc8::Sample sample;
    spc8::VoicePool pool;

    double hostRate = 48000.0;
    std::atomic<float>* masterParam   = nullptr;
    std::atomic<float>* attackParam   = nullptr;
    std::atomic<float>* decayParam    = nullptr;
    std::atomic<float>* sustainParam  = nullptr;
    std::atomic<float>* releaseParam  = nullptr;
    std::atomic<float>* interpParam   = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SPC8AudioProcessor)
};
