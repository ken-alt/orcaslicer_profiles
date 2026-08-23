# Display-Space IDT — DaVinci Resolve DCTL

An inverse display-rendering transform. It takes **display-referred** footage —
already-graded Rec.709/P3 masters, stock, screen grabs, AI-generated frames,
delivered ProRes — and undoes the display render, so the shot can be graded like
scene-referred camera material in a wide-gamut working space.

Modelled on the UI in the reference screenshots: same six sliders, the same
Show Curve toggle, and the same Display Space / Destination Space selectors.

## Install

Copy `Display_Space_IDT.dctl` into Resolve's LUT folder, then right-click the
LUT list in the Color page and choose **Refresh** (or restart Resolve).

| OS | Path |
|---|---|
| macOS | `~/Library/Application Support/Blackmagic Design/DaVinci Resolve/LUT/` |
| Windows | `%APPDATA%\Blackmagic Design\DaVinci Resolve\Support\LUT\` |
| Linux | `~/.local/share/DaVinciResolve/LUT/` |

Apply it with a **DCTL** OFX effect on the **first node** of the clip, before any
grading. Set *Display Space* to how the footage was mastered and *Destination
Space* to your timeline colour space.

## Controls

| Control | Default | What it does |
|---|---|---|
| **Highlight Unroll** | 0.500 | Inverts the highlight rolloff. Everything below a knee is untouched; the knee→white range is expanded back out to a scene-referred peak. At 0.5, display white lands at 4.75 scene-linear; at 1.0, 16.0. |
| **Toe Uncompress** | 0.500 | Inverts the display toe `x²/(x+t)`, restoring linear shadow separation. Normalised so white stays at 1.0. |
| **Gamut Expand** | 0.500 | Inverse of the ACES-style distance-based gamut compression. Pushes saturated colours back outside the display gamut. At 1.0 it reaches the full ACES 1.3 RGC limits (cyan 1.147 / magenta 1.264 / yellow 1.312). |
| **Preserve Highlight Color** | 0.500 | Blends per-channel unroll against a ratio-locked unroll driven by a single norm. At 0 the hot channel runs away and highlights skew hue; at 1 channel ratios are held through the unroll. Gated to the unroll's own knee→white region, so shadows are never touched. |
| **Input Black** | 0.000 | Removes a black lift / flare offset before everything else. |
| **Input Contrast** | 1.000 | Removes contrast the footage already carries, pivoted on 0.18. Set it to the contrast that was baked in; the transform applies the reciprocal. |
| **Show Curve** | off | Draws the tone response in the lower-left corner. X = input display code value, Y = output plotted in DaVinci Intermediate (so the whole 0–100 scene range fits). The faint warm line marks 18% scene grey. |

With all sliders at identity (unroll/toe/expand/preserve 0, black 0, contrast 1)
the DCTL reduces to a pure colour-space conversion — verified to 1.2e-7.

### Display Space
Rec.709 with Gamma 2.4 / 2.2 / sRGB / Rec.709 OETF · P3-D65 with Gamma 2.6 / 2.4 ·
Rec.2020 with Gamma 2.4 / ST.2084. PQ is normalised so 203 nits = 1.0.

### Destination Space
DaVinci Wide Gamut / Linear · DWG / DaVinci Intermediate · ACES AP1 / ACEScct ·
ACES AP0 / Linear · Rec.709 / Linear. The ACES matrices carry a Bradford
D65→D60 adaptation.

## Notes on the maths

- Primaries matrices were derived from the published chromaticities rather than
  copied; the DWG→XYZ result reproduces Blackmagic's published matrix exactly.
- DaVinci Intermediate and ACEScct encodes match their reference values to six
  decimals (`ACEScct(0.18) = 0.413588`, `DI(0.18) = 0.336043`, `DI(100) = 1.0`).
- Negative (sub-black) and out-of-gamut values pass through rather than being
  clamped, so nothing is thrown away before the grade. ACEScct and DaVinci
  Intermediate scale negatives by their linear-segment slope — that is what
  those specs define, not a defect.
- The gamut expand only inverts inside the display gamut's own domain
  (distance ≤ 1); anything already outside is carried through with the
  boundary's offset so the function stays continuous and bounded.
- **Clipped highlights are not recoverable.** Anything at code value 1.0 was
  already flat when it arrived; the unroll gives it a plausible scene-referred
  value, it does not restore detail that was never there.

The reference tool showed a *DCTL Expiration* field. That is a licence-expiry
lock for a commercial plugin, so there is no equivalent here — this one does not
expire.

## Tests

```
./test/run_tests.sh
```

Compiles the DCTL as C++ against a shim that emulates the DCTL runtime, so it
can be exercised without Resolve. Covers: syntax, NaN/Inf and monotonicity over
the full parameter grid × all 8 display spaces (including sub-black input),
value blow-ups on random sub-black/super-white colours, hue stability through
the unroll, the identity-bypass case, and overlay pixels leaking outside their
panel.
