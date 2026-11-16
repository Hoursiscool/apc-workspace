# SPC8 — Style Guide v1

**Derived from:** GENNYFL reference screenshots (layout language) + SPC8's own S-DSP palette.
**Rule:** every colour is a token in `Design/v1-ui-map.json`; nothing is styled ad hoc in markup.

## Colour

| Token | Hex | Role |
| :--- | :--- | :--- |
| `--plate` | `#c9c6d4` | Console shell (moulded grey-lavender, SNES hardware read) |
| `--plate-2` | `#b8b4c6` | Shell panels, knob faces, flip caps |
| `--surface` | `#a8a4b8` | Secondary shell surface |
| `--inset` | `#2b2836` | Dark recesses, letterbox surround |
| `--rule` | `#8f8aa2` | 1 px component borders |
| `--rule-2` | `#6d6880` | Panel seams and recessed edges |
| `--ink` | `#24222e` | Primary text and values |
| `--ink-2` | `#4c4860` | Labels, captions, inactive text |
| `--accent` | `#6a4fa3` | SNES purple — knob pointers, lit LEDs |
| `--panel-lcd` | `#7cae74` | Green LCD panel face |
| `--lcd-rule` | `#4d7749` | LCD bezel |
| `--lcd-ink` | `#1e3a24` | Label ink on LCD panels |
| `--lcd-face` / `--lcd-glow` | `#0b1410` / `#8cffb0` | Dot-matrix backplate / phosphor |
| `--mark-fill` / `--mark-shade` / `--mark-edge` | `#f3d64e` / `#8a4a12` / `#2a1608` | Wordmark fill, hard bevel, outline |
| `--grip-ink` | `#6d6880` | Corner gripper hatch |

**Discipline:** two surfaces only — **moulded shell** (grey-lavender, purple seam) and **green LCD** (instrument + preset panels, scanlined). Anything inside an LCD panel carries `kind: "lcd"` so its labels switch to `--lcd-ink`; that is what keeps 36 controls legible on green. Green is never used for body text outside an LCD face.

## Typography

- **Family:** `ui-monospace, "Cascadia Mono", Consolas, monospace` — system monospace only, no webfonts, no licensed pixel fonts.
- **Sizes (rem, 1 rem = 1 cell):** `--t-micro` 0.375 (chips), `--t-label` 0.4375 (control labels), `--t-value` 0.5625 (flip values), `--t-readout` 0.625 (LCD screens).
- **Treatment:** uppercase for all labels, `letter-spacing: .12em`, weight 600. Numeric readouts use `font-variant-numeric: tabular-nums` so values don't jitter.
- **Wordmark:** the `SPC8` label is set in the same monospace at the largest available size with heavy letter-spacing; no custom artwork, no graphic logo.

## Spacing

- Grid: **50 × 32 cells**, all positions on 0.25-cell steps.
- Panel padding: 0.5 cell minimum from section edge to first control.
- Control gutters: ≥ 0.5 cell between knob groups; knobs are 3.25–3.5 cells tall so labels never collide with dials.
- Section gaps: 1.0 cell vertical between mast and body, 0.75 cell between body and voice row.

## Controls

| Component | Look | Behaviour |
| :--- | :--- | :--- |
| `knob` | Circular dial, recessed centre, green pointer at −135°…+135°, label beneath | Drag vertical, shift for fine |
| `flip` | Rectangular cap with centred value, label beneath, recessed overlay | Click cycles detents |
| `psw` | Panel switch with LED + label; LED dim grey when off, green glow when on | Click toggles |
| `screen` | Inset LCD, green tabular text | Display only |
| `label` | Uppercase caption | Display only |
| `custom` (name) | Cap with the instrument name | Text entry in impl |
| `well` / `canvas` | Recessed slot; `led-readout` painter draws green monospace text | Display only |
| `grip` | Bottom-right hatched triangle, 2.5-cell hit area | Drag to resize (aspect locked) |

## States

- **Off:** LED at 35% opacity, label `--ink-2`.
- **On:** LED `--accent` with a soft glow, label `--ink`.
- **Hover:** border shifts to `--accent` on chips; knobs and flips brighten by ~15%.
- **Active/click:** brief brightness lift (kit default `filter: brightness(1.15)`).

## Scaling behaviour

- Modes: `scalable`, `aspect_lock: true`, `scale_range` 0.5–3.0, presets 50/75/100/150/200 %.
- `edge_policy: letterbox` with `letterbox_fill: --inset` — on a host-forced aspect mismatch the plate centres in an inset-black surround; never a stretched or margin-shifted layout.
- Text floors: labels never render below 9–10 device px (kit's `max()` floors).

## Anti-patterns (explicitly forbidden)

- `zoom`, `transform: scale()` wrappers, JS-computed scale variables, `cqw`/`cqh`, raw `px` in authored layout.
- Images, gradients-as-texture, webfonts, or GENNYFL-derived artwork/assets.
- Dead buttons for unshipped features — use the `v0.x` captions instead.
- A per-voice pan control (the hardware has none).
