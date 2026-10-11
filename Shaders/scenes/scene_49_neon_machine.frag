// NEON SUNSET DRIVE — 3D: a sleek neon car racing over an endless glowing grid towards a striped synthwave sun
// setting behind neon mountains; the chrome body mirrors the sunset. (Identity: retro sunset — pink / orange / cyan.)
//
// Self-driving: every part is automated from the music.
//   bass          the grid races towards the camera; the camera sways and dips
//   kick          every light strip flares, the sun pulses, the wheels flash
//   snare         the tail lights strobe white
//   hi-hat        the rim lights sparkle
//   mids          the light runs along the strips
//   highs         the reflections sharpen
//   BUILD         the camera closes in, the under-glow rises, the sun climbs
//   PEAK          everything blazes
//   CHAOS         the mirror view multiplies
//   colour        three colour families, the music moves between them
// Macros: A Mirror (none / left-right / four ways) · B Glow · C Race Speed · D Colour Family

vec3 C0, C1, C2, C3;
void setPalette()
{
    vec3 w = familyWeights(colourFamily(uMacro.w));
    // A: hot pink · orange · cyan     B: red · amber · electric blue     C: teal · pink · violet
    vec3 a0 = hex(1311774.0), a1 = hex(16723592.0), a2 = hex(16747039.0), a3 = hex(3073791.0);
    vec3 b0 = hex(1049354.0), b1 = hex(16720704.0), b2 = hex(16756736.0), b3 = hex(3170559.0);
    vec3 c0 = hex(201752.0),  c1 = hex(61648.0),    c2 = hex(16732080.0), c3 = hex(10502399.0);
    C0 = a0 * w.x + b0 * w.y + c0 * w.z; C1 = a1 * w.x + b1 * w.y + c1 * w.z;
    C2 = a2 * w.x + b2 * w.y + c2 * w.z; C3 = a3 * w.x + b3 * w.y + c3 * w.z;
}

float gEm = 1e9, gEmId = 0.0, gMat = 0.0;     // gMat: 0 body, 1 floor, 2 wheel
float gScroll = 0.0, gSunLift = 0.0;

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
    if (tail < gEm) { gEm = tail; gEmId = 5.0; }
    float rim = abs(length(wq.yz) - 0.17) + max(abs(wq.x) - 0.05, 0.0);                         // wheel rims
    rim = length(vec2(rim, wq.x - 0.05));
    if (rim < gEm) { gEm = rim; gEmId = 2.0; }
    // the endless neon grid racing under the car
    {
        float gz = p.z + gScroll;
        float fl = min(abs(fract(gz * 0.5) - 0.5), abs(fract(p.x * 0.5 + 0.5) - 0.5)) * 2.0;
        float gl = length(vec2(p.y + 0.355, fl));
        if (gl < gEm) { gEm = gl; gEmId = 4.0; }
    }
    float chev = abs(q.z - 0.55 - q.x * 0.8) * 0.78;                                            // chevrons on the bonnet
    chev = length(vec2(chev, (q.y - 0.17))) + max(q.x - 0.45, 0.0);
    if (chev < gEm) { gEm = chev; gEmId = 1.0; }
    return d;
}

vec3 emitCol(float id)
{
    float run = 0.7 + 0.6 * smoothstep(0.85, 1.0, sin(uHighTime * 4.0));
    if (id > 4.5) return mix(C1 * 1.5, vec3(1.0), clamp(uSnare * (0.6 + 0.6 * uMacro.y), 0.0, 0.9));
    if (id > 3.5) return C1 * 0.9;
    if (id < 0.5) return C3 * 1.2 * run;
    if (id < 1.5) return mix(C2, vec3(1.0), clamp(0.3 + uSnare * (0.6 + 0.6 * uMacro.y), 0.0, 0.9)) * 1.3;
    if (id < 2.5) return C2 * (0.6 + 1.2 * uHat + 1.0 * uKick);
    return mix(C3, vec3(1.0), 0.4);
}

// the synthwave sky: a sunset gradient, a striped sun sinking behind neon mountains, stars above
vec3 sky(vec3 rd)
{
    float el = asin(clamp(rd.y, -1.0, 1.0));
    float az = atan(rd.x, rd.z);
    vec3 horizon = mix(C2, C1, 0.35);
    vec3 c = mix(horizon * 0.75, C1 * 0.3, smoothstep(0.0, 0.08, el));
    c = mix(c, C0 * 1.2, smoothstep(0.06, 0.3, el));
    if (el < 0.0) c = mix(horizon * 0.5, C0 * 0.8, smoothstep(0.0, -0.06, el));
    // stars
    vec2 sg = vec2(az, el) * 90.0;
    c += vec3(1.0) * step(0.993, hash12(floor(sg))) * smoothstep(0.35, 0.0, length(fract(sg) - 0.5)) * smoothstep(0.2, 0.5, el) * (0.5 + uHigh);
    // the sun
    vec2 sp = vec2(az, el - (0.06 + gSunLift));
    float R = 0.13 * (1.0 + 0.05 * uKick);
    float r = length(sp);
    vec3 sunC = mix(C1 * 1.5, mix(C2, vec3(1.0, 0.95, 0.6), 0.45) * 1.7, smoothstep(-R, R * 0.9, sp.y));
    float band = fract(sp.y * 30.0 + uTime * 0.25);
    float cut = step(band, mix(0.0, 0.6, smoothstep(R * 0.15, -R, sp.y)));
    float disc = smoothstep(R, R - 0.004, r) * (1.0 - cut);
    c += C1 * (0.2 + 0.4 * uKick) * exp(-max(r - R, 0.0) * 9.0) * smoothstep(-0.05, 0.05, el + 0.03);
    c = mix(c, sunC, disc);
    // neon mountains in front of the sun, lower in the middle so the sun shows
    float ridge = (0.012 + 0.085 * fbm(vec2(az * 2.6 + 5.0, 3.0), 4)) * mix(0.25, 1.0, smoothstep(0.08, 0.55, abs(az)));
    if (el < ridge && el > -0.01)
    {
        float wire = smoothstep(0.92, 1.0, 1.0 - abs(fract(az * 22.0) - 0.5) * 2.0) * smoothstep(-0.01, ridge, el);
        c = C0 * 0.7 + C3 * 0.25 * wire;
    }
    c += C3 * 1.4 * exp(-abs(el - ridge) * 260.0) * step(-0.01, el);
    c += horizon * 0.5 * exp(-abs(el) * 30.0);
    return c;
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
        float fade = gEmId > 3.5 && gEmId < 4.5 ? exp(-0.12 * t) : 1.0;
        glow += emitCol(gEmId) * (glowK * 0.0016 / (0.0003 + gEm * gEm)) * h * fade;
        if (d < hitEps(t)) { hit = true; break; }
        t += h;
        if (t > far) break;
    }
    return glow;
}

vec3 shade(vec3 ro, vec3 rd, float glowK, int steps)
{
    float t; bool hit;
    vec3 col = march(ro, rd, steps, 26.0, t, hit, glowK);
    if (!hit) return col + sky(rd);
    vec3 p = ro + rd * t;
    float mat = gMat, em = gEm, id = gEmId;
    vec3 n = normalAt(p);
    vec3 rr = reflect(rd, n);
    float t2; bool hit2;
    vec3 refl = march(p + n * 0.02, rr, steps / 3, 8.0, t2, hit2, glowK);
    if (!hit2) refl += sky(rr);
    float fres = 0.06 + 0.94 * pow(1.0 - clamp(dot(-rd, n), 0.0, 1.0), 4.0);
    vec3 key = normalize(vec3(0.5, 0.9, 0.4));
    float spec = pow(clamp(dot(rr, key), 0.0, 1.0), 60.0);
    if (mat > 0.5 && mat < 1.5)
    {
        // mirror floor: dark reflection, fading with distance
        col += refl * 0.35 + C0 * 0.1;
    }
    else
    {
        float rim = pow(1.0 - clamp(dot(-rd, n), 0.0, 1.0), 2.0);
        float diff = clamp(dot(n, key), 0.0, 1.0);
        col += C0 * 0.3 + C2 * 0.1 * diff + C1 * 0.3 * rim + refl * (0.2 + 0.6 * fres) + vec3(1.0, 0.9, 0.85) * spec * (0.4 + 0.6 * uHigh);
        if (mat > 1.5) col += C2 * 0.25;
    }
    float lw = 0.03 + 0.004 * t;
    col += emitCol(id) * 2.5 * (1.0 - smoothstep(0.0, lw, em)) * (id > 3.5 && id < 4.5 ? exp(-0.1 * t) : 1.0);
    if (mat > 0.5 && mat < 1.5) col = mix(col, mix(C2, C1, 0.35) * 0.35, (1.0 - exp(-0.05 * t)) * 0.7);     // haze towards the horizon
    return col;
}

void main()
{
    setPalette();
    vec2 uv = centered();
    float build = uState.y, peak = uState.z, chaos = uState.w, calm = uState.x;

    // Photism-style mirror of the filmed picture (macro A; CHAOS adds a kaleidoscope fold)
    float mirror = uMacro.x;
    if (mirror > 0.33) uv.x = abs(uv.x);
    if (mirror > 0.66) uv.y = abs(uv.y) - 0.06;
    if (chaos > 0.4) { float a = atan(uv.y, uv.x), seg = PI / 3.0; a = abs(mod(a, 2.0 * seg) - seg); uv = length(uv) * vec2(cos(a), sin(a)); }

    gScroll = uBassTime * (0.6 + 1.6 * uMacro.z) + uTime * (0.6 + 1.2 * uMacro.z) + build * uTime * 0.6;
    gSunLift = 0.03 * build;
    float sway = 0.45 * sin(uBassTime * (0.02 + 0.06 * uMacro.z) + uTime * 0.03);
    float dist = 4.3 - 0.9 * build;
    vec3 target = vec3(0.0, 0.25, 0.0);
    vec3 ro = vec3(sin(sway) * dist, 0.55 + 0.25 * sin(uBassTime * 0.03) + 0.1 * uSub, -cos(sway) * dist);
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
