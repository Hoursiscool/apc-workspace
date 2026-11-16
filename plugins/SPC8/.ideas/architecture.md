# SPC8 — DSP Architecture Specification

**Phase:** plan · **Version:** v0.1 · **Framework:** webview · **Complexity:** 4/5

## Scope of this document

v0.1 delivers the S-DSP engine, one instrument page, and the factory sample bank. The 16-instrument table, WAV→BRR import, patch I/O and `.spc` export are designed-for but not built — the interfaces below are shaped so they slot in without rework.

## Core components

1. **SampleBank** — immutable, compiled into the binary as `BinaryData`.
   - 16 factory samples stored as BRR (9-byte blocks: 1 header + 16 nibble pairs).
   - Per-slot metadata: block count, loop block index, root rate, loop flag.
   - Decoded on the fly per voice (no predecode to RAM at v0.1; BRR decode is 2 multiplies per 2 samples).

2. **BrrDecoder** — per voice, per 16-sample block.
   - Header: range (4 bits) + filter (2 bits) + loop/end flags.
   - Output: 16 signed 16-bit PCM samples, filter applied against previous two samples.
   - Filters 0–3 per the DSP's 4-tap prediction — included even though the factory bank favours filter 0/1.

3. **VoiceEngine × 8** — the heart of the plugin.
   - **Position:** block pointer + 4-bit nibble phase, advanced by a 14-bit pitch counter.
   - **Pitch:** `PITCH` register (14-bit) derived from note + coarse/fine tune + root note; sample-rate-independent because the engine runs at 32 kHz internally.
   - **Interpolation:** 4-tap Gaussian table (hardware table, 512 entries), switchable to linear or off — off is important, it is the raw aliasing some SNES tracks use.
   - **Envelope:** ADSR1/ADSR2/GAIN register model; 32-entry rate tables and 8-level sustain; GAIN modes direct/linear-dec/bent-line.
   - **Noise:** 15-bit LFSR clocked by the global `noise_rate`; replaces the sample output for that voice.
   - **Pitch modulation:** previous voice's output offsets this voice's pitch counter — hardware-accurate ordering matters (voice n modulates n+1).

4. **VoiceAllocator** — 8 slots; v0.1 serves one instrument.
   - Policies: oldest / quietest / lowest_pitch (`voice_steal`).
   - v0.2 extends this to per-instrument ownership without changing the policy code.

5. **Mixer** — sums 8 voices (mono), applies `master_volume`, splits dry to L/R.

6. **EchoUnit** — the only source of stereo.
   - Circular buffer sized from `echo_delay` (EDL × 16 ms, max 240 ms).
   - 8-tap FIR (`echo_fir_0..7`), feedback (`echo_feedback`) with hardware-style clamping.
   - Wet L/R from `EVOL`; echo enable is `FLG` bit 5; per-voice send is `EON` bit n.

7. **Resampler** — engine clock 32 kHz → host rate.
   - Rational for authenticity: the DSP's own numeric behaviour (4-bit samples, 32 kHz step) is preserved; only the final stream is resampled.
   - Implementation: JUCE `LagrangeInterpolator` on the mixed stereo output, prepared in `prepareToPlay`.

8. **Parameter layer** — `AudioProcessorValueTreeState`.
   - 36 parameters (16 global + 20 `inst1_*`), all IDs fixed per `parameter-spec.md`.
   - Register-model smoothing: envelope/GAIN values are applied at the rate the hardware steps (not per-sample linear ramps), so stair-stepping is audible and correct.

9. **UI bridge** — WebView2 page + `WebSliderRelay` / attachments.
   - v0.1: one instrument page (sample slot, tune, envelope, gain mode, echo send, pitch mod, noise) plus a global echo strip.
   - Relay-before-webview-before-attachment member ordering is mandatory (documented APC crash trap).

## Processing chain

```
MIDI note on/off
   → VoiceAllocator (8 slots, steal policy)
   → VoiceEngine: BRR decode → Gaussian interp → ADSR/GAIN
                       ├─ noise LFSR (if NON)
                       └─ pitch mod (if PMON)
   → Mixer (sum 8 voices, mono) → master volume
   → ┬ dry ──────────────────────────────┐
     └ EchoUnit (FIR + feedback + EDL) ──┤ wet (EVOL)
                                          ↓
                                    Resampler 32 kHz → host
                                          ↓
                                        Output L/R
```

## Parameter mapping (v0.1)

| Parameter(s) | Component | Function | Range |
| :--- | :--- | :--- | :--- |
| `inst1_sample_slot`, `inst1_loop`, `inst1_root_note` | SampleBank / VoiceEngine | Select BRR slot, loop behaviour, pitch anchor | 0–15, on/off, 0–127 |
| `inst1_tune`, `inst1_fine` | VoiceEngine pitch counter | PITCH register scale | ±48 st, ±100 cents |
| `inst1_attack`…`inst1_release`, `inst1_envelope_mode`, `inst1_gain_*` | Envelope | ADSR1/ADSR2/GAIN registers | hardware rates |
| `inst1_noise_enable` + `noise_rate` | Noise LFSR | Replace sample with noise | off/on, clock 0–31 |
| `inst1_pitch_mod` | Pitch mod path | Previous-voice output offsets pitch counter | off/on |
| `inst1_volume`, `inst1_echo_send`, `inst1_mute` | Mixer / EchoUnit | Per-voice level, EON bit, mute | dB, off/on |
| `echo_enable`, `echo_delay`, `echo_feedback`, `echo_fir_0..7` | EchoUnit | EDL/EFB/C0–C7 | see spec |
| `master_volume` | Mixer | MVOL | -96–0 dB |
| `interpolation` | VoiceEngine | Gaussian / linear / off | enum |
| `voice_steal` | VoiceAllocator | Steal policy | enum |
| `soft_reset` | VoiceEngine | FLG bit 7 equivalent | off/on |

## Real-time constraints

- No allocation, no file I/O, no locks in `processBlock`.
- Sample bank is immutable binary data; echo buffer and resampler state allocated in `prepareToPlay`.
- Voice state is plain structs in a fixed array — no `std::map`, no per-note heap.
- Denormal protection via `juce::ScopedNoDenormals`; all internal math float, envelope accumulators clamped.

## Complexity assessment

**Score: 4/5 — Expert.**

Rationale: bit-accurate chip emulation is expert work — BRR ADPCM decode with prediction filters, the Gaussian interpolation table, the 14-bit pitch counter, envelope rate tables with hardware stepping, and voice-modulation ordering. It is **not** 5/5 because every algorithm is fully documented (DSP register maps and decoder behaviour are public, unambiguous specs) and there is no ML or novel modelling; the risk is fidelity and real-time discipline, not research.

The v0.2 table and v0.3 `.spc` export add breadth (routing, encoder, file format) without changing the engine's difficulty — which is why phasing them out of v0.1 lowers delivery risk without lowering the ceiling.
