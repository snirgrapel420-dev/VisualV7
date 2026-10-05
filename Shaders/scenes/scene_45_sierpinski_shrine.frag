// SIERPINSKI SHRINE — a Sierpinski tetrahedron, a pyramid of pyramids, lit like an ancient shrine.
//
// Self-driving: every part is automated from the music.
//   bass / sub    slow orbit; the shrine breathes (fold offset)
//   kick          the folds TWIST: every level of the pyramid turns a little and settles
//   snare         a shimmer runs down the levels
//   mids          the light swings around the shrine
//   highs         dust in the light beams through the gaps
//   BUILD         the camera rises toward the apex, light from inside grows
//   PEAK          the shrine is lit from within: every gap glows
//   CHAOS         the levels shiver out of alignment
//   colour        three colour families, the music moves between them
// Macros: A Levels · B Inner Light · C Orbit Speed · D Colour Family

vec3 C0, C1, C2, C3;
void setPalette()
{
    vec3 w = familyWeights(colourFamily(uMacro.w));
    // A: night · sandstone · gold · white   B: black · lapis · turquoise · white   C: black · terracotta · saffron · cream
    vec3 a0 = hex(197380.0),  a1 = hex(9862730.0),  a2 = hex(15248958.0), a3 = hex(16775399.0);
    vec3 b0 = hex(131586.0),  b1 = hex(2051962.0),  b2 = hex(4245717.0),  b3 = hex(16316671.0);
    vec3 c0 = hex(131586.0),  c1 = hex(11880757.0), c2 = hex(16029246.0), c3 = hex(16774604.0);
    C0 = a0 * w.x + b0 * w.y + c0 * w.z; C1 = a1 * w.x + b1 * w.y + c1 * w.z;
    C2 = a2 * w.x + b2 * w.y + c2 * w.z; C3 = a3 * w.x + b3 * w.y + c3 * w.z;
}

float gLevel;

float sier(vec3 z)
{
    float scale = 2.0;
    vec3 off = vec3(1.0) * (1.0 + 0.03 * uSub);
    float tw = 0.06 * uKick + 0.05 * uMid + 0.04 * uState.w * sin(uHighTime * 5.0);
    int n = 8 + int(uMacro.x * 3.0 + 0.5);
    gLevel = 0.0;
    float m = 1.0;
    for (int i = 0; i < 11; i++)
    {
        if (i >= n) break;
        if (z.x + z.y < 0.0) z.xy = -z.yx;
        if (z.x + z.z < 0.0) z.xz = -z.zx;
        if (z.y + z.z < 0.0) z.zy = -z.yz;
        z.xy *= rot(tw);
        z = z * scale - off * (scale - 1.0);
        m *= scale;
        gLevel = max(gLevel, step(1.5, length(z)) * float(i));
    }
    return (length(z) - 1.2) / m;
}

void main()
{
    setPalette();
    vec2 uv = (gl_FragCoord.xy - 0.5 * uRes) / uRes.y;
    float build = uState.y, peak = uState.z, chaos = uState.w, calm = uState.x;

    float orbit = uBassTime * (0.04 + 0.12 * uMacro.z) + uTime * 0.02;
    float dist = 4.3 - 0.5 * build + 0.3 * calm;
    vec3 ro = dist * normalize(vec3(sin(orbit), 0.55 + 0.4 * build, cos(orbit)));
    vec3 ta = vec3(0.0, 0.1 + 0.25 * build, 0.0);
    vec3 fw = normalize(ta - ro);
    vec3 rt = normalize(cross(vec3(0.0, 1.0, 0.0), fw));
    vec3 up = cross(fw, rt);
    vec3 rd = normalize(uv.x * rt + uv.y * up + 1.7 * fw);
    rd = roleCamera(rd);                       // sub / mids / bassline move the camera
    // the tetrahedron stands on a face: rotate the world so an apex points up
    // world -> fractal space: the fractal's (1,1,1) vertex points straight up
    vec3 ax = normalize(vec3(1.0, 1.0, 1.0)), bx = normalize(cross(ax, vec3(0.0, 0.0, 1.0))), cx = cross(ax, bx);
    mat3 R = mat3(bx, ax, cx);

    float t = 0.0, glow = 0.0;
    bool hit = false;
    for (int i = 0; i < 160; i++)
    {
        if (i >= stepBudget(160)) break;   // adaptive quality
        vec3 p = R * (ro + rd * t);
        float d = sier(p);
        glow += exp(-d * 40.0) * 0.004;
        if (d < hitEps(t) * 0.7) { hit = true; break; }
        t += d * 0.9;
        if (t > 12.0) break;
    }

    vec3 col = mix(C0 * 0.3, C1 * 0.1, 0.5 + 0.5 * uv.y);
    if (hit)
    {
        float lev = gLevel;
        vec3 pw = ro + rd * t;
        vec2 e = vec2(0.0006 * t + 0.0002, 0.0);
        vec3 n = normalize(vec3(sier(R * (pw + e.xyy)) - sier(R * (pw - e.xyy)), sier(R * (pw + e.yxy)) - sier(R * (pw - e.yxy)), sier(R * (pw + e.yyx)) - sier(R * (pw - e.yyx))));
        float ao = clamp(sier(R * (pw + n * 0.03)) / 0.03, 0.0, 1.0);
        vec3 ld = normalize(vec3(sin(uMidTime * 0.05), 0.9, cos(uMidTime * 0.05)));    // the light swings around
        float key = clamp(dot(n, ld), 0.0, 1.0);
        float head = clamp(dot(n, -rd), 0.0, 1.0);
        vec3 stone = ramp4(0.3 + 0.05 * lev, C0, C1, C2, C3);
        col = stone * (0.05 + 0.7 * key + 0.2 * head) * (0.2 + 0.8 * ao);
        // inner light through the gaps: PEAK and BUILD; snare shimmer down the levels
        float shimmer = exp(-abs(lev - (1.0 - uSnare) * 10.0) * 0.5) * uSnare * 2.0;
        col += mix(C2, C3, 0.4) * (1.0 - ao) * (0.06 + 0.9 * peak * (0.3 + uMacro.y) + 0.5 * build + 0.6 * uKick + shimmer);
    }
    col += mix(C1, C2, 0.5) * glow * (0.3 + 0.8 * peak + 0.6 * uKick);
    // dust in the beams
    vec2 sg = uv * 45.0 + vec2(uMidTime * 0.2, uHighTime);
    col += C3 * step(0.994 - 0.005 * uHigh, hash12(floor(sg))) * smoothstep(0.25, 0.0, length(fract(sg) - 0.5)) * (0.2 + 0.7 * uHigh);

    col *= 1.0 - 0.1 * calm;
    col *= smoothstep(2.4, 0.6, length(uv * vec2(0.8, 1.0)));      // soft vignette: the frame stays full on a big screen
    col *= uIntensity * 1.45 * mix(0.6, 1.0, uActivity);
    col *= 1.3;                                   // exposure matched to the other scenes (consistency pass)
    fragColor = vec4(col, 1.0);
}
