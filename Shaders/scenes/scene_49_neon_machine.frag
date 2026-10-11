// NEON MACHINE — 3D: a sleek neon-lit machine on a mirror floor, filmed and mirrored like a kaleidoscope.
//
// Self-driving: every part is automated from the music.
//   bass          the camera orbits and dips with the bass
//   kick          every light strip flares, the wheels flash
//   snare         the headlights strobe white
//   hi-hat        the rim lights sparkle
//   mids          the light runs along the strips
//   highs         the reflections sharpen
//   BUILD         the camera closes in, the under-glow rises
//   PEAK          everything blazes
//   CHAOS         the mirror view multiplies
//   colour        three colour families, the music moves between them
// Macros: A Mirror (none / left-right / four ways) · B Glow · C Orbit Speed · D Colour Family

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

float gEm = 1e9, gEmId = 0.0, gMat = 0.0;     // gMat: 0 body, 1 floor, 2 wheel

float sdRoundBox(vec3 p, vec3 b, float r) { vec3 q = abs(p) - b + r; return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0) - r; }
float sdEllipsoid(vec3 p, vec3 r) { float k0 = length(p / r); float k1 = length(p / (r * r)); return k0 * (k0 - 1.0) / k1; }
float sdSeg3(vec3 p, vec3 a, vec3 b) { vec3 pa = p - a, ba = b - a; return length(pa - ba * clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0)); }

float map(vec3 p)
{
    vec3 q = vec3(abs(p.x), p.y, p.z);                       // the machine is symmetric
    // body: a low wedge with a teardrop cabin
    float body = sdRoundBox(q - vec3(0.0, 0.0, 0.0), vec3(0.62, 0.15, 1.35), 0.12);
    body = max(body, dot(q - vec3(0.0, 0.14, 1.0), normalize(vec3(0.0, 1.0, 0.55))));          // the nose slopes down
    float cabin = sdEllipsoid(q - vec3(0.0, 0.16, -0.15), vec3(0.46, 0.22, 0.75));
    float d = smin(body, cabin, 0.12);
    // wheel arches cut in, glowing rims inside
    vec3 wq = q - vec3(0.6, -0.12, 0.0); wq.z = abs(wq.z) - 0.82;
    float wheel = length(vec2(length(wq.yz) - 0.17, wq.x)) - 0.05;
    d = max(d, -(length(wq.yz) - 0.27));
    gMat = 0.0;
    if (wheel < d) { d = wheel; gMat = 2.0; }
    // mirror floor
    float floorD = p.y + 0.36;
    if (floorD < d) { d = floorD; gMat = 1.0; }

    // ---- emitters ----
    gEm = 1e9;
    float side = sdSeg3(q, vec3(0.67, 0.02, -1.15), vec3(0.67, 0.02, 1.1));                    // side strips
    if (side < gEm) { gEm = side; gEmId = 0.0; }
    float roof = sdSeg3(q, vec3(0.3, 0.33, -0.7), vec3(0.18, 0.37, 0.35));                      // roof lines
    if (roof < gEm) { gEm = roof; gEmId = 3.0; }
    float head = sdSeg3(q, vec3(0.2, 0.04, 1.38), vec3(0.5, 0.06, 1.25));                       // headlights
    if (head < gEm) { gEm = head; gEmId = 1.0; }
    float tail = sdSeg3(q, vec3(0.0, 0.1, -1.4), vec3(0.58, 0.08, -1.36));                      // tail bar
    if (tail < gEm) { gEm = tail; gEmId = 0.0; }
    float rim = abs(length(wq.yz) - 0.17) + max(abs(wq.x) - 0.05, 0.0);                         // wheel rims
    rim = length(vec2(rim, wq.x - 0.05));
    if (rim < gEm) { gEm = rim; gEmId = 2.0; }
    // a ring of light pillars around the stage (they glow and mirror in the car and the floor)
    {
        float ang = atan(p.z, p.x), seg = TAU / 14.0;
        float a = mod(ang + seg * 0.5, seg) - seg * 0.5;
        float r = length(p.xz);
        vec2 pq = vec2(r * cos(a) - 6.5, r * sin(a));
        float pil = length(pq) + max(abs(p.y - 0.9) - 1.25, 0.0);
        if (pil < gEm) { gEm = pil; gEmId = mod(floor((ang + seg * 0.5) / seg), 2.0) < 0.5 ? 0.0 : 2.0; }
        d = min(d, pil - 0.025);
    }
    // neon lines in the mirror floor
    {
        float fl = abs(fract(p.z * 0.5 + 0.5) - 0.5) * 2.0;
        float gl = length(vec2(p.y + 0.355, min(fl, abs(fract(p.x * 0.5 + 0.5) - 0.5) * 2.0)));
        gl = max(gl, length(p.xz) - 6.2);
        if (gl < gEm) { gEm = gl; gEmId = 0.0; }
    }
    float chev = abs(q.z - 0.55 - q.x * 0.8) * 0.78;                                            // chevrons on the bonnet
    chev = length(vec2(chev, (q.y - 0.17))) + max(q.x - 0.45, 0.0);
    if (chev < gEm) { gEm = chev; gEmId = 1.0; }
    return d;
}

vec3 emitCol(float id)
{
    float run = 0.7 + 0.6 * smoothstep(0.85, 1.0, sin(uHighTime * 4.0));
    if (id < 0.5) return C1 * 1.2 * run;
    if (id < 1.5) return mix(C2, vec3(1.0), clamp(0.3 + uSnare * (0.6 + 0.6 * uMacro.y), 0.0, 0.9)) * 1.3;
    if (id < 2.5) return C2 * (0.6 + 1.2 * uHat + 1.0 * uKick);
    return mix(C3, vec3(1.0), 0.4);
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
    for (int i = 0; i < 150; i++)
    {
        if (i >= steps) break;
        vec3 p = ro + rd * t;
        float d = map(p);
        float h = clamp(d * 0.85, 0.006, 0.4);
        glow += emitCol(gEmId) * (glowK * 0.0016 / (0.0003 + gEm * gEm)) * h;
        if (d < hitEps(t)) { hit = true; break; }
        t += h;
        if (t > far) break;
    }
    return glow;
}

vec3 shade(vec3 ro, vec3 rd, float glowK, int steps)
{
    float t; bool hit;
    vec3 col = march(ro, rd, steps, 22.0, t, hit, glowK);
    if (!hit) return col + C0 * 0.5;
    vec3 p = ro + rd * t;
    float mat = gMat, em = gEm, id = gEmId;
    vec3 n = normalAt(p);
    vec3 rr = reflect(rd, n);
    float t2; bool hit2;
    vec3 refl = march(p + n * 0.02, rr, steps / 3, 8.0, t2, hit2, glowK);
    float fres = 0.06 + 0.94 * pow(1.0 - clamp(dot(-rd, n), 0.0, 1.0), 4.0);
    vec3 key = normalize(vec3(0.5, 0.9, 0.4));
    float spec = pow(clamp(dot(rr, key), 0.0, 1.0), 60.0);
    if (mat > 0.5 && mat < 1.5)
    {
        // mirror floor: dark reflection, fading with distance
        col += refl * 0.7 * exp(-0.12 * t) + C0 * 0.15;
    }
    else
    {
        float rim = pow(1.0 - clamp(dot(-rd, n), 0.0, 1.0), 2.0);
        float diff = clamp(dot(n, key), 0.0, 1.0);
        col += C0 * 0.3 + C2 * 0.12 * diff + C1 * 0.5 * rim + refl * (0.35 + 0.8 * fres) + vec3(1.0, 0.9, 0.85) * spec * (0.4 + 0.6 * uHigh);
        if (mat > 1.5) col += C2 * 0.25;
    }
    col += emitCol(id) * 2.5 * (1.0 - smoothstep(0.0, 0.03, em));
    return col;
}

void main()
{
    setPalette();
    vec2 uv = centered();
    float build = uState.y, peak = uState.z, chaos = uState.w, calm = uState.x;

    // Photism-style mirror of the filmed picture (macro A; CHAOS adds a kaleidoscope fold)
    float mirror = uMacro.x;
    if (mirror > 0.33) uv.x = abs(uv.x) - (0.08 + 0.05 * sin(uMidTime * 0.1));
    if (mirror > 0.66) uv.y = abs(uv.y) - 0.06;
    if (chaos > 0.4) { float a = atan(uv.y, uv.x), seg = PI / 3.0; a = abs(mod(a, 2.0 * seg) - seg); uv = length(uv) * vec2(cos(a), sin(a)); }

    float orbit = uBassTime * (0.04 + 0.12 * uMacro.z) + uTime * (0.02 + 0.04 * uMacro.z);
    float dist = 4.3 - 0.9 * build;
    vec3 target = vec3(0.0, 0.0, 0.0);
    vec3 ro = vec3(sin(orbit + 0.8) * dist, 0.55 + 0.3 * sin(uBassTime * 0.03) + 0.1 * uSub, cos(orbit + 0.8) * dist);
    vec3 fw = normalize(target - ro);
    vec3 rt = normalize(cross(vec3(0.0, 1.0, 0.0), fw));
    vec3 up = cross(fw, rt);
    vec3 rd = normalize(uv.x * rt + uv.y * up + 1.55 * fw);
    rd = roleCamera(rd);

    float glowK = mix(0.6, 1.4, uMacro.y) * (1.0 + 1.1 * uKick + 0.5 * peak) * mix(0.55, 1.0, uActivity);
    vec3 col = shade(ro, rd, glowK, stepBudget(110));
    // under-glow on BUILD
    col += C1 * 0.25 * build * exp(-abs(uv.y + 0.18) * 8.0);

    col *= (1.0 + 0.3 * peak) * (1.0 - 0.15 * calm);
    col *= uIntensity * mix(0.6, 1.0, uActivity);
    fragColor = vec4(safeHDR(col), 1.0);
}
