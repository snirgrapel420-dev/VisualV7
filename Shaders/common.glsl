// ============================================================================
//  DALI VISUAL — common shader header
//  Prepended (after "#version 150") to every fragment shader by ShaderLibrary.
//  All audio uniforms are already smoothed and reactivity-scaled on the CPU.
// ============================================================================

in vec2 vUV;
out vec4 fragColor;

// --- frame ---------------------------------------------------------------
uniform vec2  uRes;          // target size in pixels
uniform float uTime;         // scene time (speed/motion scaled), seconds
uniform float uAbsTime;      // wall-clock seconds since engine start (wrapped)

// --- audio features (0..1 unless stated) ---------------------------------
uniform float uBass;         // low band envelope
uniform float uMid;          // mid band envelope
uniform float uHigh;         // high band envelope
uniform float uEnergy;       // overall energy
uniform float uKick;         // kick envelope (fast attack / decay)
uniform float uTransient;    // transient burst envelope
uniform float uBeat;         // beat pulse (1 on beat, decays)
uniform float uCentroid;     // spectral centroid (log scaled)
uniform float uFlux;         // spectral flux
uniform float uWidth;        // stereo width
uniform float uPan;          // stereo balance  -1..1
uniform float uSnare;        // snare / clap envelope
uniform float uHat;          // hi-hat envelope
uniform float uBuild;        // 0..1 tension of a breakdown / build-up
uniform float uDrop;         // 1 at a drop (kick returns after a breakdown), decays
uniform float uActivity;     // 0 in silence .. 1 while music plays
// time that only advances while its band sounds (Synesthesia-style "band time"):
// motion tied to uBassTime moves with the bass and stops when the bass stops
uniform float uBassTime, uMidTime, uHighTime, uLevelTime;
// six bands, time scales, densities and the musical state
uniform float uSub, uLowMid, uHighMid;            // FAST bands (bass/mid/high are uBass/uMid/uHigh)
uniform float uBassNote;      // a rolling-bassline note between the beats (role-separated from the kick)
uniform float uEnergyMed, uMidMed;                // MEDIUM (~0.5 s)
uniform float uBassSlow, uEnergySlow, uCentroidSlow, uFluxSlow;   // SLOW (2-4 s)
uniform float uKickDensity, uOnsetDensity, uDynRange;
uniform vec4  uState;          // smoothed weights: x CALM, y BUILD, z PEAK, w CHAOS
uniform float uStateRelease;   // weight of RELEASE (all five sum to 1)
uniform float uStateTime;      // seconds in the current state

// --- musical clock -------------------------------------------------------
uniform float uBeatPhase;    // 0..1 within the current beat
uniform float uBarPhase;     // 0..1 within the current bar
uniform float uSyncPhase;    // 0..1 within the selected sync division
uniform float uBeatClock;    // continuous beat count (wrapped at 256)

// --- scene controls ------------------------------------------------------
uniform vec4  uMacro;        // scene macros A..D (0..1)
uniform float uIntensity;    // global intensity

// --- colour --------------------------------------------------------------
uniform vec3  uPalA;
uniform vec3  uPalB;
uniform vec3  uPalC;
uniform vec3  uPalD;
uniform float uPalShift;
uniform float uColorAmount;

// --- textures ------------------------------------------------------------
uniform sampler2D uTex;      // effect input / primary input
uniform sampler2D uPrev;     // this pass' own previous frame (feedback)
uniform sampler2D uSpectrum; // 128 x 2: row 0 = log spectrum 30 Hz..16 kHz (0..1), row 1 = waveform (-1..1)

#define PI  3.14159265359
#define TAU 6.28318530718

mat2 rot(float a) { float c = cos(a), s = sin(a); return mat2(c, -s, s, c); }

float hash12(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec2 hash22(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * vec3(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.xx + p3.yz) * p3.zy);
}

float vnoise(vec2 p)
{
    vec2 i = floor(p), f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    float a = hash12(i);
    float b = hash12(i + vec2(1.0, 0.0));
    float c = hash12(i + vec2(0.0, 1.0));
    float d = hash12(i + vec2(1.0, 1.0));
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float fbm(vec2 p, int octaves)
{
    float v = 0.0, a = 0.5;
    mat2 m = mat2(1.6, 1.2, -1.2, 1.6);
    for (int i = 0; i < 8; ++i)
    {
        if (i >= octaves) break;
        v += a * vnoise(p);
        p = m * p;
        a *= 0.5;
    }
    return v;
}

// Musical ease: holds, then snaps forward — used for beat-locked motion.
float beatEase(float ph) { float x = clamp(ph, 0.0, 1.0); return 1.0 - pow(1.0 - x, 4.0); }

// Continuous musical travel: whole beats + eased fraction (monotonic, seamless).
float beatTravel() { return floor(uBeatClock) + beatEase(fract(uBeatClock)); }

// Palette lookup (cosine palette), respects Color Amount (0 = monochrome).
vec3 palette(float t)
{
    vec3 c = uPalA + uPalB * cos(TAU * (uPalC * t + uPalD + uPalShift));
    c = clamp(c, 0.0, 1.0);
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    return mix(vec3(l), c, uColorAmount);
}

// Aspect-correct centred coordinates, y in -0.5..0.5
vec2 centered() { return (gl_FragCoord.xy - 0.5 * uRes) / uRes.y; }

vec2 screenUV() { return gl_FragCoord.xy / uRes; }

vec3 safeHDR(vec3 c) { return clamp(c, 0.0, 8.0); }

// smoothstep with descending edges (defined behaviour on every driver)
float smoothstepR(float e0, float e1, float x) { return 1.0 - smoothstep(e1, e0, x); }

// ---- the sound itself ----------------------------------------------------------
// spectrum at log-frequency position x (0 = 30 Hz, 0.5 ~ 700 Hz, 1 = 16 kHz)
float spec(float x)  { return texture(uSpectrum, vec2(clamp(x, 0.0, 1.0), 0.25)).r; }
// average level over a log-frequency range
float specBand(float a, float b)
{
    float s = 0.0;
    for (int i = 0; i < 6; ++i) s += spec(mix(a, b, (float(i) + 0.5) / 6.0));
    return s / 6.0;
}
// waveform at position x (0..1 across the latest ~11 ms)
float wave(float x)  { return texture(uSpectrum, vec2(fract(x), 0.75)).r; }

// ---- quality helpers -----------------------------------------------------------------
// ACES-fitted filmic curve (keeps saturated neon without clipping to white)
vec3 aces(vec3 x) { return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0); }
// iq's rich cosine palette on the current palette uniforms, with an extra brightness lift
vec3 pal(float t) { return palette(t); }

// ---- designed colour: a smooth ramp through four chosen colours (no rainbow cycling) ------
vec3 ramp4(float t, vec3 c0, vec3 c1, vec3 c2, vec3 c3)
{
    t = clamp(t, 0.0, 1.0) * 3.0;
    vec3 a = mix(c0, c1, smoothstep(0.0, 1.0, t));
    vec3 b = mix(c1, c2, smoothstep(1.0, 2.0, t));
    vec3 c = mix(c2, c3, smoothstep(2.0, 3.0, t));
    return t < 1.0 ? a : (t < 2.0 ? b : c);
}
vec3 hex(float h) { return vec3(floor(h / 65536.0), mod(floor(h / 256.0), 256.0), mod(h, 256.0)) / 255.0; }
// smooth minimum (organic blending of shapes)
float smin(float a, float b, float k) { float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0); return mix(b, a, h) - k * h * (1.0 - h); }

// ---- colour that evolves with the music -----------------------------------------------------
// Position 0..2 across a scene's three colour families. Macro D sets the starting family; the
// musical state moves through them on its own: BUILD leans forward, CHAOS pushes to the far
// family, CALM settles back, and a very slow drift (driven by the level clock) keeps long passages
// from standing still.
uniform float uColourShift;    // COLOR tab "Colour Family": moves every scene through its families (wraps)
uniform float uMusicColour;    // COLOR tab "Music -> Colour": 0 fixed colour .. 1 full response to the music
float colourFamily(float macroD)
{
    float music = 0.45 * uState.y + 0.75 * uState.z * smoothstep(0.3, 0.9, uEnergySlow) + 1.1 * uState.w
                - 0.35 * uState.x + 0.35 * sin(uLevelTime * 0.02);
    float v = macroD * 2.0 + uColourShift * 4.0 + music * (2.0 * uMusicColour) + 0.35;
    return 2.0 - abs(mod(v, 4.0) - 2.0);          // ping-pong 0..2..0: any shift keeps flowing
}
// weights of the three families for a position v (cross-fade, sum 1)
vec3 familyWeights(float v)
{
    float wa = clamp(1.0 - v, 0.0, 1.0), wc = clamp(v - 1.0, 0.0, 1.0);
    return vec3(wa, 1.0 - wa - wc, wc);
}

// ---- resolution-true raymarching ---------------------------------------------------------------
// the surface is reached when the distance is below the footprint of one pixel at depth t:
// at 4K the march resolves 4x finer detail than at 1080p, automatically
uniform float uQuality;        // 0..1, set by the adaptive quality controller (1 = full detail)
// steps the march may take at this quality (a scene's budget scaled down when the GPU is short of time)
int stepBudget(int full) { return int(float(full) * mix(0.45, 1.0, clamp(uQuality, 0.0, 1.0))); }
// one pixel of footprint at full quality, up to ~3 pixels when the controller trades detail for speed
float hitEps(float t) { return t * mix(2.2, 0.9, clamp(uQuality, 0.0, 1.0)) / uRes.y + 1e-5; }

// ---- the role layer of the camera, shared by every 3D scene ----------------------------------------
// SUB breathes the field of view; MIDS sway the camera; the BASSLINE (16ths between the kicks) gives a
// small rhythmic throb. Each scene adds its own structural response on top of this.
vec3 roleCamera(vec3 rd)
{
    float sway = 0.05 * uMid + 0.03 * uMidMed;
    rd.xy *= rot(sway * sin(uMidTime * 0.7));
    rd.yz *= rot(0.6 * sway * cos(uMidTime * 0.53));
    float fov = 1.0 + 0.06 * uSub + 0.025 * uBassNote;
    return normalize(vec3(rd.xy * fov, rd.z));
}

// ---- a living backdrop for scenes built around one object: the frame is never an empty black void ----
// a slow nebula in the scene's own colours + stars, by ray direction (no seams), moved by the music
vec3 sceneBackdrop(vec3 rd, vec3 A, vec3 B, vec3 C)
{
    vec2 q = rd.xy / (1.0 + abs(rd.z)) * 2.2 + vec2(rd.z * 0.7, 0.0);
    float n  = fbm(q * 1.4 + vec2(uMidTime * 0.012, uBassTime * 0.006), 5);
    float n2 = fbm(q * 3.1 - vec2(uBassTime * 0.008, 0.0), 4);
    vec3 neb = mix(A * 0.6 + B * 0.12, B * 0.75, smoothstep(0.3, 0.75, n));
    neb = mix(neb, C * 0.8, smoothstep(0.55, 0.95, n * n2 * 1.7) * 0.55);
    neb *= 0.55 + 0.35 * uEnergyMed + 0.25 * uState.z + 0.35 * uState.w;
    vec2 sg = q * 70.0;
    float st = step(0.994, hash12(floor(sg))) * smoothstep(0.32, 0.0, length(fract(sg) - 0.5));
    return neb + C * st * (0.5 + 0.8 * uHigh);
}
