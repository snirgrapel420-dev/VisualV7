#include "ShaderProgram.h"
#include "BinaryData.h"

using namespace juce::gl;

namespace dali
{
namespace
{
GLuint compileStage(GLenum type, const juce::String& src, juce::String& err)
{
    const GLuint s = glCreateShader(type);
    const juce::CharPointer_UTF8 utf8 = src.toUTF8();
    const GLchar* ptr = utf8.getAddress();
    glShaderSource(s, 1, &ptr, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (ok == GL_FALSE)
    {
        GLchar log[4096] = {};
        GLsizei len = 0;
        glGetShaderInfoLog(s, (GLsizei) sizeof(log), &len, log);
        err = juce::String(log, (size_t) len);
        glDeleteShader(s);
        return 0;
    }
    return s;
}
}

juce::String Shader::resource(const char* resName)
{
    int size = 0;
    if (const char* data = BinaryData::getNamedResource(resName, size))
        return juce::String::fromUTF8(data, size);
    return {};
}

juce::String Shader::assembleFragment(const juce::String& body)
{
    return "#version 150\n" + resource("common_glsl") + "\n#line 1\n" + body;
}

bool Shader::build(const juce::String& vertexSrc, const juce::String& fragmentSrc, const juce::String& debugName)
{
    release();
    name = debugName;
    error.clear();
    if (vertexSrc.isEmpty() || fragmentSrc.isEmpty()) { error = "missing shader source"; return false; }

    juce::String e;
    const GLuint vs = compileStage(GL_VERTEX_SHADER, vertexSrc, e);
    if (vs == 0) { error = "vertex: " + e; return false; }
    const GLuint fs = compileStage(GL_FRAGMENT_SHADER, fragmentSrc, e);
    if (fs == 0) { error = "fragment: " + e; glDeleteShader(vs); return false; }

    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glBindFragDataLocation(program, 0, "fragColor");
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (ok == GL_FALSE)
    {
        GLchar log[4096] = {};
        GLsizei len = 0;
        glGetProgramInfoLog(program, (GLsizei) sizeof(log), &len, log);
        error = "link: " + juce::String(log, (size_t) len);
        release();
        return false;
    }
    return true;
}

void Shader::release()
{
    if (program != 0) glDeleteProgram(program);
    program = 0;
    cache.clear();
}

void Shader::use() const noexcept { glUseProgram(program); }

int Shader::loc(const char* uniform)
{
    auto it = cache.find(uniform);
    if (it != cache.end()) return it->second;
    const int l = (int) glGetUniformLocation(program, uniform);
    cache.emplace(uniform, l);
    return l;
}

void Shader::set(const char* n, float v)                              { const int l = loc(n); if (l >= 0) glUniform1f(l, v); }
void Shader::set(const char* n, int v)                                { const int l = loc(n); if (l >= 0) glUniform1i(l, v); }
void Shader::set(const char* n, float a, float b)                     { const int l = loc(n); if (l >= 0) glUniform2f(l, a, b); }
void Shader::set(const char* n, float a, float b, float c)            { const int l = loc(n); if (l >= 0) glUniform3f(l, a, b, c); }
void Shader::set(const char* n, float a, float b, float c, float d)   { const int l = loc(n); if (l >= 0) glUniform4f(l, a, b, c, d); }
} // namespace dali
