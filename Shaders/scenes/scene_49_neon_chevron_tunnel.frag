// NEON CHEVRON TUNNEL — flying down a tunnel of neon diamonds and chevrons, mirrored on both axes.
//
// Self-driving: every part is automated from the music.
//   bass          the flight speed: frames rush toward you with the bass
//   kick          the nearest frames flare, the rails pulse outward
//   snare         the diamonds flash white
//   hi-hat        the side brackets flicker
//   mids          the frames breathe between diamond and square
//   highs         light runs along the vanishing rails
//   BUILD         the tunnel accelerates and the frames crowd together
//   PEAK          the rails blaze, the colours saturate
//   CHAOS         the frames twist out of line
//   colour        three colour families, the music moves between them
// Macros: A Frame Shape · B Glow · C Flight Speed · D Colour Family

vec3 C0, C1, C2, C3;
void setPalette()
{
    vec3 w = familyWeights(colourFamily(uMacro.w));
    // A: magenta diamonds · gold chevrons · white rails     B: hot pink · amber · ice      C: violet · cyan · blue
    vec3 a0 = hex(1310740.0),  a1 = hex(16722902.0), a2 = hex(16758062.0), a3 = hex(16772567.0);
    vec3 b0 = hex(1311748.0),  b1 = hex(16727182.0), b2 = hex(16761402.0), b3 = hex(12579071.0);
    vec3 c0 = hex(393246.0),   c1 = hex(9063935.0),  c2 = hex(3073791.0),  c3 = hex(2976767.0);
    C0 = a0 * w.x + b0 * w.y + c0 * w.z; C1 = a1 * w.x + b1 * w.y + c1 * w.z;
    C2 = a2 * w.x + b2 * w.y + c2 * w.z; C3 = a3 * w.x + b3 * w.y + c3 * w.z;
}

void main()
{
    setPalette();
    vec2 uv = centered();
    float build = uState.y, peak = uState.z, chaos = uState.w, calm = uState.x;
    vec2 p = abs(uv);
    float r = length(uv);
    vec3 col = vec3(0.0);
    float wBase = 0.0045 * (1.0 + 0.5 * uKick + 0.3 * peak) * mix(0.8, 1.35, uMacro.y);

    // ---- the vanishing rails: lines from the centre to the corners and the sides ---------------
    float runner = 0.6 + 0.6 * smoothstep(0.8, 1.0, sin(r * 30.0 - uHighTime * 6.0));
    for (int i = 0; i < 3; i++)
    {
        float a = 0.32 + 0.3 * float(i) + 0.05 * uMid;               // rail angles
        vec2 dir = vec2(cos(a), sin(a));
        float d = abs(dot(p, vec2(-dir.y, dir.x)));
        float grow = smoothstep(0.04, 0.5, r);                       // rails brighten toward the edge
        col += neonTube(d, wBase * (0.5 + 1.2 * r), mix(C3, C2, float(i) * 0.5), 0.6) * grow
             * runner * (0.35 + 0.4 * peak + 0.6 * uKick);
    }

    // ---- frames rushing toward the camera -------------------------------------------------------
    float travel = uBassTime * (0.12 + 0.35 * uMacro.z) + uTime * (0.04 + 0.08 * uMacro.z) + build * uTime * 0.05;
    const int K = 7;
    float shape = clamp(uMacro.x + 0.25 * sin(uMidTime * 0.3) * uMidMed, 0.0, 1.0);    // 0 diamond .. 1 square-ish
    for (int k = 0; k < K; k++)
    {
        float fk = float(k);
        float z = fract(travel / float(K) * 2.0 + fk / float(K));    // 0 far .. 1 at the camera
        float id = floor(travel / float(K) * 2.0 + fk / float(K)) * float(K) + fk;
        float s = 0.03 / max(1.0 - z, 0.03);                          // perspective size
        float fade = smoothstep(0.0, 0.2, z) * (1.0 - smoothstep(0.85, 1.0, z));
        float w = wBase * (0.6 + 2.2 * z);
        vec2 q = p / s;
        q *= rot(chaos * 0.4 * (hash12(vec2(id, 2.0)) - 0.5));
        q = abs(q);
        float h = hash12(vec2(id, 11.0));

        // the diamond (morphing toward a square with macro A / the mids)
        float dia = mix(q.x * 0.62 + q.y, max(q.x * 0.75, q.y) * 1.25, shape) - 1.0;
        float dd = abs(dia) * s;
        vec3 dc = mix(C1, vec3(1.0), clamp(uSnare * (0.5 + 0.7 * uMacro.y), 0.0, 0.8));
        col += neonTube(dd, w, dc, 1.0) * fade * step(0.25, h);

        // chevrons above and below (V shapes opening outward)
        float vy = 1.25 + 0.3 * h;
        float v = sdSeg(q, vec2(0.0, vy), vec2(0.8, vy + 0.55)) * s;
        col += neonTube(v, w, C2, 0.9) * fade;
        float v2 = sdSeg(q, vec2(0.0, vy + 0.35), vec2(0.8, vy + 0.9)) * s;
        col += neonTube(v2, w * 0.8, C2, 0.7) * fade * step(0.5, h);

        // side brackets < > (flicker with the hi-hat)
        float bx = 1.6 + 0.4 * h;
        float br = min(sdSeg(q, vec2(bx, 0.0), vec2(bx + 0.45, 0.5)), sdSeg(q, vec2(bx + 0.45, 0.5), vec2(bx + 1.4, 0.5))) * s;
        col += neonTube(br, w, mix(C3, C1, 0.3), 0.8) * fade * (0.5 + 1.2 * uHat + 0.3 * uHigh);
    }

    // the light at the end of the tunnel
    col += mix(C1, C3, 0.5) * exp(-r * 16.0) * (0.15 + 0.45 * uKick + 0.4 * build);
    col += C0 * (1.0 - smoothstep(0.0, 1.0, r));

    col *= mix(0.65, 1.0, smoothstep(0.0, 0.6, uEnergyMed + peak)) * (1.0 + 0.3 * peak) * (1.0 - 0.15 * calm);
    col *= uIntensity * mix(0.55, 1.0, uActivity);
    fragColor = vec4(safeHDR(col), 1.0);
}
