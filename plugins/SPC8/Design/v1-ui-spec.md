# SPC8 — UI Specification v1

**Phase:** design · **Framework:** webview · **Plate:** 50 × 32 cells (800 × 512 px at 16 px/cell) · **Mode:** scalable, aspect-locked
**Reference:** three GENNYFL screenshots supplied by the user (layout language only — no artwork, logo or code reused).

## Layout

| Region | Cell rect | Contents |
| :--- | :--- | :--- |
| **Mast** | (0, 0, 50, 4.25) | Wordmark, console subtitle, LED preset readout, stereo note, active-voice readout |
| **Echo / Global** | (0.5, 4.75, 15.25, 15) | Echo on, delay, feedback, 8 FIR taps, noise clock, interpolation, soft reset |
| **Instrument 01** | (16.25, 4.75, 18, 15) | Name, MIDI channel, mute, sample slot, root, loop, volume, tune, fine, echo send, pitch mod, noise, envelope mode |
| **Presets** | (34.75, 4.75, 14.75, 15) | Five v0.1 factory presets; four deferred-feature captions |
| **Voice / Envelope** | (0.5, 20.25, 49, 11.25) | ATK/DR1/SUS/DR2/RR, gain mode + value, master, steal policy, three LCD readouts |

Window is authored at 50 × 32 cells; scaling is pure CSS (`1 rem = 1 cell`), aspect locked with an in-page corner gripper. No JS geometry.

## Controls (36 parameters — all from `parameter-spec.md`)

| Parameter | Type | Position (cells) | Label |
| :--- | :--- | :--- | :--- |
| `echo_enable` | psw | 1, 5.25 | ECHO |
| `echo_delay` | knob | 6, 5.25 | DELAY |
| `echo_feedback` | knob | 10.5, 5.25 | FB |
| `echo_fir_0`…`echo_fir_7` | knob ×8 | 0.75…13, 9.5 | — |
| `noise_rate` | knob | 1, 13.5 | NOISE |
| `interpolation` | flip | 4.75, 13.75 | INTERP (GAUSS/LINEAR/OFF) |
| `soft_reset` | psw | 9.75, 13.75 | RESET |
| `inst1_name` | custom (text) | 16.75, 5.25 | NAME |
| `inst1_midi_channel` | flip | 25.25, 5.25 | MIDI |
| `inst1_mute` | psw | 29.75, 5.25 | MUTE |
| `inst1_sample_slot` | flip | 16.75, 8.5 | SAMPLE |
| `inst1_root_note` | knob | 20.75, 8.25 | ROOT |
| `inst1_loop` | psw | 24.5, 8.75 | LOOP |
| `inst1_volume` | knob | 29.5, 8.25 | VOL |
| `inst1_tune` | knob | 16.75, 12.25 | TUNE |
| `inst1_fine` | knob | 20.5, 12.25 | FINE |
| `inst1_echo_send` | psw | 24.5, 12.5 | ECHO SEND |
| `inst1_pitch_mod` | psw | 24.5, 15.5 | PITCH MOD |
| `inst1_noise_enable` | psw | 29.75, 12.5 | NOISE |
| `inst1_envelope_mode` | flip | 29.75, 15.5 | ENV (ADSR/GAIN) |
| `inst1_attack` | knob | 1.25, 21.75 | ATK |
| `inst1_decay` | knob | 5.25, 21.75 | DR1 |
| `inst1_sustain_level` | knob | 9.25, 21.75 | SUS |
| `inst1_sustain_rate` | knob | 13.25, 21.75 | DR2 |
| `inst1_release` | knob | 17.25, 21.75 | RR |
| `inst1_gain_mode` | flip | 21.5, 21.75 | GAIN MODE |
| `inst1_gain_value` | knob | 27.25, 21.75 | GAIN |
| `master_volume` | knob | 31.5, 21.75 | MASTER |
| `voice_steal` | flip | 35.75, 21.75 | STEAL |

**Display-only elements (no parameter):** `preset-readout` and `voice-readout` canvases in the mast; `voice-count`, `clock-readout`, `engine-readout` LCD screens; brand/subtitle/stereo labels; five preset name rows; four deferred-feature captions marked `v0.2`/`v0.3`.

## Color palette

| Token | Hex | Use |
| :--- | :--- | :--- |
| `--plate` | `#c9c6d4` | console shell (moulded grey-lavender) |
| `--plate-2` | `#b8b4c6` | shell panels, control faces |
| `--surface` | `#a8a4b8` | secondary shell surface |
| `--inset` | `#2b2836` | dark recesses, letterbox surround |
| `--rule` | `#8f8aa2` | control borders |
| `--rule-2` | `#6d6880` | panel seams |
| `--ink` | `#24222e` | primary text |
| `--ink-2` | `#4c4860` | labels, captions |
| `--accent` | `#6a4fa3` | SNES purple: knob pointers, lit LEDs |
| `--panel-lcd` | `#7cae74` | green LCD panel face (sections of kind `lcd`) |
| `--lcd-rule` | `#4d7749` | LCD bezel |
| `--lcd-ink` | `#1e3a24` | dark ink for labels sitting on LCD |
| `--lcd-face` | `#0b1410` | dot-matrix backplate |
| `--lcd-glow` | `#8cffb0` | dot-matrix phosphor |
| `--mark-fill` / `--mark-shade` / `--mark-edge` | `#f3d64e` / `#8a4a12` / `#2a1608` | pixel wordmark fill, bevel, outline |

## Style notes

- **Position language from GENNYFL:** gold-wordmark-left / big LED readout / green sub-panels / knob strips along the bottom row of the plate. Rebuilt with SPC8's palette; no GENNY artwork, font files or iconography is reproduced.
- **Green LCD panels are back** (design v2 revision): the instrument and preset panels are green LCD faces with scanlines, exactly like GENNY's FM/SQ block — but control labels inside them switch to `--lcd-ink` so 36 controls stay readable. Shell panels stay moulded grey with a purple accent, which is the SNES console read.
- **Panel seams, not gradients.** Depth comes from `--rule-2` hairlines and the kit's recessed shadow; no textures, no images, no fonts beyond the system monospace stack.
- **Deferred features are visibly captioned, never rendered as dead buttons** — `v0.2` / `v0.3` labels sit where the instrument table and import bank will land, so the layout reserves their space honestly.
- **No per-voice pan control** appears anywhere: S-DSP voices are mono, and the UI must not promise otherwise.

## Open items

- Instrument-table page (16 instruments) is designed as **v0.2**, reusing this grid: the presets column becomes the instrument table.
- Envelope graph canvas is planned alongside impl (the kit has no envelope painter yet); v1 shows numeric parameters only.
