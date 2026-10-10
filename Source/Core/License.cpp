#include "License.h"
#include "LicenseKeys.h"
#include "AppPrefs.h"
#include "../Render/Library.h"
#include <juce_cryptography/juce_cryptography.h>
#include <atomic>

namespace dali
{
namespace
{
// ---- what the DEMO allows (edit here) --------------------------------------------------------
constexpr int    kDemoScenes[] = { 0, 1, 2, 3, 4 };        // scenes 01-05; all others need the full version / trial
constexpr bool   kDemoLiveOutput = false;                  // GO LIVE fullscreen output
constexpr bool   kDemoImage      = false;                  // own image (Image Reactor + image layer)
constexpr double kDemoRecSeconds = 30.0;                   // REC in the demo: this long, watermark burned in

constexpr juce::int64 kDayMs = 24LL * 60 * 60 * 1000;

std::atomic<int> serialState { -1 };                       // -1 unknown, 0 none, 1 valid serial
std::atomic<juce::int64> trialStart { 0 };                 // ms since 1970; 0 = never; -1 = over (tampered / clock)

juce::int64 nowMs() noexcept { return juce::Time::currentTimeMillis(); }

juce::String normalise(const juce::String& code)
{
    juce::String s;
    for (auto c : code.toUpperCase())
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z')) s += juce::String::charToString(c);
    if (s.startsWith("DALI") && s.length() == 16) s = s.substring(4);
    return s;
}

juce::String saltedHash(const juce::String& text)
{
    juce::MemoryBlock data;
    data.loadFromHexString(licensekeys::kSaltHex);
    const auto utf8 = text.toStdString();
    data.append(utf8.data(), utf8.size());
    return juce::SHA256(data.getData(), data.getSize()).toHexString();
}

bool isValidSerial(const juce::String& serial)
{
    const auto body = normalise(serial);
    if (body.length() != 12) return false;
    const auto hash = saltedHash(body);
    for (const char* h : licensekeys::kHashes)
        if (hash == h) return true;
    return false;
}

juce::String storedSerial()
{
    juce::PropertiesFile f(AppPrefs::options());
    return f.getValue("serial");
}

// ---- trial record: "start:lastSeen:signature", in the settings file AND a second file ---------
struct TrialRecord { juce::int64 start = 0, lastSeen = 0; bool present = false, valid = false; };

juce::String sign(juce::int64 start, juce::int64 lastSeen)
{
    return saltedHash("dv-trial|" + juce::String(start) + "|" + juce::String(lastSeen)).substring(0, 32);
}

juce::String encode(juce::int64 start, juce::int64 lastSeen)
{
    return juce::String(start) + ":" + juce::String(lastSeen) + ":" + sign(start, lastSeen);
}

TrialRecord decode(const juce::String& s)
{
    TrialRecord r;
    if (s.isEmpty()) return r;
    r.present = true;
    const auto parts = juce::StringArray::fromTokens(s, ":", {});
    if (parts.size() != 3) return r;
    r.start = parts[0].getLargeIntValue();
    r.lastSeen = parts[1].getLargeIntValue();
    r.valid = r.start > 0 && parts[2] == sign(r.start, r.lastSeen);
    return r;
}

juce::File secondFile()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile(".com.daliaudio.dvstate");
}

void writeTrial(juce::int64 start, juce::int64 lastSeen)
{
    const auto rec = encode(start, lastSeen);
    {
        juce::PropertiesFile f(AppPrefs::options());
        f.setValue("trialState", rec);
        f.saveIfNeeded();
    }
    const auto second = secondFile();
    second.getParentDirectory().createDirectory();
    second.replaceWithText(rec);
}

/** Reads both copies. Returns the start (0 never, -1 over) and keeps lastSeen current. */
juce::int64 loadTrial()
{
    TrialRecord a, b;
    {
        juce::PropertiesFile f(AppPrefs::options());
        a = decode(f.getValue("trialState"));
    }
    if (secondFile().existsAsFile()) b = decode(secondFile().loadFileAsString().trim());
    if (!a.present && !b.present) return 0;
    if ((a.present && !a.valid) || (b.present && !b.valid)) return -1;           // edited by hand

    juce::int64 start = 0, lastSeen = 0;
    for (auto* r : { &a, &b })
        if (r->valid)
        {
            start = start == 0 ? r->start : juce::jmin(start, r->start);
            lastSeen = juce::jmax(lastSeen, r->lastSeen);
        }
    const auto now = nowMs();
    if (now + kDayMs < lastSeen) return -1;                                     // clock set back
    if (now > start + License::trialDays * kDayMs) return start;                 // over: keep the record as it is
    if (!a.present || !b.present || now > lastSeen) writeTrial(start, juce::jmax(now, lastSeen));   // restore a deleted copy
    return start;
}

bool fullSerial() noexcept
{
    int s = serialState.load();
    if (s < 0) { License::refresh(); s = serialState.load(); }
    return s == 1;
}

bool trialRunning() noexcept
{
    const auto start = trialStart.load();
    if (start <= 0) return false;
    const auto now = nowMs();
    return now >= start - kDayMs && now < start + License::trialDays * kDayMs;
}
} // namespace

void License::refresh()
{
    serialState.store(isValidSerial(storedSerial()) ? 1 : 0);
    trialStart.store(loadTrial());
}

License::Mode License::mode() noexcept
{
    if (fullSerial()) return Mode::Full;
    return trialRunning() ? Mode::Trial : Mode::Demo;
}

bool License::isSceneAllowed(int sceneIndex) noexcept
{
    if (mode() != Mode::Demo) return true;
    for (int s : kDemoScenes) if (s == sceneIndex) return true;
    return false;
}

int License::nearestAllowedScene(int sceneIndex) noexcept
{
    if (isSceneAllowed(sceneIndex)) return sceneIndex;
    int best = kDemoScenes[0];
    for (int s : kDemoScenes) if (std::abs(s - sceneIndex) < std::abs(best - sceneIndex)) best = s;
    return best;
}

bool License::allows(Feature f) noexcept
{
    if (mode() != Mode::Demo) return true;
    switch (f)
    {
        case Feature::LiveOutput: return kDemoLiveOutput;
        case Feature::Recording:  return true;                 // limited: recordingLimitSeconds()
        case Feature::Image:      return kDemoImage;
        case Feature::Scene:      return false;                // per scene: isSceneAllowed()
    }
    return false;
}

double License::recordingLimitSeconds() noexcept
{
    return mode() == Mode::Demo ? kDemoRecSeconds : 0.0;
}

// ---- trial ---------------------------------------------------------------------------------------
bool License::canStartTrial()
{
    fullSerial();
    return trialStart.load() == 0;
}

bool License::startTrial()
{
    if (!canStartTrial()) return false;
    const auto now = nowMs();
    writeTrial(now, now);
    trialStart.store(loadTrial());
    return trialRunning();
}

bool License::trialExpired()
{
    fullSerial();
    return trialStart.load() != 0 && !trialRunning();
}

int License::trialDaysLeft() noexcept
{
    if (!trialRunning()) return 0;
    const auto left = trialStart.load() + trialDays * kDayMs - nowMs();
    return int(juce::jlimit<juce::int64>(1, trialDays, (left + kDayMs - 1) / kDayMs));
}

// ---- serial --------------------------------------------------------------------------------------
bool License::activate(const juce::String& serial)
{
    if (!isValidSerial(serial)) return false;
    juce::PropertiesFile f(AppPrefs::options());
    f.setValue("serial", serial.trim().toUpperCase());
    f.saveIfNeeded();
    serialState.store(1);
    return true;
}

void License::deactivate()
{
    juce::PropertiesFile f(AppPrefs::options());
    f.removeValue("serial");
    f.saveIfNeeded();
    serialState.store(0);
}

juce::String License::maskedSerial()
{
    if (!fullSerial()) return {};
    return "DALI-****-****-" + normalise(storedSerial()).substring(8);
}

// ---- text ----------------------------------------------------------------------------------------
juce::String License::featureName(Feature f)
{
    switch (f)
    {
        case Feature::LiveOutput: return "GO LIVE (fullscreen output)";
        case Feature::Recording:  return "REC (video export)";
        case Feature::Image:      return "Your own image (Image Reactor)";
        case Feature::Scene:      return "This scene";
    }
    return {};
}

juce::String License::demoSummary()
{
    return "Demo: scenes 01-" + juce::String(int(std::size(kDemoScenes))).paddedLeft('0', 2)
         + ", REC up to " + juce::String(int(kDemoRecSeconds)) + " s, watermark. "
         + "Full version: all " + juce::String(int(sceneLibrary().size())) + " scenes, GO LIVE, unlimited REC, your own images, no watermark.";
}

juce::String License::statusText()
{
    switch (mode())
    {
        case Mode::Full:  return "Full version (" + maskedSerial() + ")";
        case Mode::Trial: return "Trial - " + juce::String(trialDaysLeft()) + (trialDaysLeft() == 1 ? " day left" : " days left");
        case Mode::Demo:  break;
    }
    return trialExpired() ? "Demo (trial ended)" : "Demo";
}
} // namespace dali
