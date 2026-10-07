# DALI VISUAL 6 — by DALI AUDIO

Audio-reactive generative visual instrument. **VST3 + Standalone**, one shared engine.
C++17 · JUCE 8 · CMake · OpenGL 3.2 core / GLSL 150.

---

## מה חדש ב-v5 (עברית)

* **בלי פריסטים.** הסצנות עצמן חיות: כל חלק בסצנה (מצלמה, גיאומטריה, תאורה, צבע) מונע מהמוזיקה מבפנים,
  והצבע עובר בין "משפחות" צבע לבד לפי המצב המוזיקלי (CALM / BUILD / PEAK / CHAOS).
* **סט סצנות תלת-ממדי חדש:** 01 Kali Cathedral · 02 Tidal Cathedral · 03 Infinite Tunnel (חדש, תלת-ממד אמיתי) ·
  04 Apollonian Dream (חדש) · 05 Gyroid Caverns (חדש) · 06 Image Reactor. כל הסצנות הישנות נמחקו.
* **v5.1:** חלון התצוגה ומסך הלייב מראים בדיוק אותו דבר (ציר זמן משותף). איכות 4K: raymarching ברזולוציה אמיתית של הפיקסל,
  Bloom מנורמל לרזולוציה, Sharpness, ורינדור 150%/200% (supersampling). סצנות חדשות: Mandelbulb Bloom, Fourth Dimension.
* **v5.2:** 13 סצנות תלת-ממדיות + Image Reactor. חדשות: Mandelbox Temple, Crystal Sanctum, Menger Void, Quaternion Julia,
  Fractal Ocean, Dimension Gate.
* **v6.6:** אוטומציות שבנית כבר לא מתאפסות: מעבר סצנה מחליף רק את ה-routes של הסצנה (מסומנים S בלשונית MOD), שלך נשארים;
  עריכה של route של סצנה הופכת אותו לשלך. תוקן: שחזור מצב (הפעלה / טעינת פרויקט) כבר לא טוען init על המצב המשוחזר.
  בחירת סצנות בתווי MIDI כבויה כברירת מחדל (Program Change עדיין פעיל).
* **v6.5:** Standalone מתחיל כל הפעלה "נקי" (הסצנה הראשונה עם ה-init שלה); ההגדרות הטכניות (מסך, רזולוציה, מיפויי MIDI,
  מקור סאונד) תמיד נשמרות. "Restore the last session on launch" ב-SETTINGS מחזיר גם את המצב היצירתי. כפתור Reset Everything
  (ניתן ל-Undo). הוסר וינייט ה-BUILD במוצא (נראה כמו פילטר עדשה).
* **v6.4:** GO LIVE מסך מלא אמיתי (תיקון מדידתי של משטח ה-GL מול החלון + שורת אבחון ב-SETTINGS), וינייט רך וקומפוזיציה
  קרובה יותר בסצנות של אובייקט מרכזי. ה-init של כל סצנה כולל עכשיו ניתוב אוטומציות לפי תפקידים בלשונית MOD
  (קיק→אור, מידים→מבנה, באס איטי→מהירות, היי-האט→Bloom, סנר→Trip, בסליין→עוצמה, Chaos→משפחת צבע). מקור חדש: Bassline.
* **v6.3:** Auto Pilot תוקן (ספר "תיבות" שקפצו בין שני מנועים → החליף סצנות מהר מדי; עכשיו זמן מוזיקלי משלו, ווריאציות משותפות).
  Undo/Redo (Ctrl+Z / Ctrl+Y) לכל פעולה הרסנית. Init ייחודי לכל סצנה שנטען בבחירתה (+ Reset Scene). כפתור CHAOS.
  מעבר "מעבר-ממדים": נפילה למערבולת → מנהרת אור → הגחה מקליידוסקופ.
* **v6.2:** ביצועים: איכות אדפטיבית (רזולוציה ועומק raymarching לפי ה-FPS, מוצג בשורת המדים כ-Q), החלון הקטן קל בזמן לייב,
  200% הוסר, אופטימיזציה ל-Kali / Crystal / Nebula / Mandelbulb. תפקידים: מטריצת תגובה (Docs/role_matrix.txt) –
  בסליין, סאב, מידים, היי-האט, גבוהים וסנר מגיבים עכשיו בכל סצנה (שכבת מצלמה + שכבת מוצא + הוק מבני לכל סצנה).
* **v6.1:** הפרדת תפקידים לפסיטראנס: הבאסליין המתגלגל כבר לא מפעיל את הקיק (62 → 3 שגיאות), ו-CHAOS יחסי לנורמה של
  הטראק (גרוב פסי רגיל = PEAK במקום 97% CHAOS). תנועה חלקה (שעונים מוחלקים), Trip (שכבה פסיכדלית אוטומטית),
  Music Response, זמן מעבר בין סצנות, COLOR שעובד על כל הסצנות, FX עם קטגוריות והסברים, תצוגה מקדימה באיכות מלאה, FXAA.
* **v6 – הגרסה הראשונית המלאה:** 19 סצנות תלת-ממדיות + Image Reactor = 20. חדשות: Alien Megastructure, KIFS Reliquary,
  Biomech Hive, Nebula Drift, Sierpinski Shrine, Torus Nexus. מעבר עקביות: חשיפה מותאמת לכל סצנה, כך שהמעברים בסט אחידים.
* **Auto FX** (עמוד SCENE) – שכבת מצלמה ועדשה אוטומטית לכל סצנה: פאנץ' על הקיק, פיצול כרומטי על הסנר,
  טשטוש מהירות בשיאים, סחיפת צבע עם בהירות הסאונד.

## מה חדש ב-v4 (עברית)

* **כיוון ויזואלי חדש: שישה עולמות** (סצנות 18–23) – Sacred Bloom, Tidal Cathedral, Liquid Glass, Mycelium,
  Hyperdimension, Solar Temple. לכל עולם פלטה מתוכננת של 3–4 צבעים (+2 וריאציות ב-Macro D) ופרשנות משלו לסאונד.
* **מנוע ניתוח מוזיקלי מלא** – שש רצועות (Sub…High), שלוש שכבות זמן (מהיר/בינוני/איטי), צפיפות קיקים ומכות,
  טווח דינמי, ו**מצב מוזיקלי**: CALM → BUILD → PEAK → CHAOS → RELEASE. כל עולם משנה התנהגות לפי המצב.
* **Custom Image בסגנון Photism** – Flow Lines ו-Flow Paint: הוויזואל נבנה ממבנה התמונה עצמה (שדה כיוונים,
  קצוות, אזורים) ובפלטה שחולצה ממנה. ברירת המחדל לתמונה חדשה: Flow Lines.
* **פריסטים חדשים** A1–F3 (3 לכל עולם), G1–G2 (תמונה), H1 (מסע בין העולמות).

## מה חדש ב-v3 (עברית)

* **הוויזואל מגיב לסאונד עצמו, לא רק ל-BPM** – כל סצנה מקבלת את הספקטרום המלא (128 פסים) ואת צורת הגל,
  ו"שעונים" נפרדים לכל תחום תדרים: תנועה שקשורה לבס זזה רק כשיש בס.
* **8 סצנות חדשות ברמה גבוהה** (09–16): Kali Cathedral, Spectral Mandala, Julia Bloom, Hyperspace,
  Iridescent Oil, Hyperbolic Dream, Infinite Feedback, Waveform Geometry.
* **Bloom + ACES** – זוהר אמיתי וצבעי ניאון רוויים בלי שריפה (פקד *Bloom* בעמוד COLOR).
* **17 פריסטים חדשים** (20–36). מקשים 1–9 ו-0 לסצנות 1–10.
* **Image Reactor (סצנה 17)** – התמונה עצמה היא הוויזואל, לא שכבה על סצנה אחרת. גוררים תמונה → היא הופכת לוויזואל.
  8 מצבים: Kaleidoscope, Liquid, Tunnel, Spectral Slices, Droste, Glitch, Depth 3D, Neon Outline –
  כולם מונעים מהסאונד עצמו (שעוני התדרים והספקטרום), בלי BPM sync. תמונות עם שקיפות "צפות" בחלל; תמונות רגילות נשארות שלמות.
  7 פריסטים (09, 10, 37–42). השכבה הישנה נשארה כאופציה מתקדמת: "Overlay on current scene".

## מה חדש ב-v2 (עברית)

* **Standalone מאזין לסאונד של המחשב** (Windows) – בוחרים *System Audio* בכותרת. בלי כבל וירטואלי.
  ב-Mac: מנתבים דרך BlackHole ובוחרים אותו כ-Audio Input.
* **אין סאונד = אין תנועה** – התמונה נחה כשאין אודיו; המהירות נגזרת מהאנרגיה של המוזיקה (*Audio Drive*, *Idle Motion*).
* **זיהוי מוזיקלי חדש** – Snare, Hi-Hat, Build (שבירה/עלייה) ו-Drop (חזרת הקיק), כמקורות מודולציה וכאפקט *Build / Drop*.
* **Auto Pilot** – וריאציות לפי משפטים מוזיקליים, החלפת סצנות, קפיצה על ה-Drop.
* **יציאה למסך** – *OUTPUT DISPLAY* + *ID* (מספר על כל מסך) + *GO LIVE* בכותרת.
* **Settings בחלון נפרד** – תמיד מעל הכל. **Tab** מסתיר את הפאנל להגדלת התצוגה.

## התחלה מהירה (עברית)

1. התקן CMake ≥ 3.22 ו-Git.
   Windows: Visual Studio 2022 (Desktop C++). macOS: Xcode 14+.
2. בתיקיית הפרויקט:
   ```
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --config Release --parallel
   ```
   (JUCE 8.0.4 יורד אוטומטית. יש לך JUCE מקומי? הוסף `-DDALI_JUCE_PATH=C:/path/to/JUCE`)
3. התוצרים:
   `build/DaliVisual_artefacts/Release/VST3/Dali Visual.vst3`
   `build/DaliVisual_artefacts/Release/Standalone/Dali Visual(.exe/.app)`
4. **Standalone בהפעלה ראשונה:** Options → Audio/MIDI Settings → בחר כניסת אודיו.
   אם מופיעה הודעת "Audio input is muted" – לחץ Unmute. (הפלט של ה-Standalone שקט תמיד – אין פידבק.)
5. **אם יש שגיאות קומפילציה** – שלח לי את הפלט המלא של `cmake --build` ואתקן.

---

## Build

| | |
|---|---|
| Windows | Visual Studio 2022, CMake ≥ 3.22 |
| macOS | Xcode 14+, CMake ≥ 3.22 (universal: add `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"`) |

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release            # JUCE 8.0.4 via FetchContent
# or: cmake -B build -DDALI_JUCE_PATH=/path/to/JUCE
# or: put a JUCE checkout in ./JUCE
cmake --build build --config Release --parallel
```

Xcode / VS project: `cmake -B build -G Xcode` or `-G "Visual Studio 17 2022"`.

Install the VST3: copy `Dali Visual.vst3` to
`C:\Program Files\Common Files\VST3\` (Windows) or `~/Library/Audio/Plug-Ins/VST3/` (macOS).

Core unit tests (no JUCE, no GPU): `cmake -B build -DDALI_BUILD_TESTS=ON && cmake --build build --target DaliVisualCoreTests`

---

## What's new in v4 — a new visual direction

* **Six generative worlds** (scenes 18–23), designed composition-first with limited, planned palettes
  (`ramp4()` through 3–4 chosen colours; macro D blends three palette variants) — no rainbow cycling.
  Each world interprets the same analysis differently: kick = heartbeat contraction (Sacred Bloom), a wave of light
  down the nave (Tidal Cathedral), an impact ripple (Liquid Glass), a signal along the filaments (Mycelium), tunnel
  expansion (Hyperdimension), a pulse ring to ring (Solar Temple).
* **Musical analysis engine** — six bands (sub 20–60, bass, low-mid 150–500, mid, high-mid 2–6k, high), three time
  scales (FAST envelopes, MEDIUM ~0.5 s, SLOW 2–4 s), kick/onset densities, dynamic range (crest factor), and a
  **musical state machine** CALM / BUILD / PEAK / CHAOS / RELEASE with hysteresis, dwell times and cross-faded
  weights (`uState`, `uStateRelease`). Build = a kick-less passage after kicks whose upper bands rise in absolute
  level (a pad swell, a fading outro or a kick dropping out is not a build). Verified on a 110 s structured test track.
* **Photism-style image engine** — ImageDNA adds a structure-tensor flow field (contour tangent + coherence) and a
  palette-region map. New Image Reactor modes **Flow Lines** (line-integral streamlines of the image's own structure)
  and **Flow Paint** (feedback painting advected along it), coloured from the image's extracted palette.
* 13 new modulation sources (Sub, Low Mid, High Mid, Bass/Energy slow, densities, dynamic range, state weights).
* New presets A1–F3, G1–G2, H1 (Auto Pilot keeps the journey within the six worlds).

## What's new in v3 — the picture follows the sound itself

* **Full audio picture on the GPU** — every scene can read a 128-band log spectrum (30 Hz–16 kHz, pink-weighted,
  auto-levelled) and the live waveform (`uSpectrum`, helpers `spec()`, `specBand()`, `wave()` in common.glsl).
* **Band times** — `uBassTime`, `uMidTime`, `uHighTime`, `uLevelTime` advance only while their band sounds, so motion
  is driven by what is playing, not by a tempo clock.
* **8 new scenes (09–16)**, all validated on a real GL driver: raymarched fractal cathedral, spectrum mandala with
  feedback echoes, morphing Julia set (cardioid-boundary path, always connected), fractal-ornament hyperspace tunnel,
  thin-film iridescent oil, hyperbolic {p,q} tiling on the Poincaré disk, kaleidoscopic video feedback, and
  waveform-drawn sacred geometry. Each maps kick / snare / hi-hat / build / drop and the spectrum to its structure.
* **Output quality** — mip-chain bloom (*Bloom* parameter) and ACES filmic tone mapping for all scenes.
* **Image Reactor (scene 17)** — the dropped image itself becomes the visual (no longer an overlay on another scene).
  8 modes — Kaleidoscope, Liquid, Tunnel, Spectral Slices (every strip is a frequency), Droste, Glitch, Depth 3D
  (steep parallax on luminance), Neon Outline — all driven by band clocks and the spectrum, never by a tempo grid.
  Images with transparency float in a dark void; photos stay whole. 13 image controls + 8 live mode buttons in the
  IMAGE tab; the previous overlay remains available as "Overlay on current scene".
* **17 new factory presets** (20–36) + 7 Image Reactor presets (09, 10, 37–42) Keys 1–9 and 0 select scenes 1–10; MIDI notes from C1 upward select all 16.

## What's new in v2

* **System Audio (standalone, Windows)** — WASAPI loopback of the default output device ("what you hear");
  follows default-device changes and recovers from device loss. Default source in the standalone.
  macOS has no loopback API: use BlackHole and select it as the input.
* **Silence rests** — an activity gate (analysis + "no audio arriving" timeout) stops all motion, the beat clock
  and beat pulses when nothing plays. *Idle Motion* sets optional motion without audio.
* **Motion follows the music** — scene time advances with the music's energy (*Audio Drive*).
* **New analysis** — snare/clap (spectral-flatness gated), hi-hat, **Build** (breakdown / riser tension) and
  **Drop** (kick returns after a breakdown). New modulation sources: Snare, Hi-Hat, Build, Drop.
  Kick detection fix: a rejected candidate no longer starts the refractory period; breakdown noise is rejected
  by a kick-band peak memory.
* **Musical Dynamics** (*Build / Drop* knob) — builds drain colour and close in, drops hit with a zoom/light burst.
* **Auto Pilot** — phrase-based macro/colour variations (every 2–32 bars), optional scene changes, snap on drops.
* **UI** — scene tiles with real renders, Settings as a real top-level window (the OpenGL preview is a native
  child window on Windows, so nothing is drawn over it), output display chooser + *Identify* + *GO LIVE* in the
  header, **Tab** hides the side panel, tooltips on every control, Kick/Snare/Hat/Build meters, NO SIGNAL hint.
* 2 new factory presets (Festival - Build and Drop, Auto Pilot - Journey); factory presets updated once (v2).

## Using it

* **Scenes** — 8 scenes (keys **1–8**, the SCENE tab, the header, Program Change 0–7, or notes C1–G1).
  Changing scene cross-fades over 0.6 s. Each scene has 4 scene-specific macros + Intensity + Motion.
* **Audio** — sensitivity (AGC-normalised), smoothing, per-band reaction, clock source
  (Auto = host transport when playing → detected tempo → internal clock), sync division incl. triplets and dotted.
* **Modulation Matrix (MOD)** — 16 slots, any of 22 sources → any modulatable parameter (~100 targets).
  Per slot: Amount, Min, Max, Smoothing, Attack, Release, Curve, Polarity (±), Invert, Sensitivity.
  Shortcut: right-click any knob → *Modulate by…*. Knobs show the live modulated value as a cyan ring.
* **Effects (FX)** — 20 GPU effects, each On/Amount/P2, fully chainable; ▲▼ change the order.
  Feedback and Trails keep their own history, so reordering does not break persistence.
* **Colour** — MONO, ACID, DEEP PURPLE, RED/BLACK, BLUE/BLACK, CYAN, EARTH, PSYCHEDELIC, CUSTOM
  + Hue, Saturation, Brightness, Contrast, Color Amount, Color Shift, Audio Color.
* **Image Reactive Mode (IMAGE)** — drag a PNG/JPG/logo anywhere onto the plug-in. It is analysed on a
  background thread (luminance, edges, distance field, shape mask, 4-colour palette) into an "image DNA".
  The source image is never modified; the **template** (7 modes, 4 blend modes, 21 structure parameters,
  mirror, kaleidoscope) turns it into generative structure. *Add Audio Routes* wires the typical
  reactions (Bass→Scale, Kick→Symmetry, Mid→Warp, High→Detail, Transient→Feedback, Sync LFO→Rotation).
  Templates save/load as `.dvtemplate` (image embedded) and are also stored inside presets.
* **Presets** — Save / Save As / Duplicate / Delete / prev / next in the header. A preset stores every
  parameter, the modulation matrix, effect order, template parameters + image, and output settings.
  12 factory presets are installed on first run (Settings → *Reinstall factory presets* restores them).
  Folder: `%APPDATA%\Dali Audio\Dali Visual\Presets` / `~/Library/Dali Audio/Dali Visual/Presets`.
* **MIDI** — right-click any control → *MIDI Learn*, then move a CC. Mappings are global and saved
  with the plug-in state. Every note-on fires the *MIDI Trigger* modulation source (velocity-scaled).
* **Fullscreen** — header *FULLSCREEN* (▾ chooses the display), key **F**, or double-click the preview.
  Borderless, no UI, on any monitor; **ESC** or double-click closes it. The output keeps running with the
  editor closed. While it is live the preview renders at half resolution (or pauses — Settings).
* **Settings** — output display, render resolution (50/75/100 %), V-Sync (off = uncapped FPS),
  preview-while-output, MIDI options, renderer information and shader diagnostics.

---

## Architecture

```
Audio thread ──push──► SPSC ring ──► AudioAnalyzer thread (FFT 2048 / hop 512)
   │  host transport (try-lock)            │ RMS, peak, bands + envelopes, kick, transient, onset,
   │  MIDI ──► SPSC ring ──► MidiMapper     │ centroid, flux, stereo width/energy/pan,
   ▼  (message thread, 60 Hz)               │ BPM + beat phase (autocorrelation + comb, phase-locked)
 audio passes through untouched             ▼
                                   AudioAnalyzer::snapshot()
                                            │
 OpenGL thread(s): RenderEngine (preview) / RenderEngine (fullscreen output)
   analysis smoothing → MusicalClock (host / detected / internal) → ModulationEngine
   → scene pass (own feedback, crossfade) → image template layer + composite
   → effects rack (user order) → output grade → screen (+ FrameSinks)
```

| Requested abstraction | Implementation |
|---|---|
| VisualScene | `Render/VisualScene.h` + `Shaders/scenes/*.frag` |
| Shader | `Render/ShaderProgram.h` (class `Shader`) |
| VisualEffect | `Render/VisualEffect.h` + `Shaders/fx/*.frag`, order in `Render/EffectChain.h` |
| AudioAnalyzer | `Audio/AudioAnalyzer.*` (thread) over `FeatureExtractor`, `BeatTracker`, `FFT` |
| ModulationSource / Target | `Modulation/ModulationMatrix.h` |
| ModulationMatrix | `Modulation/ModulationMatrix.*` (config) + `ModulationCore.*` (DSP) |
| Preset | `Preset/PresetManager.h` (struct `Preset`), factory set in `FactoryPresets.cpp` |
| MIDIMapper | `Midi/MidiMapper.*` |
| RenderEngine | `Render/RenderEngine.*` |
| OutputManager | `Output/OutputManager.*`, `Output/FrameSink.h` |
| ImageProcessor | `Image/ImageProcessor.*` + `Image/ImageDNA.*` |
| TemplateGenerator | `Image/TemplateGenerator.*` + `Shaders/template_layer.frag` |

**Threading rules.** The audio thread never locks, allocates or waits: it writes into wait-free
SPSC rings and publishes host timing with a *try*-lock (skips a block rather than wait). Parameters are
read as atomics by the GL threads. Shaders are compiled once per GL context, so switching scenes/effects
never stalls a frame. Render targets are RGBA16F (HDR headroom for glow/feedback).

**Performance.** Analysis costs ~50 µs per 512-sample hop (≈0.5 % of one core). GPU cost scales with
resolution; use *Render resolution* 75 % / 50 % for 4K projectors on weaker GPUs, and disable V-Sync to
see the uncapped frame rate.

### Spout / Syphon
`FrameSink` is the integration point: the engine that drives the output calls
`publishFrame(texture, w, h)` every frame with the final graded RGBA16F texture, on its GL thread.
A Spout (Windows, SpoutGL) or Syphon (macOS, Syphon.framework) adapter implements that interface and
registers with `processor.output.addFrameSink(...)`. The SDKs themselves are **not** bundled
(licensing / platform build), so no sender is active out of the box.

---

## Verification status (honest)

Verified in the development environment:
* **All 33 shaders** compile, link and render on a real OpenGL 3.2 core driver (Mesa), assembled exactly
  as the plug-in assembles them — `Tools/ShaderHarness` (renders contact sheets of every scene, effect,
  template mode and palette; checks for NaN / black / blown-out output).
* **DSP / modulation / image core** — `Tests/CoreTests.cpp`, 90 checks (v4: musical state on a full track structure, image flow field; v3: spectrum band accuracy, waveform; v2: kick 25/25, snare 25/25, hat 100/100, 0 false snares, one drop at the right moment): BPM detection at 100/128/140/145/
  150/174 BPM within ±0.5 BPM, beat-phase error ≤ 0.063 beat, level-independent AGC, stereo metrics,
  modulation curves/attack/release/polarity, image DNA on a 4000×3000 image in ~180 ms.

Not verified (no JUCE / compiler for the plug-in target available there):
* the v2 JUCE-layer changes (UI, WASAPI loopback) are compiled by the GitHub CI, not here;
* VST3 validation, DAW testing, multi-monitor fullscreen on Windows/macOS, frame-rate on real GPUs.

Expect a few compile errors on the first build; send the build log and they will be fixed.

---

## Project layout
```
CMakeLists.txt          plug-in build (JUCE 8)
Source/                 C++ (Audio, Modulation, Image, Render, Output, Midi, Preset, UI, Core)
Shaders/                GLSL 150: common, scenes/, fx/, template, output, crossfade
Tests/                  JUCE-free unit tests (+ CMakeLists)
Tools/ShaderHarness/    headless GL harness (Linux/EGL) used to validate the shaders
```
