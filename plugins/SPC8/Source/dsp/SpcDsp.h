#pragma once
// ============================================================================
//  SPC8 — S-DSP core (phase 2.1.1)
//
//  Deliberately JUCE-free so the engine can be compiled and exercised by a
//  plain console self-test (Source/dsp/selftest.cpp) as well as inside the
//  plugin. Everything here is allocation-free after construction.
//
//  Fidelity notes for this phase:
//   * BRR decode implements the real 9-byte block layout, 4 range/filter
//     semantics and the 4-tap prediction filters. The prediction coefficients
//     are the documented hardware ratios (15/16, 61/32-15/16, 115/64-13/16).
//   * Gaussian interpolation uses a computed 4-tap gaussian kernel, NOT the
//     hardware's 512-entry table yet — that transcription is tracked as the
//     "gaussian table fidelity" risk in .ideas/plan.md.
//   * Envelope rate registers are mapped to per-sample increments with a
//     documented approximation; exact hardware stepping is a 2.1.2 refinement.
// ============================================================================

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace spc8
{
constexpr double kEngineRate = 32000.0;   // S-DSP native clock
constexpr int    kNumVoices  = 8;         // hardware voice budget
constexpr int    kBrrBlock   = 9;         // 1 header + 8 sample bytes
constexpr int    kBrrSamples = 16;        // samples per BRR block

// ---------------------------------------------------------------- BRR decode
struct BrrState
{
    int32_t prev1 = 0;
    int32_t prev2 = 0;
};

inline void decodeBrrBlock (const uint8_t* block, int16_t* out, BrrState& st)
{
    const int range  = (block[0] >> 4) & 0x0F;
    const int filter = (block[0] >> 2) & 0x03;

    for (int i = 0; i < kBrrSamples; ++i)
    {
        const uint8_t byte = block[1 + (i >> 1)];
        int nib = (i & 1) ? (byte & 0x0F) : (byte >> 4);
        if (nib >= 8) nib -= 16;                      // 4-bit two's complement

        int32_t s = (int32_t) nib << range;

        switch (filter)                                // prediction filters
        {
            case 1:  s += (st.prev1 * 15) / 16; break;
            case 2:  s += (st.prev1 * 61) / 32 - (st.prev2 * 15) / 16; break;
            case 3:  s += (st.prev1 * 115) / 64 - (st.prev2 * 13) / 16; break;
            default: break;
        }

        s = std::clamp (s, -32768, 32767);
        st.prev2 = st.prev1;
        st.prev1 = s;
        out[i] = (int16_t) (s & ~1);                   // hardware drops bit 0
    }
}

// ------------------------------------------------------- interpolation kernel
inline const float* gaussTable()
{
    static float table[512];
    static bool init = false;
    if (! init)
    {
        for (int i = 0; i < 512; ++i)
        {
            const double x = (i - 256) / 256.0 * 2.0;
            table[i] = (float) std::exp (-2.0 * x * x);
        }
        init = true;
    }
    return table;
}

inline float gaussWeight (float dist)
{
    const float* t = gaussTable();
    int idx = (int) std::lround (dist * 256.0f) + 256;
    idx = std::clamp (idx, 0, 511);
    return t[idx];
}

// ---------------------------------------------------------------- sample data
struct Sample
{
    const int16_t* pcm = nullptr;
    int   length    = 0;
    int   loopStart = 0;
    bool  loop      = true;
    float rootHz    = 250.0f;

    int index (int i) const
    {
        if (length <= 0) return 0;
        if (loop)
        {
            const int span = length - loopStart;
            if (span <= 0) return 0;
            while (i >= length)     i -= span;
            while (i <  loopStart)  i += span;
        }
        else
        {
            i = std::clamp (i, 0, length - 1);
        }
        return i;
    }

    // mode: 0 = off (nearest), 1 = linear, 2 = gaussian
    float read (double pos, int mode) const
    {
        if (pcm == nullptr || length <= 0) return 0.0f;
        const int   base = (int) std::floor (pos);
        const float frac = (float) (pos - base);

        constexpr float kNorm = 1.0f / 32768.0f;   // int16 → ±1.0

        if (mode == 0)
            return (float) pcm[index (base)] * kNorm;

        if (mode == 1)
        {
            const float a = (float) pcm[index (base)];
            const float b = (float) pcm[index (base + 1)];
            return (a + (b - a) * frac) * kNorm;
        }

        const float s0 = (float) pcm[index (base - 1)];
        const float s1 = (float) pcm[index (base)];
        const float s2 = (float) pcm[index (base + 1)];
        const float s3 = (float) pcm[index (base + 2)];

        const float w0 = gaussWeight (frac + 1.0f);
        const float w1 = gaussWeight (frac);
        const float w2 = gaussWeight (1.0f - frac);
        const float w3 = gaussWeight (2.0f - frac);
        const float wsum = w0 + w1 + w2 + w3;

        if (wsum <= 0.0f) return s1 * kNorm;
        return ((s0 * w0 + s1 * w1 + s2 * w2 + s3 * w3) / wsum) * kNorm;
    }
};

// ------------------------------------------------------------------- envelope
enum class EnvStage { idle, attack, decay, sustain, release };

struct Envelope
{
    EnvStage stage = EnvStage::idle;
    float level    = 0.0f;
    float sustain  = 0.65f;

    // hardware rate registers (ADSR1/ADSR2/GAIN) mapped to increments
    float atkInc = 0.002f, decInc = 0.0008f, susInc = 0.00004f, relInc = 0.0006f;

    // Rate registers are mapped to musical durations (higher register = faster),
    // converted to per-sample increments at the 32 kHz engine clock. Exact
    // hardware stepping tables are a 2.1.2 fidelity refinement.
    void setAdsr (int attack, int decay, int sustainLevel, int sustainRate, int release)
    {
        atkInc = secondsToInc (3.0f / (1.0f + (float) attack));          // 0: 3 s … 15: 0.19 s
        decInc = secondsToInc (2.0f / (1.0f + (float) decay));           // 0: 2 s … 7: 0.25 s
        susInc = secondsToInc (20.0f / (1.0f + (float) sustainRate));    // 0: 20 s … 31: 0.6 s
        relInc = secondsToInc (4.0f / (1.0f + (float) release));         // 0: 4 s … 31: 0.125 s
        sustain = (float) sustainLevel / 7.0f;
    }

    static float secondsToInc (float seconds)
    {
        const float clamped = std::max (seconds, 1.0f / 4000.0f);
        return 1.0f / (clamped * (float) kEngineRate);
    }

    void noteOn()  { stage = EnvStage::attack; }
    void noteOff() { if (stage != EnvStage::idle) stage = EnvStage::release; }
    bool finished() const { return stage == EnvStage::idle; }

    float tick()
    {
        switch (stage)
        {
            case EnvStage::attack:
                level += atkInc;
                if (level >= 1.0f) { level = 1.0f; stage = EnvStage::decay; }
                break;
            case EnvStage::decay:
                level -= decInc;
                if (level <= sustain) { level = sustain; stage = EnvStage::sustain; }
                break;
            case EnvStage::sustain:
                level -= susInc;
                if (level <= 0.0f) { level = 0.0f; stage = EnvStage::idle; }
                break;
            case EnvStage::release:
                level -= relInc;
                if (level <= 0.0f) { level = 0.0f; stage = EnvStage::idle; }
                break;
            default: level = 0.0f; break;
        }
        return level;
    }
};

// ---------------------------------------------------------------------- voice
struct Voice
{
    bool     active   = false;
    int      note     = -1;
    uint64_t age      = 0;
    double   position = 0.0;
    double   step     = 1.0;
    float    velocity = 1.0f;
    int      interp   = 2;      // 0 off / 1 linear / 2 gaussian
    Envelope env;

    void start (int midiNote, float velocityIn, double hostRate, const Sample& smp)
    {
        const double freq = 440.0 * std::pow (2.0, (midiNote - 69) / 12.0);

        // table is a 32 kHz stream; read rate scales by engine/host
        step = (freq / (double) smp.rootHz) * (kEngineRate / hostRate);
        position = 0.0;
        velocity = velocityIn;
        note = midiNote;
        active = true;
        env.noteOn();
    }

    void stop() { env.noteOff(); }

    float render (const Sample& smp)
    {
        if (! active) return 0.0f;
        const float gain = env.tick();
        if (env.finished()) { active = false; return 0.0f; }
        const float s = smp.read (position, interp);
        position += step;
        if (position >= (double) smp.length && smp.loop)
            position -= (double) (smp.length - smp.loopStart);
        return s * gain * velocity;
    }
};

// --------------------------------------------------------------- voice pool
struct VoicePool
{
    Voice    voices[kNumVoices];
    uint64_t counter = 0;

    Voice& allocate()
    {
        for (auto& v : voices)
            if (! v.active) { v.age = ++counter; return v; }

        Voice* oldest = &voices[0];
        for (auto& v : voices)
            if (v.age < oldest->age) oldest = &v;

        *oldest = Voice {};
        oldest->age = ++counter;
        return *oldest;
    }

    void noteOn (int note, float velocity, double hostRate, const Sample& smp)
    {
        auto& v = allocate();
        v.start (note, velocity, hostRate, smp);
    }

    void noteOff (int note)
    {
        for (auto& v : voices)
            if (v.active && v.note == note) v.stop();
    }

    void allNotesOff()
    {
        for (auto& v : voices)
            if (v.active) v.stop();
    }

    void setInterpolation (int mode)
    {
        for (auto& v : voices) v.interp = mode;
    }

    int activeCount() const
    {
        int n = 0;
        for (const auto& v : voices) if (v.active) ++n;
        return n;
    }

    float render (const Sample& smp)
    {
        float sum = 0.0f;
        for (auto& v : voices) sum += v.render (smp);
        return sum;
    }
};

} // namespace spc8
