#!/usr/bin/env python3
"""Generate Display_Space_IDT_ShowCurve.dctl from Display_Space_IDT.dctl.

The two published Transform DCTL signatures are mutually exclusive:

    basic    transform(int p_Width, int p_Height, float p_R, float p_G, float p_B)
    texture  transform(int p_Width, int p_Height, int p_X, int p_Y,
                       __TEXTURE__ p_TexR, __TEXTURE__ p_TexG, __TEXTURE__ p_TexB)

Only the texture form carries pixel coordinates, so the curve overlay needs it.
Rather than maintain two copies of ~450 lines of colour maths, the variant is
generated from the basic file: identical maths, different entry point.
"""
import os, sys, re

HERE = os.path.dirname(os.path.abspath(__file__))
SRC  = os.path.join(HERE, '..', 'Display_Space_IDT.dctl')
DST  = os.path.join(HERE, '..', 'Display_Space_IDT_ShowCurve.dctl')

CURVE_DEFINE = """// Set to 0 if the curve overlay lands in the wrong corner / appears flipped.
// DCTL's p_Y origin is not documented; flip this if the panel is misplaced.
#define CURVE_ORIGIN_TOP_LEFT   1

"""

SHOWCURVE_PARAM = "DEFINE_UI_PARAMS(p_ShowCurve, Show Curve,              DCTLUI_CHECK_BOX, 1)\n\n"

LINE_ALPHA = """// ---------------------------------------------------------------------------
//  curve overlay
// ---------------------------------------------------------------------------

__DEVICE__ float line_alpha(float dist, float halfwidth)
{
    return _clampf(1.0f - (dist - halfwidth) / _fmaxf(halfwidth, 1e-6f), 0.0f, 1.0f);
}

"""

TEX_SIGNATURE = """__DEVICE__ float3 transform(int p_Width, int p_Height, int p_X, int p_Y,
                            __TEXTURE__ p_TexR, __TEXTURE__ p_TexG, __TEXTURE__ p_TexB)
{
    float p_R = _tex2D(p_TexR, p_X, p_Y);
    float p_G = _tex2D(p_TexG, p_X, p_Y);
    float p_B = _tex2D(p_TexB, p_X, p_Y);
"""

OVERLAY = """
    // ---- curve overlay ------------------------------------------------------
    if (p_ShowCurve)
    {
        float pw   = (float)p_Width;
        float ph   = (float)p_Height;
        float side = 0.30f * pw;
        float marg = 0.03f * pw;

        float px = (float)p_X;
#if CURVE_ORIGIN_TOP_LEFT
        float py = ph - (float)p_Y;      // measure up from the bottom
#else
        float py = (float)p_Y;
#endif
        float x0 = marg;
        float y0 = marg;

        if (px >= x0 && px <= x0 + side && py >= y0 && py <= y0 + side)
        {
            float gx = (px - x0) / side;          // input  0..1 display code value
            float gy = (py - y0) / side;          // output 0..1 (DaVinci Intermediate)
            float ppx = 1.0f / side;              // one pixel in graph units

            // background + border
            float3 plot = make_float3(0.015f, 0.015f, 0.015f);
            float edge = _fminf(_fminf(gx, 1.0f - gx), _fminf(gy, 1.0f - gy));
            if (edge < 3.0f * ppx) plot = make_float3(0.12f, 0.12f, 0.12f);

            // grid every 0.25
            float gxm = _fabs(gx * 4.0f - _floor(gx * 4.0f + 0.5f)) / 4.0f;
            float gym = _fabs(gy * 4.0f - _floor(gy * 4.0f + 0.5f)) / 4.0f;
            if (gxm < ppx || gym < ppx) plot = make_float3(0.05f, 0.05f, 0.05f);

            // 18% scene grey reference
            float mid = lin_to_di(0.18f);
            if (_fabs(gy - mid) < 1.2f * ppx) plot = make_float3(0.10f, 0.09f, 0.05f);

            // the transfer curve itself, plotted in DaVinci Intermediate so the
            // whole 0..100 scene range is visible
            float e1 = decode_eotf(make_float3(gx, gx, gx), p_Display).x;
            float e2 = decode_eotf(make_float3(gx + ppx, gx + ppx, gx + ppx), p_Display).x;
            float c1 = _clampf(lin_to_di(invert_tone(e1, p_Source, dp, p_Black, p_Toe, p_Unroll, p_Contrast)), 0.0f, 1.0f);
            float c2 = _clampf(lin_to_di(invert_tone(e2, p_Source, dp, p_Black, p_Toe, p_Unroll, p_Contrast)), 0.0f, 1.0f);

            float slope = (c2 - c1) / ppx;
            float dist  = _fabs(gy - c1) / _sqrtf(1.0f + slope * slope);
            float a     = line_alpha(dist, 1.5f * ppx);

            plot = make_float3(_mix(plot.x, 0.9f, a), _mix(plot.y, 0.9f, a), _mix(plot.z, 0.9f, a));

            out = encode_dest(plot, p_Dest);
        }
    }

"""

def main():
    s = open(SRC).read()

    s = s.replace(
        "//  Signature: the published basic Transform DCTL form\n"
        "//    transform(int p_Width, int p_Height, float p_R, float p_G, float p_B)\n"
        "//  which carries no pixel coordinates, so the Show Curve overlay lives in the\n"
        "//  companion Display_Space_IDT_ShowCurve.dctl built on the __TEXTURE__ form.",
        "//  Signature: the published __TEXTURE__ Transform DCTL form, which is the\n"
        "//  only one carrying pixel coordinates and so the only one that can draw the\n"
        "//  Show Curve overlay. Requires DaVinci Resolve Studio. If it will not build,\n"
        "//  use Display_Space_IDT.dctl - identical maths, no overlay.\n"
        "//\n"
        "//  GENERATED from Display_Space_IDT.dctl by tools/make_showcurve_variant.py\n"
        "//  Do not edit directly; edit the source file and regenerate.")

    # curve-only define goes above the UI params
    anchor = "DEFINE_UI_PARAMS(p_Unroll,"
    s = s.replace(anchor, CURVE_DEFINE + anchor, 1)

    # Show Curve checkbox after the six sliders
    anchor = "DEFINE_UI_PARAMS(p_Display,   Display Space,"
    s = s.replace(anchor, SHOWCURVE_PARAM + anchor, 1)

    # overlay helper before the transform banner
    anchor = "// ---------------------------------------------------------------------------\n\n__DEVICE__ float3 transform"
    s = s.replace(anchor, LINE_ALPHA + anchor, 1)

    # swap the entry point
    s = s.replace(
        "__DEVICE__ float3 transform(int p_Width, int p_Height, float p_R, float p_G, float p_B)\n{\n",
        TEX_SIGNATURE, 1)

    # splice the overlay in just before the final return
    idx = s.rindex("    return out;\n}")
    s = s[:idx] + OVERLAY + s[idx:]

    open(DST, 'w').write(s)

    bad = [c for c in s if ord(c) > 127]
    if bad:
        print("ERROR: non-ASCII in generated file", file=sys.stderr); return 1
    print("wrote %s (%d bytes, pure ASCII)" % (os.path.basename(DST), len(s)))
    return 0

if __name__ == '__main__':
    sys.exit(main())
