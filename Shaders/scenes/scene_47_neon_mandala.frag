// NEON MANDALA — mirrored neon line art falling toward you, layer after layer (Photism-style symmetry).
//
// Self-driving: every part is automated from the music.
//   bass          the zoom: new mandala layers rise out of the centre with the bass
//   kick          every tube flares and the whole mandala punches outward
//   snare         the chevrons flash white
//   hi-hat        the dashed rings sparkle
//   mids          the layers turn against each other
//   highs         light runs along the dashes
//   BUILD         the symmetry multiplies (more kaleidoscope segments), the zoom tightens
//   PEAK          everything blazes, more layers at once
//   CHAOS         the symmetry jumps on every beat
//   colour        three colour families, the music moves between them
// Macros: A Symmetry · B Glow · C Zoom Speed · D Colour Family

vec3 C0, C1, C2, C3;
void setPalette()
{
    vec3 w = familyWeights(colourFamily(uMacro.w));
    // A: magenta · gold · hot orange      B: hot pink · amber · ice white      C: violet · cyan · electric blue
    vec3 a0 = hex(1310740.0),  a1 = hex(16722902.0), a2 = hex(16758062.0), a3 = hex(16738842.0);
    vec3 b0 = hex(1311748.0),  b1 = hex(16727182.0), b2 = hex(16761402.0), b3 = hex(12579071.0);
    vec3 c0 = hex(393246.0),   c1 = hex(9063935.0),  c2 = hex(3073791.0),  c3 = hex(2976767.0);
    C0 = a0 * w.x + b0 * w.y + c0 * w.z; C1 = a1 * w.x + b1 * w.y + c1 * w.z;
    C2 = a2 * w.x + b2 * w.y + c2 * w.z; C3 = a3 * w.x + b3 * w.y + c3 * w.z;
}

// one mandala layer in the folded quadrant q (x, y >= 0); seed picks its design
vec3 layer(vec2 q, float seed, float w)
{
    float h1 = hash12(vec2(seed, 1.7)), h2 = hash12(vec2(seed, 9.1)), h3 = hash12(vec2(seed, 4.3)), h4 = hash12(vec2(seed, 7.7));
    float r = length(q);
    vec3 col = vec3(0.0);

    // 1. the great X: two parallel strands from the centre outward
    float sl = 0.55 + 0.9 * h1;                                      // its angle
    vec2 dir = normalize(vec2(1.0, sl));
    vec2 nrm = vec2(-dir.y, dir.x);
    for (int i = 0; i < 2; i++)
    {
        float off = (float(i) - 0.5) * (0.012 + 0.02 * h2);
        float dx = abs(dot(q, nrm) - off);
        dx = max(dx, abs(dot(q, dir) - 0.45) - 0.42);                // a long segment, not to infinity
        col += neonTube(dx, w * 1.2, C1, 0.9) * step(0.25, h3);
    }

    // 2. long horizontal bars with hot tips (the helix-like strokes)
    int nBars = 2 + int(h4 * 2.99);
    for (int i = 0; i < 4; i++)
    {
        if (i >= nBars) break;
        float fi = float(i);
        float by = 0.06 + fi * (0.07 + 0.05 * h2) + 0.03 * h1;
        float x0 = 0.03 + 0.18 * hash12(vec2(seed, 30.0 + fi)), x1 = x0 + 0.25 + 0.45 * hash12(vec2(seed, 40.0 + fi));
        float bar = sdSeg(q, vec2(x0, by), vec2(x1, by));
        float tip = smoothstep(x1 - 0.12, x1, q.x);                   // hot coloured ends, white middles
        vec3 bc = mix(mix(C2, vec3(1.0), 0.35), C1, tip);
        col += neonTube(bar, w, bc, 0.7);
    }

    // 3. chevrons pointing at the centre along the horizontal axis
    float snareFlash = clamp(uSnare * (0.6 + 0.8 * uMacro.y), 0.0, 0.85);
    int nChev = 1 + int(h1 * 2.99);
    for (int i = 0; i < 3; i++)
    {
        if (i >= nChev) break;
        float x0 = 0.1 + float(i) * (0.06 + 0.04 * h3);
        float sp = 0.07 + 0.06 * h2;
        float dv = sdSeg(q, vec2(x0, 0.0), vec2(x0 + sp, sp * 1.3));
        col += neonTube(dv, w, mix(C2, vec3(1.0), snareFlash), 0.8);
    }

    // 4. a great arc with dashed ribs
    float hasRing = step(0.3, h2);
    float R = 0.34 + 0.2 * h1;
    float ang = atan(q.y, q.x);
    float arc = max(abs(r - R), (0.25 + 0.35 * h3) - ang);           // open around the horizontal axis
    col += neonTube(arc, w * 1.1, C3, 0.9) * hasRing;
    float dash = abs(fract(ang * (10.0 + 8.0 * h3) / PI - uHighTime * 0.6) - 0.5);
    float dashD = max(abs(r - R + 0.03), (dash - 0.2) * r * 0.32);
    dashD = max(dashD, (0.25 + 0.35 * h3) - ang);
    col += neonTube(dashD, w * 0.7, C3, 0.6) * (0.4 + 1.6 * uHat + 0.4 * uHigh) * hasRing;

    // 5. the pink spine and inner diamond
    float spine = max(abs(q.x - 0.006 - 0.01 * h4), abs(q.y - 0.2) - 0.16 - 0.1 * h1);
    col += neonTube(spine, w, C1, 1.0) * step(0.5, h4);
    float dd = abs(q.x * 0.55 + q.y - (0.05 + 0.06 * h2)) * 0.87;
    col += neonTube(dd, w * 0.9, C1, 1.2);
    return col;
}

void main()
{
    setPalette();
    vec2 uv = centered();
    float build = uState.y, peak = uState.z, chaos = uState.w, calm = uState.x;

    // the symmetry: macro A sets it, BUILD multiplies it, CHAOS jumps it on the beat
    float segs = floor(uMacro.x * 6.0 + 0.5) + floor(build * 3.0) + chaos * floor(hash12(vec2(floor(uBeatClock), 3.0)) * 5.0);
    float nSeg = segs < 1.0 ? 1.0 : segs + 1.0;

    float pulse = 1.0 - 0.07 * uKick - 0.03 * uDrop;                   // the kick punches outward
    vec2 p = uv * pulse;

    float travel = uBassTime * (0.05 + 0.12 * uMacro.z) + uTime * (0.01 + 0.03 * uMacro.z) + 0.15 * build * uTime * 0.1;
    const int K = 3;
    vec3 col = vec3(0.0);
    float w = 0.0042 * (1.0 + 0.6 * uKick + 0.3 * peak) * mix(0.8, 1.3, uMacro.y);
    for (int k = 0; k < K; k++)
    {
        float fk = float(k);
        float z = fract(travel + fk / float(K));                       // 0 = deep inside .. 1 = passing the camera
        float id = floor(travel + fk / float(K)) * float(K) + fk;
        float s = exp2(z * 3.0 - 1.4);                                  // layer scale
        float fade = smoothstep(0.0, 0.25, z) * (1.0 - smoothstep(0.7, 1.0, z));
        vec2 q = p / s;
        // mirror first (the symmetry stays on the screen axes), then turn inside it, then the kaleidoscope
        q = abs(q);
        q *= rot((0.12 * sin(fk * 2.1) + 0.06 * sin(uMidTime * 0.05 * (mod(fk, 2.0) < 0.5 ? 1.0 : -1.0))) * (0.5 + uMid));
        q = neonFold(q, nSeg);
        col += layer(q, id, w / s) * fade * (0.55 + 0.45 * z);
    }

    // the heart of the mandala and a faint backdrop glow
    float r = length(uv);
    col += mix(C1, C3, 0.4) * exp(-r * 14.0) * (0.12 + 0.5 * uKick + 0.3 * peak);
    col += C0 * 0.8 * (1.0 - smoothstep(0.0, 0.9, r));

    col *= mix(0.65, 1.0, smoothstep(0.0, 0.6, uEnergyMed + peak)) * (1.0 + 0.35 * peak) * (1.0 - 0.15 * calm);
    col *= uIntensity * mix(0.55, 1.0, uActivity);
    fragColor = vec4(safeHDR(col), 1.0);
}
