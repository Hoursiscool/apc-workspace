// SPC8 — S-DSP console self-test (phase 2.1.1 acceptance)
//
// Proves, without a DAW or JUCE, that:
//   1. a MIDI note-on reaches the voice pool and produces audio frames
//   2. the gaussian-interpolated sample path outputs a sane peak/RMS
//   3. the BRR decoder consumes a real 9-byte block layout and emits 16 samples
//   4. note-off releases the voice and the voice returns to idle

#include "dsp/SpcDsp.h"

#include <cmath>
#include <cstdio>
#include <vector>

namespace
{
constexpr double kPi = 3.14159265358979323846;
}

int main()
{
    // 128-sample single-cycle sine at the engine root rate (32 kHz / 128 = 250 Hz)
    std::vector<int16_t> table (128);
    for (int i = 0; i < 128; ++i)
        table[(size_t) i] = (int16_t) std::lround (std::sin (2.0 * kPi * i / 128.0) * 16384.0);

    spc8::Sample sample;
    sample.pcm       = table.data();
    sample.length    = 128;
    sample.loopStart = 0;
    sample.loop      = true;
    sample.rootHz    = 250.0f;

    spc8::VoicePool pool;
    pool.setInterpolation (2);                        // gaussian
    for (auto& v : pool.voices) v.env.setAdsr (12, 4, 5, 8, 12);

    const double hostRate = 48000.0;
    const int    total    = 48000;                    // one second

    pool.noteOn (60, 0.8f, hostRate, sample);

    double peak = 0.0, energy = 0.0;
    int    soundingSamples = 0;

    for (int i = 0; i < total; ++i)
    {
        const float v = pool.render (sample) * 0.22f;
        peak = std::max (peak, (double) std::fabs (v));
        energy += (double) v * (double) v;
        if (std::fabs (v) > 1.0e-4f) ++soundingSamples;

        if (i == 24000)
            pool.noteOff (60);
    }

    const double rms = std::sqrt (energy / (double) total);

    std::printf ("SPC8 self-test\n");
    std::printf ("  note-on → peak %.4f  rms %.4f  sounding %d/%d samples\n",
                 peak, rms, soundingSamples, total);
    std::printf ("  voices still active after release: %d\n", pool.activeCount());

    // BRR: header 0x00 -> range 0, filter 0; nibbles 1,2,3,...15
    uint8_t block[9] = { 0x00, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF };
    int16_t decoded[16];
    spc8::BrrState st;
    spc8::decodeBrrBlock (block, decoded, st);
    std::printf ("  BRR decode [range 0, filter 0]: %d %d %d … %d\n",
                 decoded[0], decoded[1], decoded[2], decoded[15]);

    const bool voiceFinished = pool.activeCount() == 0;
    // Nibbles 0..15 decode to 0,1,2…15 then the LSB is dropped (hardware),
    // so 1→0 and the negative nibbles E/F become -2.
    const bool brrOk = decoded[0] == 0 && decoded[1] == 0 && decoded[2] == 2
                    && decoded[4] == 4 && decoded[15] == -2;

    bool ok = peak > 0.01 && rms > 0.005 && soundingSamples > total / 2 && voiceFinished && brrOk;

    // eight voices must not exceed the hardware budget
    for (int n = 0; n < 12; ++n) pool.noteOn (48 + n, 0.5f, hostRate, sample);
    const int active = pool.activeCount();
    std::printf ("  12 note-ons → %d active voices (budget %d)\n", active, spc8::kNumVoices);
    ok = ok && active == spc8::kNumVoices;

    std::printf ("  RESULT: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
