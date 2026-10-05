#pragma once
// ============================================================================
//  Palettes — cosine-gradient definitions (colour = a + b·cos(2π(c·t + d))).
//  Framework independent: shared by the engine and the shader harness.
// ============================================================================
#include <array>
#include <cmath>

namespace dali
{
struct CosinePalette { float a[3], b[3], c[3], d[3]; };

enum class PaletteId { Mono = 0, Acid, DeepPurple, RedBlack, BlueBlack, Cyan, Earth, Psychedelic, Custom, count };

inline const char* const* paletteNames()
{
    static const char* names[] = { "MONO", "ACID", "DEEP PURPLE", "RED / BLACK", "BLUE / BLACK", "CYAN", "EARTH", "PSYCHEDELIC", "CUSTOM" };
    return names;
}

inline const CosinePalette& builtInPalette(int index)
{
    static const CosinePalette table[] = {
        { { .45f, .45f, .47f }, { .45f, .45f, .45f }, { .5f, .5f, .5f }, { .00f, .00f, .00f } }, // MONO
        { { .45f, .55f, .15f }, { .45f, .45f, .20f }, { 1.f, 1.f, 1.f }, { .60f, .52f, .30f } }, // ACID
        { { .40f, .17f, .55f }, { .40f, .20f, .45f }, { 1.f, 1.f, 1.f }, { .00f, .12f, .22f } }, // DEEP PURPLE
        { { .45f, .10f, .08f }, { .55f, .12f, .10f }, { .8f, .8f, .8f }, { .00f, .05f, .08f } }, // RED / BLACK
        { { .10f, .22f, .50f }, { .12f, .25f, .50f }, { .8f, .8f, .8f }, { .10f, .05f, .00f } }, // BLUE / BLACK
        { { .20f, .55f, .60f }, { .20f, .45f, .42f }, { 1.f, 1.f, 1.f }, { .55f, .50f, .48f } }, // CYAN
        { { .45f, .35f, .22f }, { .35f, .28f, .18f }, { 1.f, 1.f, 1.f }, { .00f, .08f, .18f } }, // EARTH
        { { .50f, .40f, .55f }, { .50f, .45f, .45f }, { 1.f, 1.f, 1.f }, { .80f, .10f, .40f } }, // PSYCHEDELIC
    };
    const int n = int(sizeof(table) / sizeof(table[0]));
    return table[index < 0 ? 0 : (index >= n ? n - 1 : index)];
}

/** HSV (0..1) → RGB */
inline std::array<float, 3> hsvToRgb(float h, float s, float v)
{
    h = h - std::floor(h);
    const float i = std::floor(h * 6.0f), f = h * 6.0f - i;
    const float p = v * (1 - s), q = v * (1 - f * s), t = v * (1 - (1 - f) * s);
    switch (int(i) % 6)
    {
        case 0: return { v, t, p }; case 1: return { q, v, p }; case 2: return { p, v, t };
        case 3: return { p, q, v }; case 4: return { t, p, v }; default: return { v, p, q };
    }
}

/** CUSTOM: dark base hue → bright highlight hue. t=0 highlight, t=0.5 deep shadow. */
inline CosinePalette makeCustomPalette(float baseHue, float highlightHue)
{
    auto A = hsvToRgb(baseHue, 0.85f, 0.35f);
    auto B = hsvToRgb(highlightHue, 0.75f, 1.0f);
    CosinePalette p {};
    for (int i = 0; i < 3; ++i)
    {
        p.a[i] = 0.5f * (B[size_t(i)] + A[size_t(i)] * 0.25f);
        p.b[i] = 0.5f * (B[size_t(i)] - A[size_t(i)] * 0.25f);
        p.c[i] = 1.0f;
        p.d[i] = 0.0f;
    }
    return p;
}
} // namespace dali
