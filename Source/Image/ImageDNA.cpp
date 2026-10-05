#include "ImageDNA.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace dali
{
namespace
{
struct RGBAF { float r, g, b, a; };

std::vector<RGBAF> resample(const std::uint8_t* src, int w, int h, int ow, int oh)
{
    std::vector<RGBAF> out(size_t(ow) * oh);
    const float sx = float(w) / float(ow), sy = float(h) / float(oh);
    for (int y = 0; y < oh; ++y)
    {
        const int y0 = int(std::floor(y * sy)), y1 = std::max(y0 + 1, std::min(h, int(std::ceil((y + 1) * sy))));
        for (int x = 0; x < ow; ++x)
        {
            const int x0 = int(std::floor(x * sx)), x1 = std::max(x0 + 1, std::min(w, int(std::ceil((x + 1) * sx))));
            double r = 0, g = 0, b = 0, a = 0; int n = 0;
            for (int yy = y0; yy < y1; ++yy)
                for (int xx = x0; xx < x1; ++xx)
                {
                    const std::uint8_t* p = src + (size_t(yy) * w + xx) * 4;
                    const double al = p[3] / 255.0;
                    r += p[0] / 255.0 * al; g += p[1] / 255.0 * al; b += p[2] / 255.0 * al; a += al; ++n;
                }
            const double inv = n > 0 ? 1.0 / n : 0.0;
            out[size_t(y) * ow + x] = { float(r * inv), float(g * inv), float(b * inv), float(a * inv) };  // premultiplied
        }
    }
    return out;
}

float percentile(std::vector<float> v, float p)
{
    if (v.empty()) return 0.0f;
    const size_t k = std::min(v.size() - 1, size_t(p * float(v.size() - 1)));
    std::nth_element(v.begin(), v.begin() + long(k), v.end());
    return v[k];
}

void blur3(std::vector<float>& v, int w, int h)
{
    std::vector<float> t(v.size());
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
        {
            const int xm = std::max(0, x - 1), xp = std::min(w - 1, x + 1);
            t[size_t(y) * w + x] = 0.25f * v[size_t(y) * w + xm] + 0.5f * v[size_t(y) * w + x] + 0.25f * v[size_t(y) * w + xp];
        }
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
        {
            const int ym = std::max(0, y - 1), yp = std::min(h - 1, y + 1);
            v[size_t(y) * w + x] = 0.25f * t[size_t(ym) * w + x] + 0.5f * t[size_t(y) * w + x] + 0.25f * t[size_t(yp) * w + x];
        }
}

std::vector<float> sobel(const std::vector<float>& v, int w, int h)
{
    std::vector<float> e(v.size(), 0.0f);
    auto at = [&](int x, int y) { x = std::clamp(x, 0, w - 1); y = std::clamp(y, 0, h - 1); return v[size_t(y) * w + x]; };
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
        {
            const float gx = -at(x - 1, y - 1) - 2 * at(x - 1, y) - at(x - 1, y + 1) + at(x + 1, y - 1) + 2 * at(x + 1, y) + at(x + 1, y + 1);
            const float gy = -at(x - 1, y - 1) - 2 * at(x, y - 1) - at(x + 1, y - 1) + at(x - 1, y + 1) + 2 * at(x, y + 1) + at(x + 1, y + 1);
            e[size_t(y) * w + x] = std::sqrt(gx * gx + gy * gy);
        }
    return e;
}

// Two-pass 3-4 chamfer distance transform from seed pixels.
std::vector<float> distanceField(const std::vector<std::uint8_t>& seed, int w, int h)
{
    const float INF = 1e9f;
    std::vector<float> d(seed.size());
    for (size_t i = 0; i < seed.size(); ++i) d[i] = seed[i] ? 0.0f : INF;
    auto D = [&](int x, int y) -> float& { return d[size_t(y) * w + x]; };
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
        {
            float& c = D(x, y);
            if (x > 0) c = std::min(c, D(x - 1, y) + 3);
            if (y > 0) c = std::min(c, D(x, y - 1) + 3);
            if (x > 0 && y > 0) c = std::min(c, D(x - 1, y - 1) + 4);
            if (x < w - 1 && y > 0) c = std::min(c, D(x + 1, y - 1) + 4);
        }
    for (int y = h - 1; y >= 0; --y)
        for (int x = w - 1; x >= 0; --x)
        {
            float& c = D(x, y);
            if (x < w - 1) c = std::min(c, D(x + 1, y) + 3);
            if (y < h - 1) c = std::min(c, D(x, y + 1) + 3);
            if (x < w - 1 && y < h - 1) c = std::min(c, D(x + 1, y + 1) + 4);
            if (x > 0 && y < h - 1) c = std::min(c, D(x - 1, y + 1) + 4);
        }
    for (auto& v : d) v = v >= INF ? -1.0f : v / 3.0f;   // pixels; -1 = no seed at all
    return d;
}
} // namespace

// box blur with radius r (separable, prefix sums) — used to integrate the structure tensor
static void boxBlur(std::vector<float>& v, int w, int h, int r)
{
    if (r < 1) return;
    std::vector<float> tmp(v.size());
    std::vector<double> acc(size_t(std::max(w, h)) + 1);
    for (int y = 0; y < h; ++y)
    {
        acc[0] = 0; for (int x = 0; x < w; ++x) acc[size_t(x) + 1] = acc[size_t(x)] + v[size_t(y) * w + x];
        for (int x = 0; x < w; ++x)
        {
            const int a = std::max(0, x - r), b = std::min(w, x + r + 1);
            tmp[size_t(y) * w + x] = float((acc[size_t(b)] - acc[size_t(a)]) / (b - a));
        }
    }
    for (int x = 0; x < w; ++x)
    {
        acc[0] = 0; for (int y = 0; y < h; ++y) acc[size_t(y) + 1] = acc[size_t(y)] + tmp[size_t(y) * w + x];
        for (int y = 0; y < h; ++y)
        {
            const int a = std::max(0, y - r), b = std::min(h, y + r + 1);
            v[size_t(y) * w + x] = float((acc[size_t(b)] - acc[size_t(a)]) / (b - a));
        }
    }
}

ImageDNA ImageDNA::analyse(const std::uint8_t* rgba, int w, int h, int maxDim)
{
    ImageDNA out;
    if (rgba == nullptr || w <= 0 || h <= 0) return out;

    maxDim = std::max(16, maxDim);
    const float scale = std::min(1.0f, float(maxDim) / float(std::max(w, h)));
    const int ow = std::max(1, int(std::lround(w * scale)));
    const int oh = std::max(1, int(std::lround(h * scale)));
    const size_t N = size_t(ow) * oh;

    auto px = resample(rgba, w, h, ow, oh);

    // --- presence mask ------------------------------------------------------
    size_t translucent = 0;
    for (auto& p : px) if (p.a < 0.9f) ++translucent;
    out.hasAlpha = translucent > N / 50;   // > 2% transparent → trust alpha

    std::vector<float> presence(N);
    if (out.hasAlpha)
    {
        for (size_t i = 0; i < N; ++i) presence[i] = px[i].a;
    }
    else
    {
        // background colour = median-ish average of the border ring
        double br = 0, bg = 0, bb = 0; int n = 0;
        const int ring = std::max(1, std::min(ow, oh) / 40);
        for (int y = 0; y < oh; ++y)
            for (int x = 0; x < ow; ++x)
                if (x < ring || y < ring || x >= ow - ring || y >= oh - ring)
                { auto& p = px[size_t(y) * ow + x]; br += p.r; bg += p.g; bb += p.b; ++n; }
        br /= n; bg /= n; bb /= n;
        std::vector<float> dist(N);
        for (size_t i = 0; i < N; ++i)
        {
            const float dr = px[i].r - float(br), dg = px[i].g - float(bg), db = px[i].b - float(bb);
            dist[i] = std::sqrt(dr * dr + dg * dg + db * db);
        }
        const float hi = std::max(0.05f, percentile(dist, 0.95f));
        for (size_t i = 0; i < N; ++i) presence[i] = std::clamp(dist[i] / (hi * 0.6f), 0.0f, 1.0f);
        blur3(presence, ow, oh);
    }

    // --- luminance (contrast normalised) -------------------------------------
    std::vector<float> lum(N);
    for (size_t i = 0; i < N; ++i) lum[i] = 0.2126f * px[i].r + 0.7152f * px[i].g + 0.0722f * px[i].b;
    const float lo = percentile(lum, 0.02f), lh = percentile(lum, 0.98f);
    const float lr = std::max(1e-3f, lh - lo);
    for (auto& l : lum) l = std::clamp((l - lo) / lr, 0.0f, 1.0f);

    // --- edges -----------------------------------------------------------------
    std::vector<float> lb = lum;  blur3(lb, ow, oh);
    std::vector<float> pb = presence;
    auto e1 = sobel(lb, ow, oh), e2 = sobel(pb, ow, oh);
    std::vector<float> edge(N);
    for (size_t i = 0; i < N; ++i) edge[i] = std::max(e1[i], e2[i] * 1.2f);
    const float eh = std::max(1e-3f, percentile(edge, 0.97f));
    for (auto& e : edge) e = std::clamp(e / eh, 0.0f, 1.0f);

    // --- distance field from edges ----------------------------------------------
    std::vector<std::uint8_t> seed(N);
    for (size_t i = 0; i < N; ++i) seed[i] = edge[i] > 0.3f ? 1 : 0;
    auto dist = distanceField(seed, ow, oh);
    const float dNorm = 0.25f * float(std::max(ow, oh));
    for (auto& d : dist) d = d < 0 ? 1.0f : std::clamp(d / dNorm, 0.0f, 1.0f);

    // --- dominant colours (k-means, k = 4) --------------------------------------
    std::vector<std::array<float, 3>> samples;
    const size_t stride = std::max<size_t>(1, N / 6000);
    for (size_t i = 0; i < N; i += stride)
        if (presence[i] > 0.5f)
        {
            const float a = std::max(px[i].a, 1e-3f);
            samples.push_back({ px[i].r / a, px[i].g / a, px[i].b / a });
        }
    if (samples.size() < 8)
        for (size_t i = 0; i < N; i += stride)
        { const float a = std::max(px[i].a, 1e-3f); samples.push_back({ px[i].r / a, px[i].g / a, px[i].b / a }); }

    std::array<std::array<float, 3>, 4> cent {};
    cent[0] = samples[samples.size() / 2];
    for (int k = 1; k < 4; ++k)  // farthest-point initialisation
    {
        float best = -1; size_t bi = 0;
        for (size_t i = 0; i < samples.size(); ++i)
        {
            float md = std::numeric_limits<float>::max();
            for (int j = 0; j < k; ++j)
            {
                float d = 0; for (int c = 0; c < 3; ++c) { float t = samples[i][size_t(c)] - cent[size_t(j)][size_t(c)]; d += t * t; }
                md = std::min(md, d);
            }
            if (md > best) { best = md; bi = i; }
        }
        cent[size_t(k)] = samples[bi];
    }
    std::array<int, 4> counts {};
    for (int iter = 0; iter < 10; ++iter)
    {
        std::array<std::array<double, 3>, 4> acc {}; counts = {};
        for (auto& s : samples)
        {
            int bk = 0; float bd = std::numeric_limits<float>::max();
            for (int k = 0; k < 4; ++k)
            {
                float d = 0; for (int c = 0; c < 3; ++c) { float t = s[size_t(c)] - cent[size_t(k)][size_t(c)]; d += t * t; }
                if (d < bd) { bd = d; bk = k; }
            }
            for (int c = 0; c < 3; ++c) acc[size_t(bk)][size_t(c)] += s[size_t(c)];
            ++counts[size_t(bk)];
        }
        for (int k = 0; k < 4; ++k)
            if (counts[size_t(k)] > 0)
                for (int c = 0; c < 3; ++c) cent[size_t(k)][size_t(c)] = float(acc[size_t(k)][size_t(c)] / counts[size_t(k)]);
    }
    std::array<int, 4> order { 0, 1, 2, 3 };
    std::sort(order.begin(), order.end(), [&](int a, int b) { return counts[size_t(a)] > counts[size_t(b)]; });
    for (int k = 0; k < 4; ++k) out.palette[size_t(k)] = cent[size_t(order[size_t(k)])];

    // --- flow field: structure tensor of the luminance (+ shape) gradients -----------
    //     the dominant local orientation of the image's lines, integrated over a
    //     neighbourhood so it describes structure (contours, strands, horizons), not noise
    std::vector<float> jxx(N), jxy(N), jyy(N);
    {
        std::vector<float> src(N);
        for (size_t i = 0; i < N; ++i) src[i] = 0.75f * lb[i] + 0.25f * presence[i];
        for (int y = 0; y < oh; ++y)
            for (int x = 0; x < ow; ++x)
            {
                auto at = [&](int xx, int yy) { xx = std::clamp(xx, 0, ow - 1); yy = std::clamp(yy, 0, oh - 1); return src[size_t(yy) * ow + xx]; };
                const float gx = (at(x + 1, y - 1) + 2 * at(x + 1, y) + at(x + 1, y + 1)) - (at(x - 1, y - 1) + 2 * at(x - 1, y) + at(x - 1, y + 1));
                const float gy = (at(x - 1, y + 1) + 2 * at(x, y + 1) + at(x + 1, y + 1)) - (at(x - 1, y - 1) + 2 * at(x, y - 1) + at(x + 1, y - 1));
                const size_t i = size_t(y) * ow + x;
                jxx[i] = gx * gx; jxy[i] = gx * gy; jyy[i] = gy * gy;
            }
        const int r = std::max(2, std::max(ow, oh) / 48);
        for (auto* t : { &jxx, &jxy, &jyy }) { boxBlur(*t, ow, oh, r); boxBlur(*t, ow, oh, r); }
    }

    // nearest palette colour per pixel = region map
    auto regionOf = [&](size_t i)
    {
        const float a = std::max(px[i].a, 1e-3f);
        const float c[3] = { px[i].r / a, px[i].g / a, px[i].b / a };
        int bk = 0; float bd = std::numeric_limits<float>::max();
        for (int k = 0; k < 4; ++k)
        {
            float d = 0; for (int ch = 0; ch < 3; ++ch) { const float t = c[ch] - out.palette[size_t(k)][size_t(ch)]; d += t * t; }
            if (d < bd) { bd = d; bk = k; }
        }
        return bk;
    };

    // --- pack --------------------------------------------------------------------
    out.width = ow; out.height = oh; out.aspect = float(ow) / float(oh);
    out.dna.resize(N * 4); out.color.resize(N * 4); out.flow.resize(N * 4);
    size_t fg = 0;
    for (size_t i = 0; i < N; ++i)
    {
        out.dna[i * 4 + 0] = lum[i];
        out.dna[i * 4 + 1] = edge[i];
        out.dna[i * 4 + 2] = dist[i];
        out.dna[i * 4 + 3] = presence[i];
        out.color[i * 4 + 0] = px[i].r;
        out.color[i * 4 + 1] = px[i].g;
        out.color[i * 4 + 2] = px[i].b;
        out.color[i * 4 + 3] = presence[i];
        // tensor eigen-analysis: theta = gradient direction, tangent = theta + 90 deg
        const float a = jxx[i] - jyy[i], b2 = 2.0f * jxy[i], tr = jxx[i] + jyy[i];
        const float theta = 0.5f * std::atan2(b2, a);
        const float coh = tr > 1e-6f ? std::sqrt(a * a + b2 * b2) / tr : 0.0f;
        out.flow[i * 4 + 0] = -std::sin(theta);          // unit tangent (image x right, y down)
        out.flow[i * 4 + 1] =  std::cos(theta);
        out.flow[i * 4 + 2] = std::clamp(coh, 0.0f, 1.0f) * std::clamp(tr * 40.0f, 0.0f, 1.0f);   // flat areas -> 0
        out.flow[i * 4 + 3] = float(regionOf(i)) / 3.0f;
        if (presence[i] > 0.5f) ++fg;
    }
    out.coverage = float(fg) / float(N);
    return out;
}
} // namespace dali
