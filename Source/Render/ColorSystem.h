#pragma once
// ============================================================================
//  ColorSystem — resolves the active palette (built-in or CUSTOM) plus the
//  audio-reactive palette motion into shader uniforms.
// ============================================================================
#include "Palettes.h"
#include "ShaderProgram.h"
#include <cmath>

namespace dali
{
class ColorSystem
{
public:
    struct Inputs
    {
        int   palette = 2;
        float customHueA = 0.75f, customHueB = 0.52f;
        float colorShift = 0.0f, audioColor = 0.35f, colorAmount = 1.0f;
        float centroid = 0.0f, flux = 0.0f, kick = 0.0f;
    };

    void update(const Inputs& in, float dt)
    {
        pal = in.palette == int(PaletteId::Custom) ? makeCustomPalette(in.customHueA, in.customHueB)
                                                   : builtInPalette(in.palette);
        // audio-reactive colour: flux drives slow palette travel, centroid sets position, kicks nudge
        drift += dt * in.audioColor * (0.02f + 0.25f * in.flux);
        drift -= std::floor(drift);
        shift = in.colorShift + drift + in.audioColor * (0.35f * in.centroid + 0.05f * in.kick);
        amount = in.colorAmount;
    }

    float getDrift() const noexcept { return drift; }
    void setDrift(float d) noexcept { drift = d; }

    void apply(Shader& s) const
    {
        s.set3("uPalA", pal.a); s.set3("uPalB", pal.b); s.set3("uPalC", pal.c); s.set3("uPalD", pal.d);
        s.set("uPalShift", shift);
        s.set("uColorAmount", amount);
    }

private:
    CosinePalette pal = builtInPalette(2);
    float drift = 0.0f, shift = 0.0f, amount = 1.0f;
};
} // namespace dali
