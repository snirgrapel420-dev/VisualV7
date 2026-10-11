// NEON LIQUID — mirrored neon strokes melting like hot glass, with a chromatic split.
//
// Self-driving: every part is automated from the music.
//   bass          the slow swell of the liquid
//   kick          a shock wave runs through the glass, the tubes flare
//   snare         the chevrons flash white
//   hi-hat        the flames at the sides flicker
//   mids          the flow of the melt
//   highs         the chromatic split widens
//   BUILD         the neon melts further and drips downward
//   PEAK          everything blazes, the strokes double
//   CHAOS         the liquid boils
//   colour        three colour families, the music moves between them
// Macros: A Melt · B Glow · C Flow Speed · D Colour Family

vec3 C0, C1, C2, C3;
void setPalette()
{
    vec3 w = familyWeights(colourFamily(uMacro.w));
    // A: magenta · white-gold · orange      B: hot pink · amber · ice white      C: violet · cyan · blue
    vec3 a0 = hex(1310740.0),  a1 = hex(16722902.0), a2 = hex(16769214.0), a3 = hex(16738842.0);
    vec3 b0 = hex(1311748.0),  b1 = hex(16727182.0), b2 = hex(16761402.0), b3 = hex(12579071.0);
    vec3 c0 = hex(393246.0),   c1 = hex(9063935.0),  c2 = hex(3073791.0),  c3 = hex(2976767.0);
    C0 = a0 * w.x + b0 * w.y + c0 * w.z; C1 = a1 * w.x + b1 * w.y + c1 * w.z;
    C2 = a2 * w.x + b2 * w.y + c2 * w.z; C3 = a3 * w.x + b3 * w.y + c3 * w.z;
}


// the neon design, mirrored on both axes, in liquid-warped coordinates
vec3 design(vec2 p, float w)
{
    vec2 q = abs(p);
    vec3 col = vec3(0.0);
    float dsg = floor(uLevelTime * 0.02);                            // the design changes every few phrases
    // stacked strokes
    for (int i = 0; i < 5; i++)
    {
        float fi = float(i);
        float y = 0.05 + fi * 0.092;
        float x0 = 0.02 + 0.12 * hash12(vec2(fi, dsg)), x1 = x0 + 0.2 + 0.28 * hash12(vec2(fi, dsg + 7.0));
        float d = sdSeg(q, vec2(x0, y), vec2(x1, y));
        vec3 c = mix(mix(C2, vec3(1.0), 0.4), C1, smoothstep(x1 - 0.1, x1, q.x));
        col += neonTube(d, w, c, 0.9);
    }
    // chevrons on the vertical axis
    float flash = clamp(uSnare * (0.5 + 0.7 * uMacro.y), 0.0, 0.8);
    for (int i = 0; i < 2; i++)
    {
        float y0 = 0.12 + float(i) * 0.24;
        float d = sdSeg(q, vec2(0.0, y0), vec2(0.07, y0 + 0.06));
        col += neonTube(d, w, mix(C1, vec3(1.0), flash), 1.0);
    }
    // flames at the sides
    float fx = 0.58 + 0.03 * sin(q.y * 12.0 + uMidTime * 2.0);
    float fl = max(abs(q.x - fx), q.y - 0.45);
    col += neonTube(fl, w * 1.4, C1, 1.2) * (0.6 + 1.2 * uHat);
    return col;
}

void main()
{
    setPalette();
    vec2 uv = centered();
    float build = uState.y, peak = uState.z, chaos = uState.w, calm = uState.x;

    // the liquid: a flowing domain warp, melting downward with BUILD / macro A, shocked by the kick
    float t = uMidTime * (0.08 + 0.2 * uMacro.z) + uTime * 0.02;
    float melt = 0.25 + 0.75 * uMacro.x + 0.8 * build + 1.2 * chaos;
    vec2 n = vec2(fbm(uv * 3.0 + vec2(0.0, t), 4), fbm(uv * 3.0 + vec2(5.2, -t), 4)) - 0.5;
    vec2 p = uv + n * (0.03 + 0.04 * melt) * (1.0 + 0.6 * uBassSlow);
    float drip = fbm(vec2(uv.x * 6.0, t * 0.5), 3);
    p.y += melt * 0.05 * drip * smoothstep(0.3, -0.5, uv.y);              // the lower half drips
    float shock = uKick * 0.02 * sin(length(uv) * 30.0 - uBeatPhase * 12.0);
    p += normalize(uv + 1e-4) * shock;

    float w = 0.0065 * (1.0 + 0.5 * uKick + 0.3 * peak) * mix(0.8, 1.35, uMacro.y);
    float ca = 0.004 + 0.012 * uHigh + 0.006 * peak;                      // the chromatic split
    vec3 col;
    col.r = design(p * (1.0 + ca), w).r;
    col.g = design(p, w).g;
    col.b = design(p * (1.0 - ca), w).b;
    if (peak > 0.5) col += design(p * 1.18, w * 0.8) * 0.2 * (peak - 0.5) * 2.0;    // PEAK: a second echo

    col += C0 * (1.0 - smoothstep(0.0, 1.0, length(uv)));
    col *= mix(0.65, 1.0, smoothstep(0.0, 0.6, uEnergyMed + peak)) * (1.0 + 0.1 * peak) * (1.0 - 0.15 * calm);
    col *= uIntensity * mix(0.55, 1.0, uActivity);
    fragColor = vec4(safeHDR(col), 1.0);
}
