#pragma once
// Iterative radix-2 complex FFT (framework independent, allocation-free after init).
#include <complex>
#include <vector>

namespace dali
{
class FFT
{
public:
    explicit FFT(int order = 11);
    int size() const noexcept { return n; }
    /** In-place forward transform of n complex values. */
    void forward(std::complex<float>* data) const noexcept;
private:
    int n = 0;
    std::vector<int> bitrev;
    std::vector<std::complex<float>> twiddle;
};
} // namespace dali
