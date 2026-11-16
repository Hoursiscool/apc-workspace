# SPC8 — Parameter Specification (v0.1)

**Scope:** engine + one instrument page + factory bank. Instrument table, WAV import, patch I/O and `.spc` export are v0.2/v0.3.

**ID stability:** voice parameters are named `inst1_*` even though v0.1 exposes only instrument 1. When the v0.2 table adds instruments 2–16 they become `inst2_*`…`inst16_*`, so existing automation and presets keep working instead of being renamed.

**Rule:** every control maps to a real Ricoh S-DSP register or a necessary host wrapper.

---

## 1. Global — S-DSP control registers (16)

| ID | Name | Type | Range | Default | Unit | Hardware |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `master_volume` | Master Volume | Float | -96.0 – 0.0 | -6.0 | dB | MVOL |
| `echo_enable` | Echo Enable | Bool | off / on | off | — | FLG bit 5 |
| `echo_delay` | Echo Delay | Int | 0 – 15 | 4 | ×16 ms | EDL |
| `echo_feedback` | Echo Feedback | Float | -1.0 – 1.0 | 0.35 | — | EFB |
| `echo_fir_0` | FIR Tap 0 | Float | -1.0 – 1.0 | 0.5 | — | C0 |
| `echo_fir_1` | FIR Tap 1 | Float | -1.0 – 1.0 | -0.5 | — | C1 |
| `echo_fir_2` | FIR Tap 2 | Float | -1.0 – 1.0 | 0.25 | — | C2 |
| `echo_fir_3` | FIR Tap 3 | Float | -1.0 – 1.0 | 0.0 | — | C3 |
| `echo_fir_4` | FIR Tap 4 | Float | -1.0 – 1.0 | 0.0 | — | C4 |
| `echo_fir_5` | FIR Tap 5 | Float | -1.0 – 1.0 | 0.0 | — | C5 |
| `echo_fir_6` | FIR Tap 6 | Float | -1.0 – 1.0 | 0.0 | — | C6 |
| `echo_fir_7` | FIR Tap 7 | Float | -1.0 – 1.0 | 0.0 | — | C7 |
| `noise_rate` | Noise Clock | Int | 0 – 31 | 16 | — | FLG bits 0–4 |
| `interpolation` | Interpolation | Enum | gaussian / linear / off | gaussian | — | FLG bit 4 |
| `soft_reset` | Soft Reset | Bool | off / on | off | — | FLG bit 7 |
| `voice_steal` | Voice Steal Policy | Enum | oldest / quietest / lowest_pitch | oldest | — | host wrapper |

**No per-voice pan:** S-DSP voices are mono; stereo comes only from the echo stage (EVOL). This is deliberate, not an omission.

## 2. Instrument 1 — voice registers (20)

| ID | Name | Type | Range | Default | Unit | Hardware |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `inst1_name` | Instrument Name | Text | 16 chars | "Init" | — | wrapper |
| `inst1_midi_channel` | MIDI Channel | Int | 1 – 16 | 1 | — | wrapper (v0.2 active) |
| `inst1_mute` | Mute | Bool | off / on | off | — | wrapper |
| `inst1_sample_slot` | Sample Slot | Int | 0 – 15 | 0 | — | SRCN |
| `inst1_root_note` | Root Note | Int | 0 – 127 | 60 | MIDI | wrapper |
| `inst1_tune` | Coarse Tune | Int | -48 – +48 | 0 | semitones | PITCH coarse |
| `inst1_fine` | Fine Tune | Float | -100 – +100 | 0.0 | cents | PITCH fine |
| `inst1_loop` | Sample Loop | Bool | off / on | on | — | SRCN loop bit |
| `inst1_volume` | Voice Volume | Float | -96.0 – 0.0 | 0.0 | dB | VxVOL |
| `inst1_attack` | Attack Rate | Int | 0 – 15 | 12 | rate | ADSR1 bits 0–3 |
| `inst1_decay` | Decay Rate | Int | 0 – 7 | 4 | rate | ADSR1 bits 4–6 |
| `inst1_sustain_level` | Sustain Level | Int | 0 – 7 | 5 | level | ADSR1 SL |
| `inst1_sustain_rate` | Sustain Rate | Int | 0 – 31 | 8 | rate | ADSR2 SR |
| `inst1_release` | Release Rate | Int | 0 – 31 | 12 | rate | ADSR2 RR |
| `inst1_envelope_mode` | Envelope Mode | Enum | adsr / gain | adsr | — | ADSR1 bit 7 |
| `inst1_gain_mode` | Gain Mode | Enum | direct / linear_dec / linear_inc / bent_line | direct | — | GAIN bits 5–7 |
| `inst1_gain_value` | Gain | Int | 0 – 127 | 96 | level | GAIN bits 0–4 |
| `inst1_echo_send` | Echo Send | Bool | off / on | on | — | EON bit 0 |
| `inst1_pitch_mod` | Pitch Modulation | Bool | off / on | off | — | PMON bit 0 |
| `inst1_noise_enable` | Noise Voice | Bool | off / on | off | — | NON bit 0 |

## 3. v0.1 total

**36 exposed parameters** (16 global + 20 instrument) — a single instrument page, no table yet.

## 4. v0.1 factory bank (16 slots, original material)

| Slot | Sample | Notes |
| :--- | :--- | :--- |
| 0 | Init sine | 1 cycle loop, the "Init" patch's source |
| 1 | Bright saw | 4-bit grit intentional |
| 2 | Pulse 25% | |
| 3 | Pulse 12.5% | |
| 4 | Organ | additive, looped |
| 5 | Strings | slow-bloom loop |
| 6 | Flute-ish | breathy loop |
| 7 | Bass | low, fast-decay loop |
| 8 | Marimba | percussive, non-looping |
| 9 | Kick | percussive |
| 10 | Snare | noise-seeded |
| 11 | Hat | short noise |
| 12 | Choir | formant-ish loop |
| 13 | Bell | inharmonic |
| 14 | Brass-ish | looped |
| 15 | Noise texture | for `inst1_noise_enable` layering |

All generated procedurally or original recordings — **no cartridge dumps**.

## 5. Deferred to later versions (kept out of v0.1 code)

- Instruments 2–16 with per-channel MIDI routing: **v0.2**
- `sample_bank` management, `brr_quality` import setting, WAV→BRR conversion: **v0.2**
- `patch_load` / `patch_save` (SPC8 patch format): **v0.2**
- `song_export_spc` (`.spc` export): **v0.3**
- Microtuning: **v0.3**
- Preset library beyond Init/Lead/Strings/Bass/Echo: **v0.2**

## 6. v0.1 presets

1. **Init** — slot 0 sine, ADSR, no echo.
2. **Lead** — slot 1 saw, pitch-mod on, echo 60 ms.
3. **Strings** — slot 5, slow attack, linear-decay gain mode, wide echo.
4. **Bass** — slot 7, low root, fast decay, echo send off.
5. **Echo Cathedral** — any slot, max delay, high feedback, dense FIR.
