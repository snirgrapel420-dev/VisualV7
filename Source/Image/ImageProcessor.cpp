#include "ImageProcessor.h"

namespace dali
{
ImageProcessor::ImageProcessor() = default;

ImageProcessor::~ImageProcessor()
{
    ++requestId;
    pool.removeAllJobs(true, 5000);
}

bool ImageProcessor::loadFile(const juce::File& file)
{
    if (!file.existsAsFile() || !isSupportedFile(file)) return false;
    juce::MemoryBlock data;
    if (!file.loadFileAsData(data)) return false;
    loadEncoded(data, file.getFileName());
    return true;
}

void ImageProcessor::loadEncoded(const juce::MemoryBlock& data, const juce::String& displayName)
{
    const auto id = ++requestId;
    busy = true;
    { const juce::SpinLock::ScopedLockType sl(lock); status = "Analysing " + displayName + " ..."; }
    sendChangeMessage();

    juce::WeakReference<ImageProcessor> weak(this);
    pool.addJob([this, weak, id, data, displayName]
    {
        if (requestId.load() != id) return;                 // superseded by a newer request
        process(data, displayName, id);
        juce::MessageManager::callAsync([weak] { if (auto* p = weak.get()) p->sendChangeMessage(); });
    });
}

void ImageProcessor::clear()
{
    ++requestId;
    {
        const juce::SpinLock::ScopedLockType sl(lock);
        dna.reset(); thumbnail = {}; png.reset(); name = {}; status = "No image";
    }
    busy = false;
    ++version;
    sendChangeMessage();
}

void ImageProcessor::process(const juce::MemoryBlock& data, const juce::String& displayName, std::uint32_t id)
{
    juce::Image img = juce::ImageFileFormat::loadFrom(data.getData(), data.getSize());
    if (!img.isValid())
    {
        const juce::SpinLock::ScopedLockType sl(lock);
        status = "Could not read " + displayName;
        busy = false;
        return;
    }

    // Keep a bounded copy (embedded in presets / templates): max 1024 px.
    constexpr int maxDim = 1024;
    if (img.getWidth() > maxDim || img.getHeight() > maxDim)
    {
        const float s = float(maxDim) / float(juce::jmax(img.getWidth(), img.getHeight()));
        img = img.rescaled(juce::jmax(1, juce::roundToInt(img.getWidth() * s)),
                           juce::jmax(1, juce::roundToInt(img.getHeight() * s)),
                           juce::Graphics::highResamplingQuality);
    }
    img = img.convertedToFormat(juce::Image::ARGB);

    const int w = img.getWidth(), h = img.getHeight();
    std::vector<std::uint8_t> rgba(size_t(w) * size_t(h) * 4);
    {
        const juce::Image::BitmapData bd(img, juce::Image::BitmapData::readOnly);
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
            {
                const juce::Colour c = bd.getPixelColour(x, y);          // un-premultiplied
                std::uint8_t* p = &rgba[(size_t(y) * size_t(w) + size_t(x)) * 4];
                p[0] = c.getRed(); p[1] = c.getGreen(); p[2] = c.getBlue(); p[3] = c.getAlpha();
            }
    }

    auto result = std::make_shared<ImageDNA>(ImageDNA::analyse(rgba.data(), w, h, 1024));

    juce::MemoryOutputStream pngOut;
    juce::PNGImageFormat().writeImageToStream(img, pngOut);

    const int tw = juce::jmax(1, w * 160 / juce::jmax(w, h)), th = juce::jmax(1, h * 160 / juce::jmax(w, h));
    auto thumb = img.rescaled(tw, th, juce::Graphics::mediumResamplingQuality);

    if (requestId.load() != id) return;                     // cleared / replaced meanwhile
    {
        const juce::SpinLock::ScopedLockType sl(lock);
        dna = result;
        thumbnail = thumb;
        png = pngOut.getMemoryBlock();
        name = displayName;
        status = displayName + "  -  " + juce::String(w) + "x" + juce::String(h) + "  -  "
               + juce::String(juce::roundToInt(result->coverage * 100.0f)) + "% shape"
               + (result->hasAlpha ? "  -  alpha" : "");
    }
    ++version;
    busy = false;
}
} // namespace dali
