# SPC8 — Creative Brief

**Phase:** plan (v0.1 scope locked)
**Name:** SPC8 (locked — directory `plugins/SPC8`, CMake target `SPC8`)
**Concept:** A hardware-faithful S-DSP (SNES) instrument in the GENNYFL mold.
**Framework:** webview · **Complexity:** 4/5 · **Strategy:** phased

---

## Hook

GENNY made the Sega Genesis' YM2612 playable inside a DAW. **SPC8 is that idea pointed at the Super Nintendo**: the eight voices of the Ricoh S-DSP — 4-bit BRR samples, Gaussian interpolation, hardware ADSR, pitch modulation, noise, and the echo that gave the console its space — with the console's limits intact, because the limits are the sound.

## Version scope (locked in plan)

| Version | Contents |
| :--- | :--- |
| **v0.1 — this build** | S-DSP engine: 8 voices, BRR decode, Gaussian interpolation, ADSR/GAIN envelopes, noise, global echo; **one instrument page** (instrument 1); **factory sample bank**; VST3 + Standalone. |
| **v0.2** | 16-instrument table with per-MIDI-channel routing, shared-voice stealing across instruments; WAV→BRR import; patch save/load. |
| **v0.3** | `.spc` export; microtuning; preset library. |

Everything below is written so v0.1 is a complete, playable instrument — not a stub that waits for the table.

## Description

- **Eight voices, shared.** The engine allocates a single instrument's notes across the DSP's eight voices with oldest-note stealing, exactly as the hardware budget forces.
- **Per-voice sound design in S-DSP vocabulary:** sample slot, PITCH (14-bit, coarse + fine), ADSR or GAIN envelope, noise enable, pitch modulation, echo send. If the chip has no register for it, SPC8 has no control for it.
- **One global echo unit.** Voices are mono; stereo exists only through echo — delay (EDL), feedback (EFB), eight-tap FIR, wet level.
- **Factory bank ships in the binary.** Original samples, no cartridge dumps, so a fresh instance plays immediately.
- **Engine clock:** the DSP runs internally at its native 32 kHz and is resampled to the host rate, so sample-rate artefacts land where the hardware put them.

## Parity map (GENNYFL → SPC8)

| GENNYFL | SPC8 |
| :--- | :--- |
| YM2612 FM: 4 operators, 8 algorithms, feedback | S-DSP voices: BRR sample + Gaussian interpolation (SNES has no FM) |
| SN76489 square/noise channels | S-DSP noise + short looped samples |
| 16 instruments, one per MIDI channel | v0.2 (v0.1: instrument 1 only) |
| 6 FM + 3 square + 1 noise voice budget | 8 shared S-DSP voices |
| Per-instrument patch editor | Single instrument page (v0.1) |
| VGM export | `.spc` export (v0.3) |
| Patch import/export | SPC8 patch format (v0.2) |

**Not copied:** GENNYFL's source, binaries, UI art, iconography or branding. Your screenshots are a layout reference only; all code and graphics are original. Nothing from GENNY's YM2612 emulation is used — different chip, different engine.

## Sonic goal

Authentic SNES: 4-bit sample quantisation, Gaussian smearing, envelope stair-stepping at hardware rate resolution, mono voices inside a wide echo field. It should sound *worse* than a modern sampler in exactly the way a SNES does.

## Non-goals

- No SPC700 CPU emulation, no `.spc` import (export only, v0.3).
- No FM, no analog modelling, no effects beyond the S-DSP echo.
- No polyphony expansion beyond eight voices — that constraint is the product.
- No general sampler features (slicing, time-stretch, velocity layers).

## Success criteria

1. Fresh instance plays the factory-init patch immediately; 8-voice polyphony with audible stealing at note 9.
2. Every v0.1 control maps to a real S-DSP register, with no invented DSP.
3. Echo delay/feedback/FIR audibly match the hardware model at 32 kHz internal rate.
4. State (all parameters) survives save/reload in the host.
