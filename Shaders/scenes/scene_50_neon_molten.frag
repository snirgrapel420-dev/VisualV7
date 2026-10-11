// NEON LIQUID GOLD — 3D: drifting between waves of molten gold, past rows of white-hot bars that melt with the music,
// with embers rising through a warm haze. (Identity: warm metal — gold / amber / copper.)
//
// Self-driving: every part is automated from the music.
//   bass          the chrome swells; the drift speed
//   kick          a shock wave runs through the chrome, the bars flare
//   snare         the bars flash white
//   hi-hat        the side flames flicker, the embers spark
//   mids          the bars ripple and melt
//   highs         the reflections sharpen
//   BUILD         the neon melts further and drips
//   PEAK          everything blazes, the chrome boils with light
//   CHAOS         the liquid boils
//   colour        three colour families, the music moves between them
// Macros: A Melt · B Glow · C Drift Speed · D Colour Family

vec3 C0, C1, C2, C3;
void setPalette()
{
    vec3 w = familyWeights(colourFamily(uMacro.w));
    // A: amber · white gold · gold     B: copper red · rose gold · orange     C: ice blue · white · silver lilac (platinum)
    vec3 a0 = hex(1313282.0), a1 = hex(16751130.0), a2 = hex(16770992.0), a3 = hex(16761402.0);
    vec3 b0 = hex(1312004.0), b1 = hex(16730652.0), b2 = hex(16760992.0), b3 = hex(16742954.0);
    vec3 c0 = hex(329742.0),  c1 = hex(7260415.0),  c2 = hex(15922943.0), c3 = hex(12103935.0);
    C0 = a0 * w.x + b0 * w.y + c0 * w.z; C1 = a1 * w.x + b1 * w.y + c1 * w.z;
    C2 = a2 * w.x + b2 * w.y + c2 * w.z; C3 = a3 * w.x + b3 * w.y + c3 * w.z;
}

float gEm = 1e9, gEmId = 0.0, gMelt = 0.3, gT = 0.0, gShock = 0.0, gRow = 0.0;

float waves(vec2 xz)
{
    float t = gT;
    float h = 0.18 * sin(xz.x * 1.3 + t * 0.7) * cos(xz.y * 0.9 - t * 0.5)
            + 0.09 * sin(xz.x * 3.1 - xz.y * 2.3 + t * 1.3)
            + 0.04 * sin(xz.x * 7.0 + xz.y * 5.0 - t * 2.1);
    h *= 1.0 + 0.8 * gMelt;
    h += gShock * 0.12 * sin(length(xz) * 4.0 - uBeatPhase * 8.0);
    return h;
}

float map(vec3 p)
{
    vec3 q = vec3(abs(p.x), abs(p.y), p.z);                       // mirrored left-right and up-down
    // liquid chrome above and below
    float chrome = (1.25 - waves(p.xz * vec2(1.0, 0.7)) - q.y) * 0.6;
    float d = chrome;

    gEm = 1e9;
    // rows of neon bars across the hall, melting
    float row = floor(p.z / 1.4 + 0.5);
    float zc = p.z - row * 1.4;
    for (int i = 0; i < 3; i++)
    {
        float fi = float(i);
        float yb = 0.16 + fi * 0.27;
        float wob = gMelt * (0.08 + 0.06 * fi) * sin(q.x * 2.6 + gT * 1.4 + row * 1.7 + fi)
                  + gMelt * 0.05 * smoothstep(0.4, 1.3, q.x) * sin(gT * 2.0 + row);
        float len = 0.55 + 0.6 * hash12(vec2(row, fi));
        vec2 dd = vec2(max(q.x - len, 0.0) + max(0.08 - q.x, 0.0), q.y - yb - wob);
        float bar = length(vec3(dd, zc));
        if (bar < gEm) { gEm = bar; gEmId = fi; gRow = row; }
        d = min(d, bar - 0.022);
    }
    // flames on both sides
    float fx = 1.55 + 0.08 * sin(q.y * 6.0 + gT * 3.0 + p.z) + 0.05 * sin(q.y * 13.0 - gT * 5.0);
    float flame = length(vec2(q.x - fx, zc * 0.6)) + max(q.y - 0.9, 0.0);
    if (flame < gEm) { gEm = flame; gEmId = 5.0; }
    return d;
}

vec3 emitCol(float id)
{
    if (id > 4.5) return C1 * 1.4 * (0.7 + 1.4 * uHat);
    vec3 c = id < 0.5 ? C2 : (id < 1.5 ? C3 : mix(C2, C1, 0.35));
    float flash = uSnare * step(0.5, mod(gRow, 2.0)) * (0.5 + 0.6 * uMacro.y);
    return mix(c, vec3(1.0), clamp(flash, 0.0, 0.8));
}

// embers rising through the hall (screen space, three depth layers drifting up and swaying)
vec3 embers(vec2 uv)
{
    vec3 c = vec3(0.0);
    for (int l = 0; l < 3; l++)
    {
        float fl = float(l);
        float sc = 6.0 + 5.0 * fl;
        vec2 g = uv * sc + vec2(0.4 * sin(uTime * 0.3 + fl * 2.0), -uTime * (0.25 + 0.12 * fl) - uBassTime * 0.15);
        vec2 id = floor(g), f = fract(g) - 0.5;
        vec2 o = hash22(id + fl * 11.0) - 0.5;
        float d = length((f - o * 0.6) * vec2(1.0, 0.6));
        float on = step(0.6, hash12(id * 1.3 + fl));
        float flick = 0.5 + 0.5 * sin(uHighTime * 4.0 + hash12(id) * 30.0);
        c += mix(C1, C2, hash12(id + 7.0)) * on * flick * (0.003 / (0.001 + d * d)) * (0.05 + 0.02 * fl);
    }
    return c * (0.5 + 1.5 * uHat + 0.6 * uState.z);
}

vec3 normalAt(vec3 p)
{
    vec2 e = vec2(0.002, 0.0);
    return normalize(vec3(map(p + e.xyy) - map(p - e.xyy), map(p + e.yxy) - map(p - e.yxy), map(p + e.yyx) - map(p - e.yyx)));
}

vec3 march(vec3 ro, vec3 rd, int steps, float far, out float t, out bool hit, float glowK)
{
    vec3 glow = vec3(0.0);
    t = 0.02; hit = false;
    for (int i = 0; i < 140; i++)
    {
        if (i >= steps) break;
        vec3 p = ro + rd * t;
        float d = map(p);
        float h = clamp(d * 0.8, 0.008, 0.3);
        glow += emitCol(gEmId) * (glowK * 0.0012 / (0.00035 + gEm * gEm)) * h;
        if (d < hitEps(t)) { hit = true; break; }
        t += h;
        if (t > far) break;
    }
    return glow;
}

void main()
{
    setPalette();
    vec2 uv = centered();
    float build = uState.y, peak = uState.z, chaos = uState.w, calm = uState.x;
    gMelt = clamp(0.2 + 0.6 * uMacro.x + 0.6 * build + 0.9 * chaos + 0.3 * uMid, 0.0, 2.0);
    gT = uMidTime * (0.6 + 0.8 * uMacro.z) + uTime * 0.2 + chaos * uHighTime * 0.5;
    gShock = uKick;

    float drift = uBassTime * (0.2 + 0.5 * uMacro.z) + uTime * (0.1 + 0.15 * uMacro.z);
    vec3 ro = vec3(0.0, 0.0, drift);
    vec3 rd = normalize(vec3(uv, 1.15));
    rd.xy *= rot(0.04 * sin(uMidTime * 0.08));
    rd = roleCamera(rd);

    float glowK = 0.75 * mix(0.6, 1.4, uMacro.y) * (1.0 + 1.0 * uKick + 0.5 * peak) * mix(0.55, 1.0, uActivity);
    float t; bool hit;
    vec3 col = march(ro, rd, stepBudget(100), 20.0, t, hit, glowK);
    if (hit)
    {
        vec3 p = ro + rd * t;
        float em = gEm, id = gEmId;
        vec3 n = normalAt(p);
        if (em < 0.035)
            col += emitCol(id) * 1.6;                                      // a bar's glowing glass
        else
        {
            // liquid chrome: almost a perfect mirror, tinted by the scene's deep colour
            vec3 rr = reflect(rd, n);
            float t2; bool hit2;
            vec3 refl = march(p + n * 0.03, rr, stepBudget(45), 10.0, t2, hit2, glowK);
            float fres = 0.35 + 0.65 * pow(1.0 - clamp(dot(-rd, n), 0.0, 1.0), 3.0);
            // molten metal: the reflection is coloured by the metal itself, with a hot glow in the troughs
            vec3 metal = mix(vec3(1.0), C3 * 1.35, 0.6);
            float trough = smoothstep(0.1, -0.15, waves(p.xz * vec2(1.0, 0.7)));
            col += refl * metal * fres * (0.8 + 0.4 * uHigh) + C0 * 0.4 + C1 * 0.18 * trough * (0.5 + uBass + peak);
        }
        col *= exp(-0.03 * t);
    }
    col += mix(C1, C3, 0.5) * 0.14 * (1.0 - exp(-0.05 * t));                // warm haze
    col += embers(uv);

    col *= (1.0 + 0.3 * peak) * (1.0 - 0.15 * calm);
    col *= uIntensity * mix(0.6, 1.0, uActivity);
    fragColor = vec4(safeHDR(col), 1.0);
}
