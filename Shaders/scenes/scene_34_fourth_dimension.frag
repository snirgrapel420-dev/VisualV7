// FOURTH DIMENSION — flying through a 3D slice of a 4D lattice that moves through the fourth dimension.
// What you see is a cross-section: as the slice travels along w, struts merge and split, chambers
// open and close, the space continuously rebuilds itself.
//
// Self-driving: every part is automated from the music.
//   bass / sub    a 4D rotation (xw / zw planes): the lattice turns itself inside out
//   kick          a JUMP along w: the whole architecture snaps to the next slice
//   snare         a fine shudder of the slice
//   mids          slow travel along w (continuous rebuilding)
//   highs         iridescent glints on the struts
//   drop          a big leap through the fourth dimension
//   BUILD         travel along w accelerates, the light gathers ahead
//   PEAK          struts glow, iridescence at full strength
//   CHAOS         w oscillates: the space flickers between slices
//   colour        three colour families, the music moves between them
// Macros: A Lattice Scale · B Strut Thickness · C Flight Speed · D Colour Family

vec3 C0, C1, C2, C3;
void setPalette()
{
    vec3 w = familyWeights(colourFamily(uMacro.w));
    // A: void · ultramarine · cyan · white   B: black · emerald · gold · cream   C: night · crimson · violet · pink
    vec3 a0 = hex(66576.0),   a1 = hex(1056944.0),  a2 = hex(4249087.0),  a3 = hex(15792383.0);
    vec3 b0 = hex(65793.0),   b1 = hex(551502.0),   b2 = hex(14264900.0), b3 = hex(16774355.0);
    vec3 c0 = hex(131336.0),  c1 = hex(10027054.0), c2 = hex(7088842.0),  c3 = hex(16754899.0);
    C0 = a0 * w.x + b0 * w.y + c0 * w.z; C1 = a1 * w.x + b1 * w.y + c1 * w.z;
    C2 = a2 * w.x + b2 * w.y + c2 * w.z; C3 = a3 * w.x + b3 * w.y + c3 * w.z;
}

float gW;            // position along the fourth dimension
mat2 gXW, gZW;       // 4D rotations

// 4D strut lattice, sliced at w = gW: tubes run along the curves where a 4D gyroid and a 4D
// Schwarz surface intersect. As the slice moves through w the struts merge, split and re-form.
float lattice(vec3 p3)
{
    vec4 p = vec4(p3, gW);
    p.xw *= gXW;
    p.zw *= gZW;
    float sc = 2.2 + 1.2 * uMacro.x;
    p *= sc;
    float g1 = sin(p.x) * cos(p.y) + sin(p.y) * cos(p.z) + sin(p.z) * cos(p.w) + sin(p.w) * cos(p.x);
    float g2 = cos(p.x) + cos(p.y) + cos(p.z) + cos(p.w);
    float r = 0.16 + 0.18 * uMacro.y + 0.06 * uSub + 0.05 * uKick;
    return (length(vec2(g1, g2 * 0.7)) - r) / (sc * 2.2);
}

void main()
{
    setPalette();
    vec2 uv = (gl_FragCoord.xy - 0.5 * uRes) / uRes.y;
    float build = uState.y, peak = uState.z, chaos = uState.w, calm = uState.x;

    // travel through the fourth dimension
    gW = uMidTime * (0.05 + 0.15 * build) + 0.3 * uMid + 0.9 * uKick + 2.2 * uDrop + 0.08 * uSnare * sin(uHighTime * 40.0)
       + chaos * 0.35 * sin(uHighTime * 3.0);
    gXW = rot(uBassTime * 0.03);
    gZW = rot(uBassTime * 0.021 + 0.4);

    float z = uBassTime * (0.12 + 0.4 * uMacro.z) + uTime * 0.03;
    vec3 ro = vec3(0.3 * sin(z * 0.4), 0.3 * cos(z * 0.33), z);
    vec3 fw = normalize(vec3(0.25 * sin(z * 0.21), 0.2 * cos(z * 0.17), 1.0));
    vec3 rt = normalize(cross(vec3(0.0, 1.0, 0.0), fw));
    vec3 up = cross(fw, rt);
    uv *= rot(0.15 * sin(z * 0.13));
    vec3 rd = normalize(uv.x * rt + uv.y * up + 1.3 * fw);
    rd = roleCamera(rd);                       // sub / mids / bassline move the camera

    float t = 0.35, glow = 0.0;                         // near clip: never render from inside a strut
    bool hit = false;
    for (int i = 0; i < 110; i++)
    {
        if (i >= stepBudget(110)) break;   // adaptive quality
        vec3 p = ro + rd * t;
        float d = lattice(p);
        glow += exp(-abs(d) * 50.0) * 0.004;
        if (abs(d) < 0.0008 * t) { hit = true; break; }
        t += abs(d) * 0.85;
        if (t > 14.0) break;
    }

    vec3 col = C0 * 0.4;
    if (hit)
    {
        vec3 p = ro + rd * t;
        vec2 e = vec2(0.0015, 0.0);
        vec3 n = normalize(vec3(lattice(p + e.xyy) - lattice(p - e.xyy), lattice(p + e.yxy) - lattice(p - e.yxy),
                                lattice(p + e.yyx) - lattice(p - e.yyx)));
        float ao = clamp(lattice(p + n * 0.1) / 0.1, 0.0, 1.0);
        vec3 kl = normalize(fw * 0.5 + up * 0.8 + rt * 0.3);
        float key = clamp(dot(n, kl), 0.0, 1.0);
        float head = clamp(dot(n, -rd), 0.0, 1.0);
        float fre = pow(1.0 - head, 3.0);
        float spec = pow(clamp(dot(reflect(rd, n), kl), 0.0, 1.0), 50.0);
        // thin-film iridescence on the struts, shifting as the slice moves through w
        float film = dot(n, rd) * 2.0 + gW * 0.6 + t * 0.15;
        vec3 irid = 0.5 + 0.5 * cos(TAU * (film + vec3(0.0, 0.33, 0.67)));
        vec3 base = mix(C1, C2, 0.5 + 0.5 * sin(t * 0.4 + gW));
        base = mix(base, base * irid * 1.5, 0.2 + 0.3 * peak);
        col = base * (0.04 + 0.7 * key * key + 0.15 * head) * (0.2 + 0.8 * ao);
        col += C3 * spec * ao * (0.4 + 1.4 * uHigh + 0.6 * uHat);
        col += mix(C2, C3, 0.5) * fre * ao * (0.25 + 0.6 * peak + 0.5 * uKick);
        float fog = 1.0 - exp(-t * 0.22);
        col = mix(col, C0 * 0.35 + C1 * 0.08 + C2 * 0.2 * build, fog);
    }
    col += mix(C1, C2, 0.5) * glow * (0.3 + 0.9 * peak + 0.7 * uKick);
    col += C2 * build * 0.25 * exp(-length(uv) * 5.0);

    col *= 1.0 - 0.1 * calm;
    col *= smoothstep(1.5, 0.45, length(uv * vec2(0.8, 1.0)));
    col *= uIntensity * 1.4 * mix(0.6, 1.0, uActivity);
    col *= 0.6;                                   // exposure matched to the other scenes (consistency pass)
    fragColor = vec4(col, 1.0);
}
