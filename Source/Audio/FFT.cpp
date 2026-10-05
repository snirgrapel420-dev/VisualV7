#include "FFT.h"
#include <cmath>

namespace dali
{
FFT::FFT(int order) : n(1 << order), bitrev(size_t(n)), twiddle(size_t(n / 2))
{
    for (int i = 0; i < n; ++i)
    {
        int r = 0;
        for (int b = 0; b < order; ++b) if (i & (1 << b)) r |= 1 << (order - 1 - b);
        bitrev[size_t(i)] = r;
    }
    for (int i = 0; i < n / 2; ++i)
        twiddle[size_t(i)] = std::polar(1.0f, float(-2.0 * 3.14159265358979323846 * i / n));
}

void FFT::forward(std::complex<float>* d) const noexcept
{
    for (int i = 0; i < n; ++i)
        if (i < bitrev[size_t(i)]) std::swap(d[i], d[bitrev[size_t(i)]]);
    for (int len = 2; len <= n; len <<= 1)
    {
        const int half = len >> 1, step = n / len;
        for (int i = 0; i < n; i += len)
            for (int j = 0; j < half; ++j)
            {
                const std::complex<float> t = twiddle[size_t(j * step)] * d[i + j + half];
                d[i + j + half] = d[i + j] - t;
                d[i + j] += t;
            }
    }
}
} // namespace dali
