#pragma once
#if defined(_MSC_VER)
 #pragma warning(disable: 4324)   // padding from alignas() is intentional (no false sharing)
#endif
// ============================================================================
//  SpscRing — wait-free single-producer / single-consumer ring buffer.
//  Producer: audio thread (never blocks, never allocates; drops on overflow).
//  Consumer: analysis thread.
// ============================================================================
#include <atomic>
#include <cstddef>
#include <vector>

namespace dali
{
template <typename T>
class SpscRing
{
public:
    explicit SpscRing(size_t capacityPow2 = 1u << 15) { resize(capacityPow2); }

    /** Not thread safe — call only while neither side is running. */
    void resize(size_t capacityPow2)
    {
        size_t c = 1; while (c < capacityPow2) c <<= 1;
        buffer.assign(c, T {}); mask = c - 1;
        head.store(0); tail.store(0);
    }

    size_t capacity() const noexcept { return buffer.size(); }

    /** Producer. Returns the number of items actually written. */
    size_t push(const T* data, size_t n) noexcept
    {
        const size_t h = head.load(std::memory_order_relaxed);
        const size_t t = tail.load(std::memory_order_acquire);
        const size_t free = buffer.size() - (h - t);
        if (n > free) n = free;
        for (size_t i = 0; i < n; ++i) buffer[(h + i) & mask] = data[i];
        head.store(h + n, std::memory_order_release);
        return n;
    }

    /** Consumer. */
    size_t available() const noexcept
    {
        return head.load(std::memory_order_acquire) - tail.load(std::memory_order_relaxed);
    }

    size_t pop(T* out, size_t n) noexcept
    {
        const size_t t = tail.load(std::memory_order_relaxed);
        const size_t h = head.load(std::memory_order_acquire);
        const size_t avail = h - t;
        if (n > avail) n = avail;
        for (size_t i = 0; i < n; ++i) out[i] = buffer[(t + i) & mask];
        tail.store(t + n, std::memory_order_release);
        return n;
    }

    /** Consumer: discard everything currently queued. */
    void clear() noexcept { tail.store(head.load(std::memory_order_acquire), std::memory_order_release); }

private:
    std::vector<T> buffer;
    size_t mask = 0;
    alignas(64) std::atomic<size_t> head { 0 };
    alignas(64) std::atomic<size_t> tail { 0 };
};
} // namespace dali
