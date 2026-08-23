# Display-Space IDT — DaVinci Resolve DCTL

**Given an image that has already been rendered for a Rec.709 display, what
scene-referred signal would have produced it?**

That is the question this DCTL answers. It matters because the answer is only
well-posed under two conditions, and the plugin is built around both.

## The problem is only half well-posed

A display render is a composition:

```
scene linear  ──tonescale──▶  ──gamut compress──▶  ──encode──▶  display code value
```

Recovering the scene signal means inverting that composition — which has two
consequences that drive the whole design.

**1. Order is not optional.** The inverse must undo the *last* forward
operation first: `decode → gamut expand → inverse tonescale`. Doing the tone
inversion before the gamut step is not an inverse, it is a rearrangement. The
tonescale is per-channel, so it changes channel ratios, and channel ratios are
exactly what the gamut step measures. Measured on a saturated red, the two
orders differ by 0.20 and flip the sign of two channels:

| order | recovered (709 linear) |
|---|---|
| tone → gamut *(wrong)* | `[3.4808, −0.0423, −0.0659]` |
| gamut → tone *(inverse)* | `[3.4808,  0.1615,  0.1137]` |

There is a second reason the order matters: on display linear, distance 1 *is*
the display gamut boundary, so the inverse gamut mapping has a meaningful
domain. After a per-channel tonescale it no longer is.

**2. Some pixels have no unique answer.** Every tonescale asymptotes to display
white, so a clipped pixel's preimage is a *ray*, not a point — infinitely many
scene values produce that same code value. The same holds at black clip, and
where a channel was clamped to the gamut boundary. No transform recovers that
information. `Diagnostic → Recoverability` marks those pixels instead of
quietly inventing values for them:

| colour | meaning |
|---|---|
| red | all channels clipped — preimage unbounded |
| amber | one or two channels clipped — hue was clamped |
| blue | crushed to black |
| cyan | a single channel at black |
| neutral grey | unique preimage; the recovered value is the answer |

## Two modes, because the answer depends on knowing the forward render

`f⁻¹` is meaningless until you say which `f`.

**`Tone-Mapped (Daniele / ACES 2 curve)`** — when you know the source was tone
mapped with that curve family (the ACES 2.0 output transform; closely related
to Resolve's DaVinci Tone Mapping), the inverse is analytic and exact:

```
forward   f(x) = t(h(x)),   h(x) = m₂·(x/(x+s₂))^g,   t(y) = y²/(y+t₁)
```

Both halves invert in closed form. Verified: `rev(fwd(x)) = x` to **2.1e-6**
relative, and a full round trip — scene signal → forward curve → Rec.709
gamma-2.4 code value → back through the DCTL — recovers the original scene
value to **1.3e-5** relative. Set *Source Peak Luminance* to the peak the
source was mastered for (100 nits for SDR).

**`Unknown / Parametric`** — for stock, screen grabs, AI frames and anything
whose provenance you don't have. The six sliders are then an *estimate* of the
inverse, not a derivation, and should be treated as such.

### Two numbers worth knowing

- **The invertible ceiling is `t(m₂)`, not `n/(u₂·n_r)`.** They differ by 1%
  (1.018338 vs 1.008269) and the lower one is wrong — it clamps legitimate
  highlight values in between and returns finite but incorrect answers.
  Confirmed against the forward curve's actual asymptote, `fwd(1e9)`.
- **Mid grey lands 0.83% above nominal `c_d`** (10.115 nits rather than
  10.013 at a 100-nit peak). That is inherent to the published construction,
  not an implementation error: algebraically `h(0.18)/g_ip = 1/u₂` exactly.
  It is 0.012 stops, and it is *not* corrected here, so the inverse stays the
  exact inverse of the real curve.

At a 100-nit peak, legal display white recovers to **59.89** scene-linear.
Values above `SCENE_MAX` (1000) only arise past code value 1.0, where the
preimage is unbounded anyway.

## Which file

Blackmagic's DCTL spec defines exactly two legal `transform` signatures, and
only one of them carries pixel coordinates:

```c
// basic - no coordinates
__DEVICE__ float3 transform(int p_Width, int p_Height, float p_R, float p_G, float p_B)

// texture - coordinates, pixels via samplers (Resolve Studio)
__DEVICE__ float3 transform(int p_Width, int p_Height, int p_X, int p_Y,
                            __TEXTURE__ p_TexR, __TEXTURE__ p_TexG, __TEXTURE__ p_TexB)
```

| File | Signature | Show Curve |
|---|---|---|
| `Display_Space_IDT.dctl` | basic | no |
| `Display_Space_IDT_ShowCurve.dctl` | `__TEXTURE__` | yes |

**Start with `Display_Space_IDT.dctl`.** The colour maths is identical in both;
the variant only adds the overlay, and is *generated* from the basic file by
`tools/make_showcurve_variant.py` so the two cannot drift. Regenerate after any
edit to the source file:

```
python3 tools/make_showcurve_variant.py
```

## Install

Copy the `.dctl` into Resolve's LUT folder, then right-click the
LUT list in the Color page and choose **Refresh**.

| OS | Path |
|---|---|
| macOS | `~/Library/Application Support/Blackmagic Design/DaVinci Resolve/LUT/` |
| Windows | `%APPDATA%\Blackmagic Design\DaVinci Resolve\Support\LUT\` |
| Linux | `~/.local/share/DaVinciResolve/LUT/` |

Apply with a **DCTL** OFX effect on the **first node**, before any grading.

## Controls

| Control | Default | What it does |
|---|---|---|
| **Highlight Unroll** | 0.500 | *(parametric mode)* Inverts the highlight rolloff above a knee. At 0.5 display white lands at 4.75 scene-linear; at 1.0, 16.0. |
| **Toe Uncompress** | 0.500 | *(parametric mode)* Inverts a display toe `x²/(x+t)`, normalised so white stays at 1.0. |
| **Gamut Expand** | 0.500 | Inverse of ACES-style distance-based gamut compression, applied on display linear. At 1.0 it reaches the full ACES 1.3 RGC limits (cyan 1.147 / magenta 1.264 / yellow 1.312). |
| **Preserve Highlight Color** | 0.500 | Blends per-channel inversion against a ratio-locked inversion driven by a single norm. At 0 the hot channel runs away and highlights skew hue; at 1 ratios hold. Gated to the region where the inverse gains hard, so shadows never enter the ratio path. |
| **Input Black** | 0.000 | Removes a black lift / flare offset from the display signal. |
| **Input Contrast** | 1.000 | Removes contrast baked into the source, in the scene domain, pivoted on 0.18. Set it to the contrast that was applied; the reciprocal is used. |
| **Source Rendering** | Unknown | Analytic inverse vs parametric estimate (above). |
| **Source Peak Luminance** | 100 | Peak the source was mastered for. Analytic mode only. |
| **Show Curve** | off | Plots the active inverse. X = input code value, Y = output in DaVinci Intermediate so the whole 0–100 scene range fits. Faint warm line marks 18% scene grey. |
| **Diagnostic** | Off | Recoverability false-colour (above). |

With every slider at identity the DCTL reduces to a pure colour-space
conversion — verified to 1.2e-7.

### Spaces
**Source:** Rec.709 with Gamma 2.4 / 2.2 / sRGB / Rec.709 OETF · P3-D65 with
Gamma 2.6 / 2.4 · Rec.2020 with Gamma 2.4 / ST.2084 (PQ normalised so
203 nits = 1.0).
**Destination:** DaVinci Wide Gamut / Linear · DWG / DaVinci Intermediate ·
ACES AP1 / ACEScct · ACES AP0 / Linear · Rec.709 / Linear. ACES matrices carry
a Bradford D65→D60 adaptation.

## Other implementation notes

- Primaries matrices derived from published chromaticities, not copied; the
  DWG→XYZ result reproduces Blackmagic's published matrix exactly.
- DaVinci Intermediate and ACEScct encodes match reference values to six
  decimals (`ACEScct(0.18) = 0.413588`, `DI(0.18) = 0.336043`, `DI(100) = 1.0`).
- Negative and out-of-gamut values pass through rather than being clamped.
  ACEScct and DaVinci Intermediate scale negatives by their linear-segment
  slope — that is what those specs define, not a defect.
- The inverse gamut mapping only inverts inside the display gamut's own domain
  (distance ≤ 1); beyond that it carries through with the boundary's offset, so
  it stays continuous and bounded.

## Tests

```
./test/run_tests.sh
```

Compiles the DCTL as C++ against a shim emulating the DCTL runtime, so it runs
without Resolve. Covers: syntax; NaN/Inf and monotonicity across the full
parameter grid × 8 source spaces × both modes, including sub-black input;
value blow-ups on random sub-black/super-white colour; hue stability;
identity bypass; the analytic inverse property
`rev(fwd(x)) = x`; end-to-end scene recovery through a synthetic forward
render; and recoverability classification. The generated `__TEXTURE__` variant
is regenerated and separately checked for overlay containment.
