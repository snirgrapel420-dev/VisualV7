// DIMENSION GATE — flying through a chain of portal gates; behind every gate lies another dimension
// with its own structure, space and colour (raymarched).
//
//   dimension 0  a field of crystal shards      dimension 1  a lattice of spheres
//   dimension 2  a forest of twisted columns
//
// Self-driving: every part is automated from the music.
//   bass / sub    flight speed through the dimensions
//   kick          the NEXT GATE flares, its ring pulses outward
//   snare         the structures of the current dimension jolt
//   mids          the camera banks, the structures rotate
//   highs         sparks on the gate rings
//   BUILD         acceleration toward the next gate, the gate opens wide and blinding
//   drop          a burst through the gate
//   CHAOS         two dimensions bleed into each other
// Macros: A Structure Density · B Gate Glow · C Flight Speed · D Colour Family

vec3 C0, C1, C2, C3;
vec3 famCol(int k, int which)
{
    // three dimensions, three palettes (dark, mid, light, accent)
    if (k == 0) return which == 0 ? hex(131843.0) : (which == 1 ? hex(2632795.0) : (which == 2 ? hex(4567551.0) : hex(15790335.0)));   // ice
    if (k == 1) return which == 0 ? hex(1048578.0) : (which == 1 ? hex(9049629.0) : (which == 2 ? hex(16747070.0) : hex(16775636.0)));  // ember
    return which == 0 ? hex(65792.0) : (which == 1 ? hex(1068329.0) : (which == 2 ? hex(5627006.0) : hex(14417918.0)));                 // jungle
}

const float GAP = 14.0;
float gMat;

float sdTorusXY(vec3 p, vec2 t) { return length(vec2(length(p.xy) - t.x, p.z)) - t.y; }

float dimension(vec3 p, int k)
{
    float dens = 1.0 + uMacro.x;
    float jolt = 0.2 * uSnare;
    vec3 q = p;
    q.xy *= rot(uMidTime * 0.03 + p.z * 0.02 + 0.3 * uMid);
    if (k == 0)
    {
        vec3 c = mod(q, 2.2 / dens) - 1.1 / dens;
        c.xz *= rot(1.0 + jolt); c.xy *= rot(0.6);
        vec3 a = abs(c);
        return (a.x + a.y + a.z - 0.35 / dens) * 0.57;                  // octahedral shards
    }
    if (k == 1)
    {
        vec3 c = mod(q, 1.8 / dens) - 0.9 / dens;
        return length(c) - (0.28 + 0.06 * uSub + jolt * 0.2) / dens;    // sphere lattice
    }
    vec3 c = q;
    c.xy = mod(c.xy, 1.6 / dens) - 0.8 / dens;
    c.xy *= rot(c.z * 1.2 + jolt * 3.0);
    return length(max(abs(c.xy) - vec2(0.12, 0.05) / dens, 0.0)) - 0.02;   // twisted columns
}

float map(vec3 p)
{
    float seg = floor(p.z / GAP);
    int k = int(mod(seg, 3.0));
    float zl = p.z - seg * GAP;
    // keep a tunnel of open space around the flight line
    float tunnel = 1.6 - length(p.xy);
    float d = max(dimension(p, k), tunnel);
    // CHAOS: the next dimension bleeds in
    if (uState.w > 0.05) d = mix(d, max(dimension(p, int(mod(seg + 1.0, 3.0))), tunnel), 0.5 * uState.w);
    gMat = float(k);
    // the gate at the start of every segment
    float gate = sdTorusXY(vec3(p.xy, zl - 0.5), vec2(2.2, 0.06));
    if (gate < d) { d = gate; gMat = 3.0 + float(k); }
    return d;
}

void main()
{
    vec2 uv = (gl_FragCoord.xy - 0.5 * uRes) / uRes.y;
    float build = uState.y, peak = uState.z, chaos = uState.w, calm = uState.x;

    float z = uBassTime * (0.8 + 2.2 * uMacro.z) * (1.0 + 0.8 * build) + uTime * 0.3 + 6.0 * uDrop;
    vec3 ro = vec3(0.3 * sin(z * 0.1), 0.25 * cos(z * 0.13), z);
    vec3 rd = normalize(vec3(uv, 1.3 - 0.3 * build));
    rd = roleCamera(rd);                       // sub / mids / bassline move the camera
    rd.xy *= rot(0.15 * sin(uMidTime * 0.04));

    float t = 0.05;
    bool hit = false;
    float gateGlow = 0.0;
    for (int i = 0; i < 140; i++)
    {
        if (i >= stepBudget(140)) break;   // adaptive quality
        vec3 p = ro + rd * t;
        float d = map(p);
        if (gMat >= 3.0) gateGlow += exp(-d * 8.0) * 0.02;
        if (d < hitEps(t)) { hit = true; break; }
        t += d * 0.8;
        if (t > 60.0) break;
    }
    // the dimension the camera is in sets the palette (cross-fading as a gate is passed)
    float seg = floor(ro.z / GAP), zl = ro.z - seg * GAP;
    int kNow = int(mod(seg, 3.0));
    vec3 A0 = famCol(kNow, 0), A1 = famCol(kNow, 1), A2 = famCol(kNow, 2), A3 = famCol(kNow, 3);
    vec3 col = A0 * 0.5;
    vec3 p = ro + rd * t;
    int kHit = int(mod(floor(p.z / GAP), 3.0));
    vec3 H0 = famCol(kHit, 0), H1 = famCol(kHit, 1), H2 = famCol(kHit, 2), H3 = famCol(kHit, 3);
    if (hit)
    {
        float mat = gMat;
        vec2 e = vec2(0.002, 0.0);
        vec3 n = normalize(vec3(map(p + e.xyy) - map(p - e.xyy), map(p + e.yxy) - map(p - e.yxy), map(p + e.yyx) - map(p - e.yyx)));
        float head = clamp(dot(n, -rd), 0.0, 1.0);
        float spec = pow(head, 30.0);
        if (mat >= 3.0)
        {
            // gates: lit rings; the next gate flares on the kick
            float nextGate = step(ro.z, p.z) * step(p.z, ro.z + GAP + 1.0);
            col = mix(H2, H3, 0.6) * (0.3 + 0.5 * head) * (0.25 + 0.7 * uMacro.y * (0.4 + peak) + 1.6 * uKick * nextGate);
            col += H3 * pow(0.5 + 0.5 * sin(atan(p.y, p.x) * 24.0 - uHighTime * 6.0), 12.0) * (0.3 + 1.2 * uHigh);
        }
        else
        {
            col = mix(H1, H2, 0.3 + 0.4 * head) * (0.08 + 0.6 * head * head) + H3 * spec * (0.3 + 0.8 * uHighMid);
            col += H2 * (0.05 + 0.5 * peak) * pow(1.0 - head, 3.0);
        }
        float fog = 1.0 - exp(-t * 0.06);
        col = mix(col, H0 * 0.5 + H2 * 0.08, fog);
    }
    col += mix(A2, A3, 0.5) * gateGlow * (0.3 + 0.8 * peak + 1.2 * uKick);
    // BUILD: the next gate opens wide and blinding
    col += A3 * build * 0.5 * exp(-length(uv) * (6.0 - 4.0 * build));
    // passing a gate: a flash
    col += A3 * exp(-zl * 2.5) * 0.6;

    col *= 1.0 - 0.1 * calm;
    col *= smoothstep(2.4, 0.6, length(uv * vec2(0.8, 1.0)));      // soft vignette: the frame stays full on a big screen
    col *= uIntensity * 1.35 * mix(0.6, 1.0, uActivity);
    col *= 0.85;                                   // exposure matched to the other scenes (consistency pass)
    fragColor = vec4(col, 1.0);
}
