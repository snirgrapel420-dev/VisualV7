#pragma once
// ============================================================================
//  ImageDNA — framework-independent image structure extraction.
//  Pure C++17 (no JUCE) so it is unit-testable and reusable by tools.
//
//  Produces the "DNA" of an image that the Template generator turns into
//  generative structure:
//     dna   RGBA float  R = luminance (contrast-normalised)
//                       G = edge strength (Sobel, normalised)
//                       B = distance to nearest edge (0..1)
//                       A = presence (foreground vs. background)
//     color RGBA float  source colour composited on black, A = presence
//     palette           4 dominant foreground colours (k-means)
// ============================================================================
#include <array>
#include <cstdint>
#include <vector>

namespace dali
{
struct ImageDNA
{
    int   width  = 0;
    int   height = 0;
    float aspect = 1.0f;          // width / height
    float coverage = 0.0f;        // fraction of pixels that are foreground
    bool  hasAlpha = false;       // source used transparency for the mask
    std::vector<float> dna;       // width * height * 4
    std::vector<float> color;     // width * height * 4
    std::vector<float> flow;      // width * height * 4: RG = contour direction (unit tangent, along the
                                  // image's lines), B = coherence (0 flat/noisy .. 1 strongly oriented),
                                  // A = region (index of the nearest palette colour / 3)
    std::array<std::array<float, 3>, 4> palette {};

    bool isValid() const noexcept { return width > 0 && height > 0 && dna.size() == size_t(width) * height * 4; }

    /** Analyses an 8-bit RGBA image (top row first). The result is resampled so
        that its longest side is at most maxDim. Thread-safe (no shared state). */
    static ImageDNA analyse(const std::uint8_t* rgba, int w, int h, int maxDim = 1024);
};
} // namespace dali
