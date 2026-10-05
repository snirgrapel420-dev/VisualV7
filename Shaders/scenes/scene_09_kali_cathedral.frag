// 09 KALI CATHEDRAL — flight through a raymarched kaleidoscopic fractal.
//   bass      flight speed (uBassTime) + the structure breathes
//   kick      walls ignite (glow burst)
//   snare     fold jolt (the architecture twists on every hit)
//   hi-hat    sparkle on the fine detail
//   spectrum  colours the orbit trap: each fractal depth takes its colour from a frequency band
// Macros: A Fold Twist · B Complexity · C Flight Speed · D Glow

vec3 K0, K1, K2, K3;
void setPalette()
{
    vec3 w = familyWeights(colourFamily(uMacro.w));
    // A: midnight · sapphire · teal · antique gold   B: plum · magenta · violet · amber   C: obsidian · emerald · jade · ivory
    vec3 a0 = hex(198413.0), a1 = hex(1388429.0), a2 = hex(1810317.0), a3 = hex(14135904.0);
    vec3 b0 = hex(1310992.0), b1 = hex(10817386.0), b2 = hex(6177449.0), b3 = hex(16756054.0);
    vec3 c0 = hex(132611.0), c1 = hex(1013332.0), c2 = hex(5096624.0), c3 = hex(15790800.0);
    K0 = a0 * w.x + b0 * w.y + c0 * w.z; K1 = a1 * w.x + b1 * w.y + c1 * w.z;
    K2 = a2 * w.x + b2 * w.y + c2 * w.z; K3 = a3 * w.x + b3 * w.y + c3 * w.z;
}
// the lit architecture uses the three bright colours, travelling back and forth (no wrap-around jump)
vec3 kpal(float t) { return ramp4(0.34 + 0.66 * abs(fract(t * 0.5) * 2.0 - 1.0), K0, K1, K2, K3) * 1.5; }

float gTrap;          // orbit trap (min distance to origin during the folds)
float gDepth;         // iteration where the trap was hit (-> spectrum colour)

float de(vec3 pos)
{
    vec3 tp = pos;
    tp.xz = abs(0.5 - mod(tp.xz, 1.0));                    // endless halls in every direction
    vec4 p = vec4(tp, 1.0);
    float y = max(0.0, 0.35 - abs(pos.y - 3.35)) / 0.35;   // extra folding near the flight height
    float twist = 0.35 + 0.35 * uMacro.x + 0.06 * sin(uMidTime * 0.35) + 0.15 * uSnare + 0.12 * uMid + 0.12 * uState.w * sin(uHighTime * 0.8);   // mids twist the folds
    float sc = 2.0 + (0.2 + 0.25 * uBass) * y;             // the structure breathes with the bass
    mat2 r = rot(twist);
    gTrap = 1e9; gDepth = 0.0;
    int iters = 5 + int(uMacro.y * 3.0 + 0.5);
    for (int i = 0; i < 8; i++)
    {
        if (i >= iters) break;
        p.xyz = abs(p.xyz) - vec3(-0.02, 1.98, -0.02);
        p = p * sc / clamp(dot(p.xyz, p.xyz), 0.4, 1.0) - vec4(0.5, 1.0, 0.4, 0.0);
        p.xz *= r;
        float t = length(p.xyz) / p.w;
        if (t < gTrap) { gTrap = t; gDepth = float(i) / 7.0; }
    }
    return length(max(abs(p.xyz) - vec3(0.1, 5.0, 0.1), 0.0)) / p.w;
}

vec3 normalAt(vec3 p, float e)
{
    vec2 k = vec2(e, 0.0);
    return normalize(vec3(de(p + k.xyy) - de(p - k.xyy), de(p + k.yxy) - de(p - k.yxy), de(p + k.yyx) - de(p - k.yyx)));
}

void main()
{
    setPalette();
    vec2 uv = (gl_FragCoord.xy - 0.5 * uRes) / uRes.y;

    // camera: flight driven by the bass clock, gentle banking from the mids
    float fly = (uBassTime * (0.15 + 0.5 * uMacro.z) + uTime * 0.04) * 1.0;
    float stateLight = 0.75 + 0.45 * uState.z + 0.35 * uState.w - 0.35 * uState.y - 0.2 * uState.x + 0.6 * uDrop;
    vec3 ro = vec3(0.25 * sin(uMidTime * 0.17), 3.35 + 0.06 * sin(uBassTime * 0.23), fly);
    vec3 rd = normalize(vec3(uv * (1.0 + 0.12 * uKick), 1.2));
    rd = roleCamera(rd);                       // sub / mids / bassline move the camera
    rd.xy *= rot(0.25 * sin(uMidTime * 0.21) + 0.08 * uPan);
    rd.xz *= rot(0.18 * sin(uMidTime * 0.13));

    float t = 0.0, glow = 0.0, hitTrap = 1.0, hitDepth = 0.0;
    bool hit = false;
    for (int i = 0; i < 160; i++)
    {
        if (i >= stepBudget(160)) break;   // adaptive quality
        vec3 p = ro + rd * t;
        float d = de(p);
        glow += exp(-d * 300.0) * 0.008;                    // volumetric edge glow
        if (d < hitEps(t)) { hit = true; hitTrap = gTrap; hitDepth = gDepth; break; }
        t += d * 0.95;
        if (t > 6.0) break;
    }

    vec3 col = vec3(0.0);
    float fog = exp(-t * 0.55);
    if (hit)
    {
        vec3 p = ro + rd * t;
        vec3 n = normalAt(p, 0.0006 * t + 0.0002);
        vec3 l = normalize(vec3(0.5, 0.7, -0.4));
        float dif = max(dot(n, l), 0.0);
        float fre = pow(1.0 - max(dot(n, -rd), 0.0), 4.0);
        float ao = clamp(de(p + n * 0.02) / 0.02, 0.0, 1.0);

        // psychedelic colour: fold depth + trap + position, cycling with the mid clock
        float hue = hitDepth * 1.1 + smoothstep(0.0, 0.08, hitTrap) * 0.35 + p.z * 0.35 + 0.05 * uMidTime + uPalShift;
        vec3 base = kpal(hue);
        // the spectrum is carved into the architecture: each depth lights up with its band
        float band = spec(0.06 + hitDepth * 0.85);
        col = base * (0.04 + 0.95 * dif * dif) * (0.3 + 0.7 * ao);
        col += base * band * band * 1.6 * ao;                                     // emissive audio bands
        col += kpal(hue + 0.5) * fre * (0.15 + 0.9 * uHigh) * ao;
        col += kpal(hue + 0.25) * smoothstep(0.006, 0.0, hitTrap) * (0.1 + 1.6 * uHat);   // hat sparkle
        col *= fog;
    }
    // edge glow ignites on the kick
    float g = glow * (0.25 + 1.2 * uMacro.w) * (0.25 + 2.5 * uKick + 0.5 * uBass);
    col += kpal(0.62 + uPalShift + 0.03 * uMidTime) * g * 0.6;
    col *= stateLight;
    col *= uIntensity * 1.35 * mix(0.35, 1.0, uActivity);
    fragColor = vec4(col, 1.0);
}
