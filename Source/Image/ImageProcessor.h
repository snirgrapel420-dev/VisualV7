#pragma once
// ============================================================================
//  ImageProcessor — owns the Image Reactive Mode *source* image and its
//  analysed DNA. Loading/analysis runs on a background thread; the render
//  engines pick up new DNA by version number and upload it on their own GL
//  thread. The source stays untouched — the template (how the DNA becomes
//  structure) lives in the tpl* parameters (see TemplateGenerator).
// ============================================================================
#include <juce_graphics/juce_graphics.h>
#include <juce_events/juce_events.h>
#include "ImageDNA.h"
#include <atomic>
#include <memory>

namespace dali
{
class ImageProcessor : public juce::ChangeBroadcaster
{
public:
    ImageProcessor();
    ~ImageProcessor() override;

    /** Message thread. Returns false if the file is not a readable image. */
    bool loadFile(const juce::File& file);
    /** Any thread. Encoded image data (PNG/JPEG/GIF/BMP). */
    void loadEncoded(const juce::MemoryBlock& data, const juce::String& name);
    void clear();

    static bool isSupportedFile(const juce::File& f) { return f.hasFileExtension("png;jpg;jpeg;gif;bmp"); }

    // ---- render side --------------------------------------------------------------------------
    std::shared_ptr<const ImageDNA> getDNA() const { const juce::SpinLock::ScopedLockType sl(lock); return dna; }
    std::uint32_t getVersion() const noexcept { return version.load(); }

    // ---- UI / preset side -----------------------------------------------------------------------
    bool hasImage() const { return getDNA() != nullptr; }
    bool isBusy() const noexcept { return busy.load(); }
    juce::Image getThumbnail() const { const juce::SpinLock::ScopedLockType sl(lock); return thumbnail; }
    juce::MemoryBlock getEncodedPNG() const { const juce::SpinLock::ScopedLockType sl(lock); return png; }
    juce::String getName() const { const juce::SpinLock::ScopedLockType sl(lock); return name; }
    juce::String getStatus() const { const juce::SpinLock::ScopedLockType sl(lock); return status; }

private:
    void process(const juce::MemoryBlock& data, const juce::String& displayName, std::uint32_t id);

    juce::ThreadPool pool { 1 };
    mutable juce::SpinLock lock;
    std::shared_ptr<const ImageDNA> dna;
    juce::Image thumbnail;
    juce::MemoryBlock png;
    juce::String name, status { "No image" };
    std::atomic<std::uint32_t> version { 0 }, requestId { 0 };
    std::atomic<bool> busy { false };

    JUCE_DECLARE_WEAK_REFERENCEABLE(ImageProcessor)
};
} // namespace dali
