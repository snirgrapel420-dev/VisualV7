// NEON HELIX TOWER — 3D: a twisting tower of neon bars rising past the camera, mirrored left and right.
//
// Self-driving: every part is automated from the music.
//   bass          the tower rises past the camera with the bass
//   kick          every bar shoots out wider and flares
//   snare         every second bar flashes white
//   hi-hat        the golden lamps on the spine sparkle
//   mids          the twist turns; the camera orbits
//   highs         light runs along the bars
//   BUILD         the twist winds tighter and faster
//   PEAK          a second strand joins (a double helix), everything blazes
//   CHAOS         the bars scatter out of line
//   colour        three colour families, the music moves between them
// Macros: A Strands · B Glow · C Rise Speed · D Colour Family

vec3 C0, C1, C2, C3;
void setPalette()
{
    vec3 w = familyWeights(colourFamily(uMacro.w));
    vec3 a0 = hex(1310740.0),  a1 = hex(16722902.0), a2 = hex(16769214.0), a3 = hex(16758062.0);
    vec3 b0 = hex(1311748.0),  b1 = hex(16727182.0), b2 = hex(16772567.0), b3 = hex(16761402.0);
    vec3 c0 = hex(393246.0),   c1 = hex(9063935.0),  c2 = hex(12579071.0), c3 = hex(3073791.0);
    C0 = a0 * w.x + b0 * w.y + c0 * w.z; C1 = a1 * w.x + b1 * w.y + c1 * w.z;
    C2 = a2 * w.x + b2 * w.y + c2 * w.z; C3 = a3 * w.x + b3 * w.y + c3 * w.z;
}

const float SP = 0.34;          // vertical spacing of the bars
float gEm = 1e9, gEmId = 0.0, gAlong = 0.0, gLevel = 0.0;
float gTwist = 0.6, gSpin = 0.0, gLen = 1.2, gStrands = 1.0, gScatter = 0.0;

float sdSeg3(vec3 p, vec3 a, vec3 b)
{
    vec3 pa = p - a, ba = b - a;
    return length(pa - ba * clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0));
}

float map(vec3 p)
{
    p.x = abs(p.x);                                              // mirrored left / right
    float k = floor(p.y / SP + 0.5);
    float d = 1e9;
    gEm = 1e9;
    for (int j = -1; j <= 1; j++)
    {
        float kk = k + float(j);
        float y = kk * SP;
        for (int s = 0; s < 3; s++)
        {
            if (float(s) >= gStrands) break;
            float th = kk * gTwist + gSpin + float(s) * PI / gStrands + gScatter * (hash12(vec2(kk, float(s))) - 0.5) * 3.0;
            vec3 dir = vec3(cos(th), 0.0, sin(th));
            float L = gLen * (0.75 + 0.25 * sin(kk * 0.9 + float(s)));
            vec3 a = vec3(0.0, y, 0.0) - dir * L, b = vec3(0.0, y, 0.0) + dir * L;
            float seg = min(sdSeg3(p, a, b), sdSeg3(p, vec3(-a.x, a.y, a.z), vec3(-b.x, b.y, b.z)));
            float bar = seg - 0.026;
            if (bar < d) { d = bar; }
            if (seg < gEm)
            {
                gEm = seg; gLevel = kk;
                vec3 pa = p - a;
                gAlong = abs(dot(pa, dir) / L - 1.0);                   // 0 centre .. 1 tips
                gEmId = float(s);
            }
        }
    }
    // the spine with golden lamps
    float spine = length(p.xz) - 0.03;
    d = min(d, spine);
    float lampY = mod(p.y + SP * 0.5, SP * 2.0) - SP;
    float lamp = length(vec3(p.x, lampY, p.z)) - 0.05;
    if (lamp < gEm) { gEm = max(lamp, 0.0); gEmId = 5.0; }
    return d;
}

vec3 emitCol()
{
    if (gEmId > 4.5) return C3 * 1.5 * (1.0 + 2.0 * uHat + uKick);
    vec3 mid = mod(gLevel, 3.0) < 0.5 ? C3 : mix(C2, vec3(1.0), 0.15);               // every third bar golden
    vec3 c = mix(mid, C1, smoothstep(0.5, 0.95, gAlong));                               // hot coloured tips
    c = mix(c, C3, 0.4 * gEmId);
    float flash = uSnare * step(0.5, mod(gLevel, 2.0)) * (0.5 + 0.6 * uMacro.y);
    c = mix(c, vec3(1.0), clamp(flash, 0.0, 0.8));
    float run = 0.75 + 0.6 * smoothstep(0.85, 1.0, sin(gAlong * 12.0 - uHighTime * 5.0 + gLevel));
    return c * run;
}

vec3 normalAt(vec3 p)
{
    vec2 e = vec2(0.0015, 0.0);
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
        float h = clamp(d * 0.8, 0.01, 0.3);
        glow += emitCol() * (glowK * 0.0006 / (0.0003 + gEm * gEm)) * h;
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
    gTwist = 0.38 + 0.5 * build + 0.2 * uMid;
    gSpin = uMidTime * (0.12 + 0.25 * build);
    gLen = (1.7 + 0.35 * uKick + 0.1 * uDrop) * (1.0 + 0.08 * peak);
    gStrands = clamp(1.0 + floor(uMacro.x * 1.99) + step(0.55, peak), 1.0, 3.0);
    gScatter = 0.6 * chaos;

    float rise = uBassTime * (0.25 + 0.7 * uMacro.z) + uTime * (0.1 + 0.2 * uMacro.z);
    float orbit = 0.35 * sin(uMidTime * 0.04) + uTime * 0.01;
    vec3 target = vec3(0.0, rise + 0.25, 0.0);
    vec3 ro = target + vec3(sin(orbit) * 2.5, -0.3 + 0.25 * sin(uBassTime * 0.05), -cos(orbit) * 2.5);
    vec3 fw = normalize(target - ro);
    vec3 rt = normalize(cross(vec3(0.0, 1.0, 0.0), fw));
    vec3 up = cross(fw, rt);
    vec3 rd = normalize(uv.x * rt + uv.y * up + 1.05 * fw);
    rd = roleCamera(rd);

    float glowK = mix(0.5, 1.1, uMacro.y) * (1.0 + 0.8 * uKick + 0.3 * peak) * mix(0.55, 1.0, uActivity);
    float t; bool hit;
    vec3 col = march(ro, rd, stepBudget(100), 14.0, t, hit, glowK);
    if (hit)
    {
        vec3 p = ro + rd * t;
        vec3 n = normalAt(p);
        vec3 ec = emitCol();
        float core = 1.0 - smoothstep(0.03, 0.06, gEm);
        // glass tube: bright emissive core with a glossy rim reflecting the other bars
        float fres = pow(1.0 - clamp(dot(-rd, n), 0.0, 1.0), 3.0);
        col += ec * (0.8 + 0.9 * core) + mix(C1, C2, 0.5) * fres * 0.4;
    }
    // the space around the tower: a faint mirrored backdrop glow and depth haze
    col += C0 * 0.6 * (1.0 - smoothstep(0.0, 1.2, length(uv)));
    col += mix(C1, C2, 0.4) * 0.08 * (1.0 - exp(-0.08 * t)) * (0.5 + peak);

    col *= (1.0 + 0.3 * peak) * (1.0 - 0.15 * calm);
    col *= uIntensity * mix(0.6, 1.0, uActivity);
    fragColor = vec4(safeHDR(col), 1.0);
}
