// 19 TIDAL CATHEDRAL — drifting through an alien cathedral at the bottom of the sea.
//
//   palette   abyss · indigo · lagoon cyan · pearl   (macro D: Coral Reef, Deep Gold variants)
//   kick      a WAVE OF LIGHT runs down the nave, from the viewer into the distance
//   sub/bass  the current: slow camera drift and sway, the water swells
//   mids      the pillars twist and the arches breathe
//   highs     caustics sharpen, plankton sparkles in the shafts of light
//   BUILD     the camera rises toward the surface, the light dims
//   PEAK      light floods the cathedral
//   CHAOS     the architecture shears and fractures
//   RELEASE   the light ebbs, long shafts remain
// Macros: A Architecture · B Caustics · C Drift Speed · D Colour Family (the music moves through the families)

vec3 P0, P1, P2, P3;
void setPalette()
{
    float v = colourFamily(uMacro.w);                    // evolves with the musical state
    vec3 a0 = hex(132106.0),  a1 = hex(1777502.0),  a2 = hex(4114121.0),  a3 = hex(16052712.0);  // abyss, indigo, cyan, pearl
    vec3 b0 = hex(198409.0),  b1 = hex(1189197.0),  b2 = hex(16743012.0), a3b = hex(16773335.0); // deep, sea teal, coral, sand
    vec3 c0 = hex(263428.0),  c1 = hex(2898722.0),  c2 = hex(13144398.0), c3 = hex(16774360.0);  // black, deep moss, old gold, ivory
    float wa = clamp(1.0 - v, 0.0, 1.0), wc = clamp(v - 1.0, 0.0, 1.0), wb = 1.0 - wa - wc;
    P0 = a0 * wa + b0 * wb + c0 * wc;  P1 = a1 * wa + b1 * wb + c1 * wc;
    P2 = a2 * wa + b2 * wb + c2 * wc;  P3 = a3 * wa + a3b * wb + c3 * wc;
}

float gMat;   // 0 floor, 1 pillar, 2 arch

float sdTorusXY(vec3 p, vec2 t) { vec2 q = vec2(length(p.xy) - t.x, p.z); return length(q) - t.y; }

float map(vec3 p)
{
    float chaos = uState.w;
    float cell = floor(p.z / 4.0);
    vec3 q = p;
    q.z = mod(q.z, 4.0) - 2.0;
    q.x += chaos * 0.25 * (hash12(vec2(cell, 3.0)) - 0.5) * smoothstep(-1.0, 2.0, q.y);   // shear in chaos

    float breathe = 1.0 + 0.04 * uMidMed + 0.03 * uMid + 0.05 * uSub;          // mids + sub breathe the arches
    // floor with slow dunes
    float fl = p.y + 1.25 - 0.08 * sin(p.x * 1.3 + p.z * 0.4) - 0.05 * sin(p.z * 1.7 - uBassTime * 0.3);
    // twisted pillars
    vec3 pp = vec3(abs(q.x) - 1.65 * breathe, q.y, q.z);
    float tw = (0.4 + 0.8 * uMacro.x) * (0.6 + 0.8 * uMidMed + 0.6 * uMid);
    pp.xz *= rot(q.y * tw);
    float pillar = max(length(pp.xz * vec2(1.0, 0.75)) - 0.22, abs(q.y - 0.2) - 1.6);
    pillar -= 0.03 * sin(q.y * 9.0 + atan(pp.z, pp.x) * 4.0);                              // carved fluting
    // pointed arch: two offset arcs meet at the apex
    vec3 ap = vec3(abs(q.x) + 0.55, q.y - 0.55, q.z);
    float arch = sdTorusXY(ap, vec2(2.2 * breathe, 0.14));
    arch = max(arch, -(q.y - 0.6));                                                       // only above the capitals
    arch = max(arch, abs(q.z) - 0.18);

    float d = fl; gMat = 0.0;
    float dp = smin(pillar, fl, 0.35);
    if (dp < d) { d = dp; gMat = pillar < fl ? 1.0 : 0.0; }
    if (arch < d) { d = arch; gMat = 2.0; }
    return d;
}

vec3 normalAt(vec3 p)
{
    vec2 e = vec2(0.002, 0.0);
    return normalize(vec3(map(p + e.xyy) - map(p - e.xyy), map(p + e.yxy) - map(p - e.yxy), map(p + e.yyx) - map(p - e.yyx)));
}

float caustic(vec2 x)
{
    float t = uMidTime * 0.25 + uBassTime * 0.15;
    float c = 0.0;
    vec2 q = x * (1.6 + 1.4 * uMacro.y);
    for (int i = 0; i < 3; i++)
    {
        q = rot(1.1) * q + vec2(0.3 * t, -0.2 * t);
        c += abs(sin(q.x + sin(q.y * 1.3 + t))) ;
    }
    return pow(clamp(1.0 - c / 3.0, 0.0, 1.0), 4.0 - 2.0 * uHighMid) * 3.0;
}

void main()
{
    setPalette();
    vec2 uv = (gl_FragCoord.xy - 0.5 * uRes) / uRes.y;
    float build = uState.y, peak = uState.z, chaos = uState.w, calm = uState.x, rel = uStateRelease;

    float travel = uBassTime * (0.4 + 1.2 * uMacro.z) + uTime * 0.12;
    vec3 ro = vec3(0.35 * sin(uBassTime * 0.11), -0.15 + 0.9 * build + 0.08 * sin(uBassTime * 0.17), travel);
    vec3 rd = normalize(vec3(uv, 1.15));
    rd = roleCamera(rd);                       // sub / mids / bassline move the camera
    rd.yz *= rot(-0.06 - 0.18 * build);
    rd.xy *= rot(0.05 * sin(uBassTime * 0.09) + 0.02 * uPan);
    rd.xz *= rot(0.08 * sin(uBassTime * 0.07));

    float t = 0.0, shafts = 0.0;
    float hit = 0.0;
    for (int i = 0; i < 140; i++)
    {
        if (i >= stepBudget(140)) break;   // adaptive quality
        vec3 p = ro + rd * t;
        float d = map(p);
        // light shafts from the surface: volume sampled along the ray
        float sh = smoothstep(0.55, 0.95, vnoise(vec2(p.x * 0.6 + 0.3 * p.y, p.z * 0.25 - uMidTime * 0.05)));
        shafts += sh * exp(-t * 0.12) * 0.012 * max(0.0, 1.0 + p.y * 0.3);
        if (d < hitEps(t) * 1.5) { hit = 1.0; break; }
        t += d * 0.8;
        if (t > 40.0) break;
    }

    // light level of the cathedral: dim in BUILD, flooding in PEAK
    float lightLvl = 0.55 + 0.6 * peak + 0.4 * chaos - 0.35 * build - 0.15 * calm + 0.2 * rel;
    vec3 fogCol = mix(P0, P1, 0.35 + 0.25 * lightLvl);
    vec3 col = fogCol;
    if (hit > 0.5)
    {
        vec3 p = ro + rd * t;
        vec3 n = normalAt(p);
        float mat = gMat;
        vec3 ld = normalize(vec3(0.15, 1.0, 0.25));
        float dif = clamp(dot(n, ld), 0.0, 1.0);
        float up = clamp(n.y * 0.5 + 0.5, 0.0, 1.0);
        float ca = caustic(p.xz + p.y * 0.3) * up;
        vec3 base = mat < 0.5 ? mix(P1 * 0.8, P2 * 0.35, 0.3) : (mat < 1.5 ? mix(P1, P3 * 0.5, 0.25) : mix(P1 * 1.2, P3 * 0.6, 0.3));
        col = base * (0.12 + 0.6 * dif * lightLvl);
        col += mix(P2, P3, 0.4) * ca * (0.25 + 0.6 * lightLvl) * (0.5 + 0.8 * uHighMid);
        // the kick's wave of light running down the nave
        float wavePos = ro.z + 2.0 + (1.0 - uKick) * 26.0;
        col += mix(P2, P3, 0.6) * exp(-abs(p.z - wavePos) * 1.5) * uKick * (0.6 + 0.8 * peak);
        float fog = 1.0 - exp(-t * (0.11 - 0.03 * lightLvl));
        col = mix(col, fogCol, fog);
    }
    // god rays + plankton
    col += mix(P2, P3, 0.5) * shafts * (0.4 + 0.9 * lightLvl);
    vec2 pg = uv * 48.0 + vec2(uMidTime * 0.6, uHighTime * 2.0);
    float plank = step(0.992 - 0.006 * uHigh - 0.01 * chaos, hash12(floor(pg))) * smoothstep(0.22, 0.0, length(fract(pg) - 0.5));
    col += P3 * plank * (0.15 + 0.8 * uHigh + 0.5 * uHat);

    col *= smoothstep(1.4, 0.4, length(uv * vec2(0.8, 1.0)));
    col *= uIntensity * 1.3 * mix(0.6, 1.0, uActivity);
    fragColor = vec4(col, 1.0);
}
