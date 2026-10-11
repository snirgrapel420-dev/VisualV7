// NEON GYROSCOPE — 3D: nested neon rings turning around a black mirror core, crossed by great beams of light.
//
// Self-driving: every part is automated from the music.
//   bass          the camera breathes in and out
//   kick          the rings punch outward, the beams flare
//   snare         a ring flips a quarter turn
//   hi-hat        the lamps on the rings sparkle
//   mids          the rings' rotation
//   highs         light runs along the dashed grooves
//   BUILD         the rings align into one plane: a portal
//   PEAK          more rings, everything blazes
//   CHAOS         the rings tumble on wild axes
//   colour        three colour families, the music moves between them
// Macros: A Mirror (none / left-right / four ways) · B Glow · C Spin Speed · D Colour Family

vec3 C0, C1, C2, C3;
void setPalette()
{
    vec3 w = familyWeights(colourFamily(uMacro.w));
    vec3 a0 = hex(1310740.0),  a1 = hex(16722902.0), a2 = hex(16758062.0), a3 = hex(16772567.0);
    vec3 b0 = hex(1311748.0),  b1 = hex(16727182.0), b2 = hex(16761402.0), b3 = hex(12579071.0);
    vec3 c0 = hex(393246.0),   c1 = hex(9063935.0),  c2 = hex(3073791.0),  c3 = hex(2976767.0);
    C0 = a0 * w.x + b0 * w.y + c0 * w.z; C1 = a1 * w.x + b1 * w.y + c1 * w.z;
    C2 = a2 * w.x + b2 * w.y + c2 * w.z; C3 = a3 * w.x + b3 * w.y + c3 * w.z;
}

float gEm = 1e9, gEmId = 0.0, gMat = 0.0;
float gSpin = 0.0, gAlign = 0.0, gPunch = 1.0, gChaos = 0.0;
int gRings = 4;

float sdSeg3(vec3 p, vec3 a, vec3 b) { vec3 pa = p - a, ba = b - a; return length(pa - ba * clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0)); }

float map(vec3 p)
{
    float d = 1e9;
    gEm = 1e9; gMat = 0.0;
    // the black mirror core
    float core = length(p) - 0.42 * gPunch;
    d = core; gMat = 1.0;

    for (int i = 0; i < 6; i++)
    {
        if (i >= gRings) break;
        float fi = float(i);
        float R = (0.75 + 0.36 * fi) * gPunch;
        float sgn = mod(fi, 2.0) < 0.5 ? 1.0 : -1.0;
        float flip = 0.5 * PI * floor(uSnare * 1.2) * step(0.5, hash12(vec2(fi, floor(uBeatClock))));
        vec3 q = p;
        float a1 = mix(gSpin * (0.6 + 0.3 * fi) * sgn + fi * 0.9 + flip, 0.0, gAlign) + gChaos * sin(uHighTime * (1.1 + 0.3 * fi));
        float a2 = mix(gSpin * (0.4 + 0.2 * fi) + fi * 1.7, 0.0, gAlign) + gChaos * cos(uHighTime * (0.9 + 0.2 * fi));
        q.yz *= rot(a1);
        q.xy *= rot(a2);
        // a flat band (glossy) with an emissive dashed groove in the middle
        vec2 tq = vec2(length(q.xz) - R, q.y);
        float band = length(max(abs(tq) - vec2(0.012, 0.045), 0.0)) - 0.008;
        if (band < d) { d = band; gMat = 0.0; }
        float ang = atan(q.z, q.x);
        float dash = abs(fract(ang * (5.0 + fi * 2.0) / PI - uHighTime * 0.3 * sgn) - 0.5);
        float groove = length(vec2(tq.x + 0.015, tq.y)) + max(dash - 0.3, 0.0) * R * 0.6;
        if (groove < gEm) { gEm = groove; gEmId = mod(fi, 2.0); }
        // lamps on the ring
        float la = mod(ang + PI / 4.0, PI / 2.0) - PI / 4.0;
        float lamp = length(vec3(length(q.xz) * cos(la) - R, q.y, length(q.xz) * sin(la))) - 0.028;
        if (lamp < gEm) { gEm = max(lamp, 0.0); gEmId = 2.0; }
    }
    return d;
}

vec3 emitCol(float id)
{
    if (id < 0.5) return C1;
    if (id < 1.5) return C2;
    if (id < 2.5) return mix(C2, vec3(1.0), 0.4) * (1.0 + 2.0 * uHat + uKick);
    return mix(C1, C3, 0.35) * (0.6 + 0.8 * uKick);
}

// smooth analytic glow of a light beam (segment a-b) seen along a ray: no marching artefacts on thin lines
float beamGlow(vec3 ro, vec3 rd, vec3 a, vec3 b, float tMax)
{
    vec3 u = normalize(b - a), w0 = ro - a;
    float bb = dot(rd, u), dd = dot(rd, w0), ee = dot(u, w0);
    float den = max(1.0 - bb * bb, 1e-4);
    float tr = clamp((bb * ee - dd) / den, 0.0, tMax);
    float sb = clamp(ee + bb * tr, 0.0, length(b - a));
    float dist = length(ro + rd * tr - (a + u * sb));
    return 0.0016 / (0.00012 + dist * dist);
}

vec3 normalAt(vec3 p)
{
    vec2 e = vec2(0.0012, 0.0);
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
        float h = clamp(d * 0.85, 0.006, 0.35);
        glow += emitCol(gEmId) * (glowK * 0.0018 / (0.00025 + gEm * gEm)) * h;
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
    float mirror = uMacro.x;
    if (mirror > 0.33) uv.x = abs(uv.x) - 0.04;
    if (mirror > 0.66) uv.y = abs(uv.y) - 0.03;

    gSpin = uMidTime * (0.15 + 0.35 * uMacro.z);
    gAlign = smoothstep(0.15, 0.9, build);
    gPunch = 1.0 + 0.1 * uKick + 0.04 * uDrop;
    gChaos = 0.9 * chaos;
    gRings = 3 + int(1.5 * uMacro.y + 1.5 * peak + 0.5);

    float orbit = uTime * 0.03 + 0.4 * sin(uBassTime * 0.03);
    float dist = 5.2 - 0.6 * uBassSlow - 0.8 * build;
    vec3 ro = vec3(sin(orbit) * dist, 1.3 + 0.6 * sin(uMidTime * 0.02), cos(orbit) * dist);
    vec3 fw = normalize(-ro);
    vec3 rt = normalize(cross(vec3(0.0, 1.0, 0.0), fw));
    vec3 up = cross(fw, rt);
    vec3 rd = normalize(uv.x * rt + uv.y * up + 1.5 * fw);
    rd = roleCamera(rd);

    float glowK = mix(0.6, 1.4, uMacro.y) * (1.0 + 1.0 * uKick + 0.5 * peak) * mix(0.55, 1.0, uActivity);
    float t; bool hit;
    vec3 col = march(ro, rd, stepBudget(110), 14.0, t, hit, glowK);
    if (hit)
    {
        vec3 p = ro + rd * t;
        float mat = gMat, em = gEm, id = gEmId;
        vec3 n = normalAt(p);
        vec3 rr = reflect(rd, n);
        float t2; bool hit2;
        vec3 refl = march(p + n * 0.02, rr, stepBudget(40), 8.0, t2, hit2, glowK);
        float fres = 0.1 + 0.9 * pow(1.0 - clamp(dot(-rd, n), 0.0, 1.0), 3.0);
        vec3 key = normalize(vec3(0.4, 0.8, 0.5));
        float spec = pow(clamp(dot(rr, key), 0.0, 1.0), 50.0);
        if (mat > 0.5) col += refl * (0.45 + 0.55 * fres) + C0 * 0.2;                         // the mirror core
        else col += C0 * 0.3 + C2 * 0.12 * clamp(dot(n, key), 0.0, 1.0) + refl * (0.3 + 0.7 * fres) + spec * 0.5;
        col += emitCol(id) * 3.0 * (1.0 - smoothstep(0.0, 0.025, em));
    }
    // two great beams crossing through the core (an X), drawn analytically
    {
        float tm = hit ? t : 30.0;
        vec3 bc = mix(C1, C3, 0.3) * (0.5 + 0.9 * uKick + 0.4 * peak) * glowK;
        col += bc * beamGlow(ro, rd, vec3(-4.0, -2.6, 0.0) * gPunch, vec3(4.0, 2.6, 0.0) * gPunch, tm);
        col += bc * beamGlow(ro, rd, vec3(-4.0, 2.6, 0.0) * gPunch, vec3(4.0, -2.6, 0.0) * gPunch, tm);
        col += C2 * 0.6 * glowK * beamGlow(ro, rd, vec3(0.0, -3.5, 0.0), vec3(0.0, 3.5, 0.0), tm) * (0.4 + uHigh);
    }
    // the portal plane on BUILD and a dark nebula behind
    col += C1 * gAlign * 0.18 * exp(-abs(uv.y) * 10.0);
    col += C0 * 0.7 * (1.0 - smoothstep(0.0, 1.3, length(uv)));

    col *= (1.0 + 0.3 * peak) * (1.0 - 0.15 * calm);
    col *= uIntensity * mix(0.6, 1.0, uActivity);
    fragColor = vec4(safeHDR(col), 1.0);
}
