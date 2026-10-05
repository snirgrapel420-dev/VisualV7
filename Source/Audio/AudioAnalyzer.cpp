#include "AudioAnalyzer.h"

namespace dali
{
AudioAnalyzer::AudioAnalyzer() : juce::Thread("DaliVisual Analysis")
{
    startThread(juce::Thread::Priority::high);
}

AudioAnalyzer::~AudioAnalyzer()
{
    stopThread(2000);
}

void AudioAnalyzer::prepare(double hostSampleRate)
{
    if (hostSampleRate > 0) hostRate.store(hostSampleRate);
    ++rateVersion;
}

void AudioAnalyzer::setExternalSampleRate(double sr)
{
    if (sr > 0) extRate.store(sr);
    ++rateVersion;
}

void AudioAnalyzer::pushInto(SpscRing<Frame>& ring, const float* left, const float* right, int numSamples) noexcept
{
    Frame tmp[256];
    int pos = 0;
    while (pos < numSamples)
    {
        const int n = juce::jmin(256, numSamples - pos);
        for (int i = 0; i < n; ++i) tmp[i] = { left[pos + i], right != nullptr ? right[pos + i] : left[pos + i] };
        ring.push(tmp, size_t(n));            // if the analyzer falls behind, excess is dropped (never blocks)
        pos += n;
    }
}

void AudioAnalyzer::push(const float* l, const float* r, int n) noexcept         { pushInto(hostRing, l, r, n); }
void AudioAnalyzer::pushExternal(const float* l, const float* r, int n) noexcept { pushInto(extRing, l, r, n); }

AudioAnalyzer::Snapshot AudioAnalyzer::snapshot() const
{
    const juce::SpinLock::ScopedLockType sl(lock);
    return latest;
}

void AudioAnalyzer::run()
{
    constexpr int H = FeatureExtractor::hopSize;
    Frame frames[H];
    float L[H], R[H];
    int seenRateVersion = -1, activeSource = -1;

    while (!threadShouldExit())
    {
        const int src = source.load();
        const int rv = rateVersion.load();
        if (src != activeSource || rv != seenRateVersion)
        {
            activeSource = src;
            seenRateVersion = rv;
            extractor.prepare(src == int(Source::External) ? extRate.load() : hostRate.load());
        }

        auto& ring  = src == int(Source::External) ? extRing : hostRing;
        auto& other = src == int(Source::External) ? hostRing : extRing;
        other.clear();                                  // consumer-side: discard the unused source

        bool worked = false;
        while (ring.available() >= size_t(H) && !threadShouldExit())
        {
            ring.pop(frames, size_t(H));
            for (int i = 0; i < H; ++i) { L[i] = frames[i].l; R[i] = frames[i].r; }
            extractor.setSensitivity(sensitivity.load());
            extractor.processHop(L, R);

            const juce::SpinLock::ScopedLockType sl(lock);
            latest.features = extractor.features();
            latest.stamp = now();
            worked = true;
        }
        if (!worked) wait(2);
    }
}
} // namespace dali
