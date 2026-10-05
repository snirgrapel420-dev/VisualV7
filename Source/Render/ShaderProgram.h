#pragma once
// ============================================================================
//  Shader — a linked GLSL program with a uniform-location cache.
//  Sources come from BinaryData: every fragment shader is assembled as
//  "#version 150" + common.glsl + "#line 1" + body. GL-thread only.
// ============================================================================
#include <juce_opengl/juce_opengl.h>
#include <unordered_map>
#include <string>

namespace dali
{
class Shader
{
public:
    Shader() = default;
    ~Shader() { release(); }

    /** Compiles & links. On failure getError() describes the problem. */
    bool build(const juce::String& vertexSrc, const juce::String& fragmentSrc, const juce::String& debugName);
    void release();

    bool isValid() const noexcept { return program != 0; }
    void use() const noexcept;
    const juce::String& getError() const noexcept { return error; }
    const juce::String& getName() const noexcept  { return name; }

    int loc(const char* uniform);
    void set(const char* n, float v);
    void set(const char* n, int v);
    void set(const char* n, float a, float b);
    void set(const char* n, float a, float b, float c);
    void set(const char* n, float a, float b, float c, float d);
    void set3(const char* n, const float* v) { set(n, v[0], v[1], v[2]); }

    /** Loads a BinaryData resource as text ("" if missing). */
    static juce::String resource(const char* name);
    /** Assembles a fragment shader around common.glsl. */
    static juce::String assembleFragment(const juce::String& body);

private:
    unsigned int program = 0;
    std::unordered_map<std::string, int> cache;
    juce::String error, name;
};
} // namespace dali
