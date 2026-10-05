#pragma once
// ============================================================================
//  Library — static descriptions of the 8 scenes and 20 effects.
//  Pure data (no JUCE / GL) so parameters, UI, engine and tools share it.
// ============================================================================
#include <array>

namespace dali
{
struct SceneInfo
{
    const char* id;
    const char* name;
    const char* resource;          // BinaryData resource name
    const char* macro[4];          // scene-specific names of macros A..D
    const char* description;
};

inline const std::array<SceneInfo, 20>& sceneLibrary();

/** Index of the Image Reactor scene (the scene that renders the loaded image itself). */
constexpr int kImageSceneIndex = 19;

// The scenes are self-driving: every moving part is automated from the music inside the scene
// (camera, geometry, light, colour evolution with the musical state). The macros only set character.
inline const std::array<SceneInfo, 20>& sceneLibrary()
{
    static const std::array<SceneInfo, 20> s { {
        { "kali",     "01  KALI CATHEDRAL",    "scene_09_kali_cathedral_frag",    { "Fold Twist", "Complexity", "Flight Speed", "Glow" },
          "Flight through a raymarched fractal cathedral. Bass flies you forward, kicks ignite the walls, snares twist the folds, the light and colour evolve with the music." },
        { "tidal",    "02  TIDAL CATHEDRAL",   "scene_19_tidal_cathedral_frag",   { "Architecture", "Caustics", "Drift Speed", "Colour Family" },
          "Drifting through an alien cathedral under the sea. Kick: a wave of light runs down the nave. Build: the camera rises and the light dims. Peak: light floods in." },
        { "tunnel",   "03  INFINITE TUNNEL",   "scene_30_infinite_tunnel_frag",   { "Wall Detail", "Ribs", "Speed", "Colour Family" },
          "A colossal winding tunnel with engraved fractal walls. Bass is the throttle, the kick blows the tunnel open, snares jolt the carvings." },
        { "apollo",   "04  APOLLONIAN DREAM",  "scene_31_apollonian_dream_frag",  { "Sphere Packing", "Glow", "Flight Speed", "Colour Family" },
          "Flight through an infinite 3D Apollonian fractal of spheres within spheres. The kick makes the whole structure breathe." },
        { "gyroid",   "05  GYROID CAVERNS",    "scene_32_gyroid_caverns_frag",    { "Cave Scale", "Veins", "Flow Speed", "Colour Family" },
          "Gliding through smooth organic caverns lit by bioluminescent veins. Bass swells the walls, the kick pulses the veins." },
        { "bulb",     "06  MANDELBULB BLOOM",  "scene_33_mandelbulb_bloom_frag",  { "Power", "Glow", "Orbit Speed", "Colour Family" },
          "The 3D Mandelbrot floating in space. The kick makes the whole form blossom, the mids slowly re-grow it, chaos makes it writhe." },
        { "fourd",    "07  FOURTH DIMENSION",  "scene_34_fourth_dimension_frag",  { "Lattice", "Iridescence", "Travel Speed", "Colour Family" },
          "Flying through a 3D slice of a 4D lattice that moves through the fourth dimension: struts merge and split, chambers open and close. The kick jumps to the next slice." },
        { "mbox",     "08  MANDELBOX TEMPLE",  "scene_35_mandelbox_temple_frag",  { "Scale", "Edge Glow", "Orbit Speed", "Colour Family" },
          "A box-folding fractal whose negative scale opens vast carved halls. The kick makes the whole temple breathe, the snare snaps an extra fold." },
        { "crystal",  "09  CRYSTAL SANCTUM",   "scene_36_crystal_sanctum_frag",   { "Crystals", "Inner Light", "Orbit Speed", "Colour Family" },
          "A cathedral of giant crystals. The kick sends a ring of light through them, the build makes them grow, the peak lights them from within." },
        { "menger",   "10  MENGER VOID",       "scene_37_menger_void_frag",       { "Levels", "Edge Light", "Flight Speed", "Colour Family" },
          "An endless corridor through a Menger sponge: square holes within square holes. The kick sends a pulse of light down the corridor." },
        { "julia4d",  "11  QUATERNION JULIA",  "scene_38_quaternion_julia_frag",  { "Morph Range", "Glow", "Orbit Speed", "Colour Family" },
          "A Julia set in four dimensions, seen as a 3D slice. The music moves its 4D constant: the form is reborn continuously, the kick snaps it to a new shape." },
        { "ocean",    "12  FRACTAL OCEAN",     "scene_39_fractal_ocean_frag",     { "Wave Height", "Foam Glow", "Glide Speed", "Colour Family" },
          "Gliding over a night sea of fractal waves under the moon. The bass is the swell, the kick sends a ring wave, chaos is a storm." },
        { "gate",     "13  DIMENSION GATE",    "scene_40_dimension_gate_frag",    { "Density", "Gate Glow", "Flight Speed", "Colour Family" },
          "Flying through portal gates; behind each gate another dimension (crystal shards, sphere lattice, twisted columns). The kick flares the next gate." },
        { "city",     "14  ALIEN MEGASTRUCTURE", "scene_41_alien_megastructure_frag", { "Tower Height", "Window Light", "Flight Speed", "Colour Family" },
          "Flying through a city of colossal alien monoliths. The kick sends a wave of light through the windows; the build lifts the camera over the city." },
        { "kifs",     "15  KIFS RELIQUARY",    "scene_42_kifs_spire_frag",        { "Fold Complexity", "Glow", "Orbit Speed", "Colour Family" },
          "A kaleidoscopic IFS sculpture that keeps refolding itself with the mids; the kick jolts the folds into a new arrangement." },
        { "hive",     "16  BIOMECH HIVE",      "scene_43_biomech_hive_frag",      { "Cell Size", "Honey Glow", "Flight Speed", "Colour Family" },
          "Flying inside a hexagonal biomechanical hive. The kick sends a pulse along the ribs; the peak lights the honey cores in every cell." },
        { "nebula",   "17  NEBULA DRIFT",      "scene_44_nebula_drift_frag",      { "Density", "Emission", "Drift Speed", "Colour Family" },
          "Volumetric flight through the glowing clouds of a stellar nursery. The kick is lightning inside the clouds; the build parts them to reveal a blazing core." },
        { "shrine",   "18  SIERPINSKI SHRINE", "scene_45_sierpinski_shrine_frag", { "Levels", "Inner Light", "Orbit Speed", "Colour Family" },
          "A Sierpinski tetrahedron, a pyramid of pyramids. The kick twists its folds, the peak lights every gap from within." },
        { "nexus",    "19  TORUS NEXUS",       "scene_46_torus_nexus_frag",       { "Rings", "Groove Light", "Spin Speed", "Colour Family" },
          "A dimension machine: nested rings turning around a living core. The build aligns the rings into a portal; the kick makes them expand." },
        { "image",    "20  IMAGE REACTOR",     "scene_17_image_reactor_frag",     { "Motion", "Reactivity", "Zoom", "Trails" },
          "Your own image becomes the visual: its structure, colours and contours drive generative modes (Flow Lines, Flow Paint, Pulse, ...), moved by the sound itself." },
    } };
    return s;
}

// Every scene's own starting point: the values that show it at its best. Loaded when the scene is
// chosen (by hand, by MIDI or by Auto Pilot) unless "Load Scene Init" is off; undoable.
struct SceneInit { float a, b, c, d, trip, intensity, speed, bloom, smooth; };
inline const SceneInit& sceneInit(int index)
{
    //                          A     B     C     D     trip  inten speed bloom smooth   (speed 0.5 = Motion x1.0)
    static const SceneInit t[] = {
        /* 01 Kali        */ { 0.55f, 0.60f, 0.45f, 0.00f, 0.45f, 0.85f, 0.55f, 0.50f, 0.55f },
        /* 02 Tidal       */ { 0.50f, 0.60f, 0.40f, 0.00f, 0.30f, 0.90f, 0.45f, 0.45f, 0.75f },
        /* 03 Tunnel      */ { 0.60f, 0.50f, 0.55f, 0.30f, 0.55f, 0.90f, 0.65f, 0.55f, 0.45f },
        /* 04 Apollonian  */ { 0.45f, 0.60f, 0.40f, 0.00f, 0.50f, 0.90f, 0.50f, 0.55f, 0.65f },
        /* 05 Gyroid      */ { 0.50f, 0.70f, 0.45f, 0.00f, 0.45f, 0.95f, 0.50f, 0.60f, 0.70f },
        /* 06 Mandelbulb  */ { 0.40f, 0.55f, 0.35f, 0.50f, 0.55f, 0.85f, 0.45f, 0.55f, 0.60f },
        /* 07 Fourth Dim  */ { 0.50f, 0.60f, 0.45f, 0.00f, 0.40f, 0.80f, 0.50f, 0.45f, 0.60f },
        /* 08 Mandelbox   */ { 0.50f, 0.50f, 0.35f, 0.20f, 0.45f, 0.85f, 0.45f, 0.50f, 0.65f },
        /* 09 Crystal     */ { 0.60f, 0.65f, 0.35f, 0.00f, 0.50f, 0.90f, 0.40f, 0.65f, 0.70f },
        /* 10 Menger      */ { 0.60f, 0.50f, 0.50f, 0.00f, 0.60f, 0.90f, 0.60f, 0.45f, 0.50f },
        /* 11 Julia       */ { 0.55f, 0.60f, 0.35f, 0.00f, 0.45f, 0.80f, 0.45f, 0.55f, 0.70f },
        /* 12 Ocean       */ { 0.50f, 0.50f, 0.50f, 0.00f, 0.25f, 0.90f, 0.50f, 0.50f, 0.80f },
        /* 13 Gate        */ { 0.50f, 0.60f, 0.60f, 0.00f, 0.60f, 0.90f, 0.65f, 0.55f, 0.50f },
        /* 14 Megastruct. */ { 0.55f, 0.60f, 0.55f, 0.00f, 0.35f, 0.95f, 0.60f, 0.55f, 0.60f },
        /* 15 KIFS        */ { 0.55f, 0.60f, 0.40f, 0.40f, 0.55f, 0.85f, 0.45f, 0.55f, 0.60f },
        /* 16 Biomech     */ { 0.50f, 0.60f, 0.55f, 0.00f, 0.50f, 0.90f, 0.60f, 0.55f, 0.55f },
        /* 17 Nebula      */ { 0.55f, 0.55f, 0.45f, 0.00f, 0.35f, 0.90f, 0.45f, 0.65f, 0.80f },
        /* 18 Sierpinski  */ { 0.50f, 0.60f, 0.40f, 0.00f, 0.55f, 0.90f, 0.45f, 0.55f, 0.60f },
        /* 19 Torus       */ { 0.60f, 0.60f, 0.50f, 0.00f, 0.50f, 0.90f, 0.55f, 0.60f, 0.55f },
        /* 20 Image       */ { 0.50f, 0.50f, 0.50f, 0.50f, 0.30f, 0.90f, 0.50f, 0.30f, 0.50f },
    };
    constexpr int n = int(sizeof(t) / sizeof(t[0]));
    return t[index < 0 ? 0 : (index >= n ? n - 1 : index)];
}

struct EffectInfo
{
    const char* id;
    const char* name;
    const char* resource;
    const char* p1Name;
    const char* p2Name;
    bool  stateful;                 // needs its own history buffer
    float defAmt, defP2;
};

inline const std::array<EffectInfo, 20>& effectLibrary()
{
    static const std::array<EffectInfo, 20> e { {
        { "blur",        "Blur",                 "fx_blur_frag",          "Radius",   "Radial",     false, 0.30f, 0.0f },
        { "glow",        "Glow",                 "fx_glow_frag",          "Amount",   "Threshold",  false, 0.45f, 0.35f },
        { "feedback",    "Feedback",             "fx_feedback_frag",      "Persist",  "Zoom/Rot",   true,  0.70f, 0.60f },
        { "kaleido",     "Kaleidoscope",         "fx_kaleidoscope_frag",  "Mix",      "Segments",   false, 1.00f, 0.30f },
        { "mirror",      "Mirror",               "fx_mirror_frag",        "Mix",      "Mode",       false, 1.00f, 0.00f },
        { "twist",       "Twist",                "fx_twist_frag",         "Amount",   "Radius",     false, 0.30f, 0.50f },
        { "warp",        "Warp",                 "fx_warp_frag",          "Amount",   "Scale",      false, 0.30f, 0.40f },
        { "noise",       "Noise",                "fx_noise_frag",         "Grain",    "Size",       false, 0.25f, 0.20f },
        { "chromatic",   "Chromatic Aberration", "fx_chromatic_frag",     "Amount",   "Lateral",    false, 0.30f, 0.00f },
        { "rgbsplit",    "RGB Split",            "fx_rgbsplit_frag",      "Offset",   "Angle",      false, 0.25f, 0.00f },
        { "displace",    "Displacement",         "fx_displacement_frag",  "Depth",    "Gradient",   false, 0.30f, 0.50f },
        { "pixelate",    "Pixelation",           "fx_pixelate_frag",      "Size",     "Dots",       false, 0.40f, 0.00f },
        { "posterize",   "Posterization",        "fx_posterize_frag",     "Amount",   "Gamma",      false, 0.50f, 0.50f },
        { "invert",      "Invert",               "fx_invert_frag",        "Mix",      "Luma Only",  false, 1.00f, 0.00f },
        { "contrast",    "Contrast",             "fx_contrast_frag",      "Contrast", "Pivot",      false, 0.60f, 0.35f },
        { "brightness",  "Brightness",           "fx_brightness_frag",    "Gain",     "Lift",       false, 0.60f, 0.00f },
        { "saturation",  "Saturation",           "fx_saturation_frag",    "Amount",   "Vibrance",   false, 0.65f, 0.30f },
        { "hueshift",    "Hue Shift",            "fx_hueshift_frag",      "Offset",   "Rotate",     false, 0.10f, 0.00f },
        { "vignette",    "Vignette",             "fx_vignette_frag",      "Amount",   "Softness",   false, 0.50f, 0.50f },
        { "trails",      "Trails",               "fx_trails_frag",        "Decay",    "Colour Drift", true, 0.75f, 0.20f },
    } };
    return e;
}

inline const char* const* templateModeNames()
{
    static const char* n[] = { "Kaleidoscope", "Mandala", "Tunnel", "Recursive", "Rotating Geometry", "Organic", "Feedback Echo" };
    return n;
}
constexpr int kNumTemplateModes = 7;

inline const char* const* templateBlendNames()
{
    static const char* n[] = { "Screen", "Add", "Mask", "Replace" };
    return n;
}
constexpr int kNumTemplateBlends = 4;
} // namespace dali
