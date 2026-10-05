// CRYSTAL SANCTUM — flying low through a forest of giant crystals (raymarched).
//
// Self-driving: every part is automated from the music.
//   bass / sub    flight speed, the crystals hum (inner light breathes)
//   kick          a WAVE OF LIGHT passes through the crystals, front to back
//   snare         facets flash
//   mids          the camera weaves between the spires
//   highs         sparkles on the facet edges
//   BUILD         the crystals grow taller, the light withdraws into them
//   PEAK          the inner light blazes, prismatic edges
//   CHAOS         crystals shiver, light splinters
//   colour        three colour families, the music moves between them
// Macros: A Crystal Density · B Inner Light · C Flight Speed · D Colour Family

vec3 C0, C1, C2, C3;
void setPalette()
{
    vec3 w = familyWeights(colourFamily(uMacro.w));
    // A: night · amethyst · lilac · white   B: deep sea · aquamarine · ice · white   C: black · ruby · rose quartz · white
    vec3 a0 = hex(131339.0),  a1 = hex(6560931.0),  a2 = hex(13410022.0), a3 = hex(16777215.0);
    vec3 b0 = hex(132625.0),  b1 = hex(2665386.0),  b2 = hex(12514815.0), b3 = hex(16777215.0);
    vec3 c0 = hex(131586.0),  c1 = hex(10293818.0), c2 = hex(15779277.0), c3 = hex(16777215.0);
    C0 = a0 * w.x + b0 * w.y + c0 * w.z; C1 = a1 * w.x + b1 * w.y + c1 * w.z;
    C2 = a2 * w.x + b2 * w.y + c2 * w.z; C3 = a3 * w.x + b3 * w.y + c3 * w.z;
}

float gCell;

// hexagonal prism with a pointed tip (a crystal)
float crystal(vec3 p, float h, float r)
{
    vec3 q = abs(p);
    float hex = max(q.x * 0.866 + q.z * 0.5, q.z) - r;
    float body = max(hex, p.y - h);
    float tip = max(hex + (p.y - h) * 0.9, h - p.y);                 // the point exists only above the body
    return max(min(body, tip), -p.y - 0.2);
}

// a cathedral of crystals: a ring-cluster around the centre, tallest in the middle
float map(vec3 p)
{
    float ground = p.y + 0.15 * vnoise(p.xz * 0.6);
    float d = ground;
    gCell = 0.0;
    int count = 9 + int(uMacro.x * 8.0);
    for (int i = 0; i < 17; i++)
    {
        if (i >= count) break;
        float fi = float(i);
        vec2 h = hash22(vec2(fi, 7.0));
        float ring = i == 0 ? 0.0 : (i < 7 ? 1.3 : 2.6);
        float ang = fi * 2.399 + h.x;                                        // golden-angle spread
        vec3 q = p - vec3(ring * cos(ang), 0.0, ring * sin(ang));
        // cheap bounding cylinder first: a crystal farther away than the current best is skipped
        if (length(q.xz) - 1.0 > d) continue;
        q.xz *= rot(h.y * 6.28 + uState.w * 0.08 * sin(uHighTime * 9.0 + fi));
        q.xy *= rot((h.x - 0.5) * 0.6 * min(ring, 1.0));                      // outer crystals lean outward
        float height = (i == 0 ? 4.2 : (i < 7 ? 2.6 : 1.4)) * (0.8 + 0.4 * h.y) * (1.0 + 0.35 * uState.y) * (1.0 + 0.12 * uMid * h.y);   // mids grow the crystals
        float c = crystal(q, height, i == 0 ? 0.55 : 0.28 + 0.2 * h.x);
        if (c < d) { d = c; gCell = hash12(vec2(fi, 3.0)); }
    }
    return d;
}

void main()
{
    setPalette();
    vec2 uv = (gl_FragCoord.xy - 0.5 * uRes) / uRes.y;
    float build = uState.y, peak = uState.z, chaos = uState.w, calm = uState.x;

    // the camera orbits the sanctum; bass carries it around, BUILD rises above the spires
    float orbit = uBassTime * (0.05 + 0.2 * uMacro.z) + uTime * 0.02;
    float radius = 7.0 - 0.8 * uState.z + 0.4 * uState.x;
    vec3 ro = vec3(radius * sin(orbit), 2.2 + 2.2 * build + 0.4 * sin(uMidTime * 0.04), radius * cos(orbit));
    vec3 ta = vec3(0.0, 1.8 + 0.6 * build, 0.0);
    vec3 fw = normalize(ta - ro);
    vec3 rt = normalize(cross(vec3(0.0, 1.0, 0.0), fw));
    vec3 up = cross(fw, rt);
    vec3 rd = normalize(uv.x * rt + uv.y * up + 1.4 * fw);
    rd = roleCamera(rd);                       // sub / mids / bassline move the camera

    float t = 0.05;
    bool hit = false;
    for (int i = 0; i < 140; i++)
    {
        if (i >= stepBudget(140)) break;   // adaptive quality
        vec3 p = ro + rd * t;
        float d = map(p);
        if (d < hitEps(t)) { hit = true; break; }
        t += d * 0.7;
        if (t > 40.0) break;
    }

    vec3 sky = mix(C0 * 0.35, C1 * 0.18, smoothstep(-0.2, 0.6, rd.y));
    vec3 col = sky;
    if (hit)
    {
        vec3 p = ro + rd * t;
        float cell = gCell;
        vec2 e = vec2(0.002, 0.0);
        vec3 n = normalize(vec3(map(p + e.xyy) - map(p - e.xyy), map(p + e.yxy) - map(p - e.yxy), map(p + e.yyx) - map(p - e.yyx)));
        bool isGround = p.y < 0.2 && n.y > 0.8;
        vec3 ld = normalize(vec3(-0.4, 0.8, 0.3));
        float fre = pow(1.0 - clamp(dot(n, -rd), 0.0, 1.0), 4.0);
        float spec = pow(clamp(dot(reflect(rd, n), ld), 0.0, 1.0), 80.0);
        if (isGround)
        {
            // a pool of light around the cluster, glossy floor catching the crystals' colour
            float pool = exp(-length(p.xz) * 0.45);
            col = C0 * (0.12 + 0.3 * clamp(dot(n, ld), 0.0, 1.0)) + C1 * 0.05;
            col += mix(C1, C2, 0.5) * pool * (0.12 + 0.6 * peak + 0.5 * uKick + 0.3 * build);
            col += C3 * pow(clamp(dot(reflect(rd, n), ld), 0.0, 1.0), 40.0) * pool * 0.4;
        }
        else
        {
            // crystal: reflected sky (fresnel) + inner light + prismatic edge
            vec3 refl = mix(C1, C2, 0.5 + 0.5 * reflect(rd, n).y);
            float wave = exp(-abs(length(p.xz) - (1.0 - uKick) * 4.0) * 1.5) * uKick;      // ring of light from the centre
            float inner = (0.05 + 0.25 * uBassSlow * uMacro.y + 0.45 * peak + 1.2 * wave) * (1.0 - 0.6 * build) + 0.3 * build * cell;
            col = mix(C1 * 0.12, refl * 0.8, fre) + mix(C1, C2, cell) * inner * (0.4 + 0.6 * (1.0 - fre));
            col += C3 * spec * (0.6 + 1.5 * uHighMid + 1.5 * uSnare);
            // inner veins: thin planes of light inside the mineral
            float vein = smoothstep(0.965, 1.0, abs(sin(dot(p, vec3(5.0, 2.3, 3.7)) + cell * 20.0)));
            col += mix(C2, C3, 0.4) * vein * (0.08 + 0.5 * peak + 0.4 * uKick) * (1.0 - fre);
            // facet edges: the numerical normal blends where two facets meet
            float facet = 1.0 - smoothstep(0.75, 0.98, max(abs(n.y), length(n.xz)));
            col += C3 * facet * (0.15 + 0.6 * uHighMid);
            // prismatic edges: facets whose normal turns sideways split the light
            float edge = smoothstep(0.85, 1.0, abs(n.x) + abs(n.z) * 0.3);
            col += vec3(0.6, 0.2, 1.0) * edge * (0.05 + 0.6 * peak + 0.5 * chaos) * fre;
        }
        float fog = 1.0 - exp(-t * 0.03);
        col = mix(col, sky, fog);
    }
    // sparkles in the air
    vec2 sg = uv * 50.0 + vec2(uMidTime * 0.3, uHighTime * 1.5);
    col += C3 * step(0.994 - 0.006 * uHigh, hash12(floor(sg))) * smoothstep(0.25, 0.0, length(fract(sg) - 0.5)) * (0.4 + uHigh + 0.6 * uHat);

    col *= 1.0 - 0.1 * calm;
    col *= smoothstep(2.4, 0.6, length(uv * vec2(0.8, 1.0)));      // soft vignette: the frame stays full on a big screen
    col *= uIntensity * 1.35 * mix(0.6, 1.0, uActivity);
    fragColor = vec4(col, 1.0);
}
