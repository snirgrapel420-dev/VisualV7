// NEON HELIX — a tower of neon bars twisting around an invisible axis, seen from the front.
//
// Self-driving: every part is automated from the music.
//   bass          the tower scrolls past the camera with the bass
//   kick          every bar shoots out wider and flares
//   snare         every second bar flashes white
//   hi-hat        sparks at the tips of the bars
//   mids          the twist turns; the bars ripple like melting neon
//   highs         light runs along the bars
//   BUILD         the twist winds up tighter and faster
//   PEAK          a second strand joins (a double helix), everything blazes
//   CHAOS         the twist jumps on every beat
//   colour        three colour families, the music moves between them
// Macros: A Strands · B Glow · C Scroll Speed · D Colour Family

vec3 C0, C1, C2, C3;
void setPalette()
{
    vec3 w = familyWeights(colourFamily(uMacro.w));
    // A: magenta tips · white-gold bars · orange       B: hot pink · amber · ice white       C: violet · cyan · blue
    vec3 a0 = hex(1310740.0),  a1 = hex(16722902.0), a2 = hex(16769214.0), a3 = hex(16738842.0);
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

    // melting neon: the mids ripple the picture, the bassline adds a slow wobble
    vec2 p = abs(uv);                          // mirrored both ways (Photism-style): the bars stream out of the middle
    p.y += (0.006 + 0.025 * uMid + 0.01 * uBassNote) * sin(p.x * 9.0 + uMidTime * 1.3) * smoothstep(0.0, 0.5, abs(p.x));
    p.x += 0.006 * sin(p.y * 14.0 + uMidTime * 0.9) * uMidMed;

    const float D = 2.2;                       // camera distance to the axis
    const float F = 1.15;                      // focal length
    float spacing = 0.13;
    float scroll = uBassTime * (0.08 + 0.25 * uMacro.z) + uTime * (0.03 + 0.06 * uMacro.z);
    float twist = 0.55 + 1.2 * build + chaos * 1.5 * (hash12(vec2(floor(uBeatClock), 5.0)) - 0.5);
    float spin = uMidTime * (0.15 + 0.2 * build) + 0.4 * uSnare;
    float L = (1.05 + 0.25 * uKick + 0.08 * uDrop) * (1.0 + 0.08 * peak);
    float strands = clamp(1.0 + floor(uMacro.x * 1.99) + step(0.6, peak), 1.0, 3.0);
    float w0 = 0.0085 * (1.0 + 0.5 * uKick + 0.3 * peak) * mix(0.8, 1.35, uMacro.y);

    vec3 col = vec3(0.0);
    float frac0 = fract(scroll);
    float base = floor(scroll);
    for (int i = -1; i <= 6; i++)
    {
        float fi = float(i);
        float y = (fi + 0.5 - frac0) * spacing;                        // world height of this level
        float level = base + fi;
        for (int s = 0; s < 3; s++)
        {
            if (float(s) >= strands) break;
            float th = level * twist + spin + float(s) * PI / strands;
            vec3 a3 = vec3( L * cos(th), y,  L * sin(th));
            vec3 b3 = vec3(-L * cos(th), y, -L * sin(th));
            vec2 a = a3.xy * F / (D + a3.z), b = b3.xy * F / (D + b3.z);
            // distance and position along the bar
            vec2 ba = b - a;
            float h = clamp(dot(p - a, ba) / max(dot(ba, ba), 1e-6), 0.0, 1.0);
            float d = length(p - a - ba * h);
            float depth = mix(a3.z, b3.z, h);                            // near (-L) .. far (+L)
            float persp = F / (D + depth);
            float edge = abs(h - 0.5) * 2.0;                             // 0 middle .. 1 tips
            vec3 bc = mix(mix(C2, vec3(1.0), 0.45), C1, smoothstep(0.45, 1.0, edge));
            bc = mix(bc, C3, 0.35 * float(s));
            float flash = uSnare * step(0.5, mod(level, 2.0)) * (0.5 + 0.6 * uMacro.y);
            bc = mix(bc, vec3(1.0), clamp(flash, 0.0, 0.8));
            float runner = 0.7 + 0.5 * smoothstep(0.85, 1.0, sin(h * 18.0 - uHighTime * 5.0 + level));
            float lit = mix(0.35, 1.15, smoothstep(L, -L, depth)) * runner;      // the far side is dimmer
            col += neonTube(d, w0 * persp, bc, 0.9) * lit;
            // sparks at the tips on the hi-hat
            float tipD = min(length(p - a), length(p - b));
            col += C1 * exp(-tipD / (w0 * 3.0)) * (0.6 * uHat + 0.15 * uHigh);
        }
    }

    // the axis glow and a deep backdrop
    col += mix(C1, C2, 0.5) * exp(-abs(uv.x) * 18.0) * (0.06 + 0.25 * uKick + 0.2 * peak);
    col += C0 * (1.0 - smoothstep(0.0, 1.0, length(uv)));

    col *= mix(0.65, 1.0, smoothstep(0.0, 0.6, uEnergyMed + peak)) * (1.0 + 0.3 * peak) * (1.0 - 0.15 * calm);
    col *= uIntensity * mix(0.55, 1.0, uActivity);
    fragColor = vec4(safeHDR(col), 1.0);
}
