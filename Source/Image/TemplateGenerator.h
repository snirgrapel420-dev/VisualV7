#pragma once
// ============================================================================
//  TemplateGenerator — the Image Reactive template as a document.
//  A template = template parameters (mode, blend, mirror, kaleido and the 21
//  structure parameters) + the source image (embedded PNG). Saved as
//  *.dvtemplate (XML) and reusable on top of any scene / preset.
// ============================================================================
#include <juce_audio_processors/juce_audio_processors.h>
#include "ImageProcessor.h"

namespace dali
{
class ModulationMatrix;

class TemplateGenerator
{
public:
    TemplateGenerator(juce::AudioProcessorValueTreeState& s, ImageProcessor& i) : apvts(s), image(i) {}

    static constexpr const char* fileExtension = ".dvtemplate";
    /** <user app data>/Dali Audio/Dali Visual/Templates */
    static juce::File defaultFolder()
    {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("Dali Audio").getChildFile("Dali Visual").getChildFile("Templates");
    }

    juce::ValueTree createTree(bool includeImage = true) const;
    bool applyTree(const juce::ValueTree& t);                 // message thread

    bool saveToFile(const juce::File& f) const;
    bool loadFromFile(const juce::File& f);

    /** Resets all template parameters to their defaults (image and enable untouched). */
    void resetParameters();

    /** Adds the recommended sound routes for images to the matrix:
        Bass→Zoom, Kick→Symmetry, Mid→Warp, Hi-Hat→Outline, Snare→Trails, Centroid→Rotation. */
    static void addReactiveRoutes(ModulationMatrix& m);

    static inline const juce::Identifier treeId { "DaliTemplate" };

private:
    juce::AudioProcessorValueTreeState& apvts;
    ImageProcessor& image;
};
} // namespace dali
