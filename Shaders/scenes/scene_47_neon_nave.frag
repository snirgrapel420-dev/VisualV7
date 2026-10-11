// NEON NAVE — 3D: flying down a mirrored hall of glossy black ribs, neon light strips and golden lamps.
//
// Self-driving: every part is automated from the music.
//   bass          the flight speed down the hall
//   kick          every strip flares, the lamps flash
//   snare         the diamond portals flash white
//   hi-hat        the lamps sparkle
//   mids          the camera sways; the strips' light runs
//   highs         fine reflections on the glossy surfaces
//   BUILD         the hall starts to twist into a spiral, the flight accelerates
//   PEAK          everything blazes, the portals open wide
//   CHAOS         the twist goes wild
//   colour        three colour families, the music moves between them
// Macros: A Twist · B Glow · C Flight Speed · D Colour Family

vec3 C0, C1, C2, C3;
void setPalette()
{
    vec3 w = familyWeights(colourFamily(uMacro.w));
    // A: magenta strips · gold lamps · white      B: hot pink · amber · ice      C: violet · cyan · electric blue
    vec3 a0 = hex(1310740.0),  a1 = hex(16722902.0), a2 = hex(16758062.0), a3 = hex(16772567.0);
    vec3 b0 = hex(1311748.0),  b1 = hex(16727182.0), b2 = hex(16761402.0), b3 = hex(12579071.0);
    vec3 c0 = hex(393246.0),   c1 = hex(9063935.0),  c2 = hex(3073791.0),  c3 = hex(2976767.0);
    C0 = a0 * w.x + b0 * w.y + c0 * w.z; C1 = a1 * w.x + b1 * w.y + c1 * w.z;
    C2 = a2 * w.x + b2 * w.y + c2 * w.z; C3 = a3 * w.x + b3 * w.y + c3 * w.z;
}

const float W = 2.0, H = 1.25, CELL = 2.2;
float gEm;      // distance to the nearest emissive strip
float gEmId;    // which emitter: 0 magenta strip, 1 gold lamp, 2 white strip, 3 portal
float gTwist = 0.0;
float gCell = 0.0;

float sdBox(vec3 p, vec3 b) { vec3 q = abs(p) - b; return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0); }

float map(vec3 p)
{
    p.xy *= rot(p.z * gTwist);                                   // BUILD / CHAOS: the hall spirals
    float cid = floor(p.z / CELL + 0.5);
    vec3 q = vec3(abs(p.xy), p.z - cid * CELL);                  // mirrored on both axes, one rib per cell

    // the hall itself (inside of a box) and the ribs (glossy black frames)
    float hall = min(W - q.x, H - q.y);
    vec2 corner = vec2(W - 0.22, H - 0.18);
    vec2 dq = q.xy - corner;
    float rect = max(dq.x, dq.y);                                // inside the rib's opening < 0
    float rib = max(abs(q.z) - 0.11, -rect - 0.0);               // a frame 0.22 thick around the opening
    rib = max(rib, -hall - 0.3);
    float solid = min(hall, rib);

    // ---- emitters (light strips, lamps, portals) ----
    gEm = 1e9; gEmId = 0.0;
    // 1. magenta strips along the rib's inner edge
    float e = length(vec2(max(dq.x, dq.y) + 0.01, abs(q.z) - 0.11));
    e = min(e, length(vec2(min(abs(dq.x), abs(dq.y)) + 0.0, abs(q.z) - 0.115)) + max(rect + 0.0, 0.0) * 10.0);
    if (e < gEm) { gEm = e; gEmId = mod(cid, 2.0) < 0.5 ? 0.0 : 4.0; }        // ribs alternate magenta / gold
    // 2. white bars on the walls between the ribs (two rows, dashed)
    float zc = q.z;
    for (int i = 0; i < 2; i++)
    {
        float yb = 0.32 + 0.48 * float(i);
        float bar = length(vec2(q.x - (W - 0.015), q.y - yb));
        bar = max(bar, abs(zc) - (CELL * 0.5 - 0.35));
        bar = max(bar, 0.32 - abs(zc));
        if (bar < gEm) { gEm = bar; gEmId = 2.0; }
    }
    // 3. golden lamps in the corners of every rib
    float lamp = length(q - vec3(W - 0.42, H - 0.36, 0.17)) - 0.06;
    if (lamp < gEm) { gEm = max(lamp, 0.0); gEmId = 1.0; }
    // 3b. golden chevrons on the walls: V struts between the ribs
    {
        vec2 v = vec2(abs(zc) - 0.55, q.y - 0.55);
        float chev = abs(v.y - abs(v.x) * 0.9 + 0.2) * 0.74;
        chev = max(length(vec2(chev, q.x - (W - 0.01))), abs(zc) - 0.95);
        if (chev < gEm) { gEm = chev; gEmId = 4.0; }
    }
    // 4. a magenta rail along the ceiling / floor centre
    float rail = length(vec2(q.x - 0.06, q.y - (H - 0.012)));
    if (rail < gEm) { gEm = rail; gEmId = 0.0; }
    // 5. a floating diamond portal every fourth cell
    if (mod(cid, 4.0) < 0.5)
    {
        float dia = abs(q.x * 0.62 + q.y - 0.95) * 0.85;
        float portal = length(vec2(dia, q.z));
        if (portal < gEm) { gEm = portal; gEmId = 3.0; }
        solid = min(solid, portal - 0.02);
    }
    return solid;
}

vec3 emitCol(float id)
{
    if (id < 0.5) return C1;
    if (id < 1.5) return C2 * 1.4 * (1.0 + 1.5 * uHat + 0.8 * uKick);
    if (id < 2.5) return mix(C3, vec3(1.0), 0.5);
    if (id > 3.5) return C2 * 1.1;
    return mix(C1, vec3(1.0), clamp(uSnare * (0.6 + 0.6 * uMacro.y), 0.0, 0.85));
}

vec3 normalAt(vec3 p)
{
    vec2 e = vec2(0.0015, 0.0);
    return normalize(vec3(map(p + e.xyy) - map(p - e.xyy), map(p + e.yxy) - map(p - e.yxy), map(p + e.yyx) - map(p - e.yyx)));
}

// marches a ray, collecting the neon glow along the way; returns the glow, the hit distance in t
vec3 march(vec3 ro, vec3 rd, int steps, float far, out float t, out bool hit, float glowK)
{
    vec3 glow = vec3(0.0);
    t = 0.02; hit = false;
    for (int i = 0; i < 140; i++)
    {
        if (i >= steps) break;
        vec3 p = ro + rd * t;
        float d = map(p);
        float h = clamp(d * 0.8, 0.008, 0.25);
        glow += emitCol(gEmId) * (glowK * 0.0009 / (0.00025 + gEm * gEm)) * h;
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
    gTwist = (0.04 + 0.25 * uMacro.x) * (0.3 * build + 1.2 * chaos) * sin(uMidTime * 0.05 + 1.0) + 0.02 * uMacro.x;

    float travel = uBassTime * (0.35 + 0.9 * uMacro.z) + uTime * (0.15 + 0.3 * uMacro.z) + build * uTime * 0.2;
    vec3 ro = vec3(0.12 * sin(uMidTime * 0.11), 0.08 * cos(uMidTime * 0.09), travel);
    vec3 rd = normalize(vec3(uv * vec2(1.0, 1.0), 1.25));
    rd.xy *= rot(0.05 * sin(uMidTime * 0.07));
    rd = roleCamera(rd);

    float glowK = mix(0.7, 1.5, uMacro.y) * (1.0 + 1.2 * uKick + 0.6 * peak) * mix(0.55, 1.0, uActivity);
    float t; bool hit;
    vec3 col = march(ro, rd, stepBudget(110), 40.0, t, hit, glowK);

    if (hit)
    {
        vec3 p = ro + rd * t;
        vec3 n = normalAt(p);
        float em = gEm; float id = gEmId;
        // glossy black lacquer reflecting the neon
        vec3 rr = reflect(rd, n);
        float t2; bool hit2;
        vec3 refl = march(p + n * 0.02, rr, stepBudget(45), 12.0, t2, hit2, glowK);
        float fres = 0.08 + 0.92 * pow(1.0 - clamp(dot(-rd, n), 0.0, 1.0), 4.0);
        col += C0 * 0.25 + refl * (0.25 + 0.75 * fres) * (0.6 + 0.6 * uHigh);
        col += emitCol(id) * 3.0 * (1.0 - smoothstep(0.0, 0.04, em));      // strips that sit on the surface
        col *= exp(-0.02 * t);
    }
    // depth haze in the strip colour and the light at the end of the hall
    col += mix(C1, C2, 0.5) * 0.15 * (1.0 - exp(-0.03 * t)) * (0.5 + peak);
    col += mix(C1, C3, 0.5) * exp(-length(uv) * 9.0) * (0.15 + 0.4 * build + 0.4 * uKick);

    col *= (1.0 + 0.3 * peak) * (1.0 - 0.15 * calm);
    col *= uIntensity * mix(0.6, 1.0, uActivity);
    fragColor = vec4(safeHDR(col), 1.0);
}
