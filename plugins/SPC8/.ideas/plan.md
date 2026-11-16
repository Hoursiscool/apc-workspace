# SPC8 — Implementation Plan

**Complexity:** 4/5 · **Strategy:** phased · **Framework:** webview · **Formats:** VST3 + Standalone (CLAP opt-in: no)

## Implementation strategy

Score ≥3, so the build is phased. v0.1 is deliberately a **complete playable instrument**, not a scaffold: engine, one instrument page, factory bank. Each phase below ends in something you can load and hear.

### Phase 2.1.1 — Engine core (audible)
- [x] Plugin skeleton: `CMakeLists.txt` (VST3 + Standalone; WebView2 and the WebView editor deferred to 2.1.4), processor with generic editor
- [x] `BrrDecoder`: 9-byte block → 16 PCM samples, filters 0–3 (verified in `SPC8_SelfTest`)
- [x] `VoiceEngine`: sample position, host-rate-scaled pitch step, looping (integer 14-bit PITCH register still to do)
- [x] Gaussian interpolation (computed kernel — hardware 512-entry table still to transcribe) + linear + off
- [x] ADSR envelope with duration-mapped rates; GAIN modes and exact hardware rate tables still open (2.1.2)
- [x] Single test sample (sine) wired end-to-end: MIDI note → audible tone (`SPC8_SelfTest`: peak 0.088, RMS 0.029)

### Phase 2.1.2 — Voice pool and global stage
- [ ] `VoiceAllocator`: 8 slots, oldest/quietest/lowest-pitch stealing
- [ ] Noise LFSR with global clock
- [ ] Pitch modulation path (previous-voice ordering)
- [ ] `Mixer` + `master_volume`
- [ ] `EchoUnit`: EDL buffer, EFB feedback, 8-tap FIR, EVOL wet
- [ ] `Resampler` 32 kHz → host rate
- [ ] RT-safety pass: no allocation in `processBlock`

### Phase 2.1.3 — Factory bank and parameter layer
- [ ] Generate 16 original BRR samples (procedural synthesis + noise shaping), embed as `BinaryData`
- [ ] APVTS: 36 parameters with locked IDs
- [ ] All parameters mapped to register model, not free-form smoothing
- [ ] State save/load round-trip
- [ ] Five v0.1 presets (Init, Lead, Strings, Bass, Echo Cathedral)

### Phase 2.1.4 — Instrument page UI
- [ ] WebView page: instrument panel + global echo strip (layout from user screenshots)
- [ ] Relays before webview before attachments (APC crash trap)
- [ ] BinaryData resource provider, inline CSS/JS only
- [ ] Parameter read/write verified both directions from the page

### Phase 2.1.5 — Validation
- [ ] Build `SPC8_VST3`, load in DAW, 8-voice steal at note 9
- [ ] pluginval (binary now installed at `_tools/pluginval-bin`)
- [ ] `.vst3` installed to `C:\Program Files\Common Files\VST3`

### v0.2 — Instrument table and interchange
- [ ] 16 instruments, `inst2_*`…`inst16_*` parameters, per-MIDI-channel routing
- [ ] Shared voice pool across instruments; stealing visible in UI
- [ ] WAV → BRR encoder with import quality setting
- [ ] SPC8 patch format save/load

### v0.3 — Hardware interchange
- [ ] `.spc` export (snapshot writer, sample directory, DSP registers)
- [ ] Microtuning
- [ ] Extended preset library

## Dependencies

**JUCE modules:** `juce_audio_basics`, `juce_audio_processors`, `juce_audio_utils`, `juce_dsp` (resampler, envelopes not used — hardware envelope is custom), `juce_gui_extra` (WebView2), `juce_audio_formats` (v0.2 WAV import), `juce_data_structures` (APVTS).

**External:** none. No third-party DSP libraries, no network, no sampler engine. Sample generation is offline tooling committed as BinaryData.

**APC integration:** plugin CMakeLists from `templates/CMakeLists.txt.template` (webview flavour), `juce_add_binary_data` for UI + sample bank.

## Risk assessment

**High risk**
- **BRR decoder correctness** — wrong filter state or nibble order produces aliasing/DC drift. Mitigation: unit-test decode of known BRR patterns against hand-computed PCM early, before UI work.
- **Gaussian table fidelity** — a wrong table sounds subtly "not SNES". Mitigation: hardware table values transcribed and spot-checked; A/B against known-accuracy playback.
- **Pitch counter precision** — 14-bit PITCH at 32 kHz; integer overflow or wrong fractional handling detunes the top octave. Mitigation: pitch sweep test across MIDI notes 0–127.
- **Echo feedback stability** — high EFB with FIR gain > 1 can run away. Mitigation: hardware-style clamping plus output limiter at the mix bus.

**Medium risk**
- **Envelope stepping vs host expectation** — users may expect smooth ramps; hardware stair-steps. Mitigation: document it, expose interpolation for the sample path only.
- **Resampler quality** — 32 kHz → 48/96 kHz can alias if the interpolator is undersized. Mitigation: Lagrange order chosen in `prepareToPlay` from host rate.
- **WebView parameter binding at scale** — 36 params through relays; wrong member order crashes DAWs. Mitigation: follow the APC template ordering exactly.

**Low risk**
- CMake/target setup (proven with the toolchain smoke test)
- Echo UI strip, presets, state save/load

## Acceptance criteria for v0.1

1. `cmake --build build --config Release --target SPC8_VST3` exits 0.
2. Loads in a DAW; plays the Init preset immediately; 8-voice polyphony with an audible steal at note 9.
3. All 36 parameters appear, automate, and persist across save/reload.
4. pluginval passes at the project's normal strictness.
5. No allocation or file access inside `processBlock` (verified by inspection of the audio path).
