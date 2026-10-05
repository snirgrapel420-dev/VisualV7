// FRACTAL OCEAN — gliding low over a night sea of fractal waves under a vast sky (raymarched heightfield).
//
// Self-driving: every part is automated from the music.
//   bass / sub    the SWELL: wave height and the slow rise and fall of the camera
//   kick          a ring wave spreads across the water from ahead, its crest flashes
//   snare         spray: foam bursts along the crests
//   mids          wind direction turns, the chop changes
//   highs         moonlight glitters on the facets
//   BUILD         the sea calms to glass, the moon grows, the horizon brightens
//   PEAK          the foam glows (bioluminescent sea)
//   CHAOS         storm: tall, choppy, fast waves
//   colour        three colour families, the music moves between them
// Macros: A Wave Height · B Foam Glow · C Glide Speed · D Colour Family

vec3 C0, C1, C2, C3;
void setPalette()
{
    vec3 w = familyWeights(colourFamily(uMacro.w));
    // A: midnight · deep blue · moonlight · white   B: night violet · plum · rose gold · cream   C: black · teal · bio-cyan · white
    vec3 a0 = hex(66320.0),   a1 = hex(1323862.0),  a2 = hex(10465763.0), a3 = hex(16316671.0);
    vec3 b0 = hex(1049123.0), b1 = hex(5056342.0),  b2 = hex(14721140.0), b3 = hex(16774616.0);
    vec3 c0 = hex(65793.0),   c1 = hex(681575.0),   c2 = hex(3141367.0),  c3 = hex(15204351.0);
    C0 = a0 * w.x + b0 * w.y + c0 * w.z; C1 = a1 * w.x + b1 * w.y + c1 * w.z;
    C2 = a2 * w.x + b2 * w.y + c2 * w.z; C3 = a3 * w.x + b3 * w.y + c3 * w.z;
}

float gCamZ;

float waves(vec2 p, int oct)
{
    float storm = uState.w, calmSea = uState.y;
    float amp = (0.35 + 0.5 * uMacro.x) * (0.55 + 0.5 * uBassSlow + 0.3 * uSub + 0.45 * storm) * (1.0 - 0.75 * calmSea) * (1.0 + 0.25 * uMid);
    float t = uBassTime * (0.4 + 0.6 * storm) + uTime * 0.2;
    vec2 dir = vec2(cos(uMidTime * 0.02 + 0.5 * uMid), sin(uMidTime * 0.02 + 0.5 * uMid));
    float h = 0.0, a = amp, f = 0.35;
    vec2 q = p;
    for (int i = 0; i < 7; i++)
    {
        if (i >= oct) break;
        q = rot(1.1) * q;
        float w = dot(q, dir) * f + t * (1.0 + 0.3 * float(i));
        // sharp-crested waves: folded sine
        h += a * (1.0 - abs(sin(w + vnoise(q * f * 0.8) * 2.0)));
        a *= 0.48; f *= 1.9;
    }
    // the kick's ring wave, travelling out from a point ahead
    vec2 c = vec2(0.0, gCamZ + 14.0);
    float r = length(p - c);
    float ring = (1.0 - uKick) * 22.0;
    h += 0.6 * uKick * exp(-abs(r - ring) * 0.8);
    return h;
}

void main()
{
    setPalette();
    vec2 uv = (gl_FragCoord.xy - 0.5 * uRes) / uRes.y;
    float build = uState.y, peak = uState.z, chaos = uState.w, calm = uState.x;

    float z = uBassTime * (0.6 + 2.0 * uMacro.z) + uTime * 0.3;
    gCamZ = z;
    float camH = 2.2 + 0.5 * sin(uBassTime * 0.15) + 0.6 * chaos;
    vec3 ro = vec3(0.0, camH, z);
    vec3 rd = normalize(vec3(uv.x, uv.y - 0.12, 1.4));
    rd = roleCamera(rd);                       // sub / mids / bassline move the camera
    rd.xy *= rot(0.04 * sin(uBassTime * 0.11) + 0.05 * chaos * sin(uAbsTime * 3.0));

    // sky: gradient, stars, moon
    vec3 moonDir = normalize(vec3(0.25, 0.22 + 0.05 * build, 1.0));
    vec3 sky = mix(C0 * 0.6, C1 * 0.7, smoothstep(0.35, -0.05, rd.y));
    sky += C2 * 0.25 * exp(-max(rd.y, 0.0) * 8.0) * (0.6 + 0.8 * build);
    float moon = smoothstep(0.9993 - 0.0015 * build, 0.9996 - 0.0015 * build, dot(rd, moonDir));
    sky += C3 * moon + C2 * pow(max(dot(rd, moonDir), 0.0), 120.0) * 0.6;
    vec2 sp = rd.xy / max(rd.z, 0.1) * 120.0;
    sky += C3 * step(0.996, hash12(floor(sp))) * smoothstep(0.0, 0.3, rd.y) * 0.5;

    vec3 col = sky;
    if (rd.y < 0.02)
    {
        // march the heightfield
        float t = 0.1, lastH = 0.0, lastY = 0.0;
        bool hit = false;
        for (int i = 0; i < 120; i++)
        {
            if (i >= stepBudget(120)) break;   // adaptive quality
            vec3 p = ro + rd * t;
            float h = waves(p.xz, 4);
            if (p.y < h) { hit = true; t -= (lastY - lastH) / max((lastY - lastH) - (p.y - h), 1e-4) * (t * 0.02 + 0.05) * 0.0; break; }
            lastH = h; lastY = p.y;
            t += max(0.03, (p.y - h) * 0.45) * (1.0 + t * 0.015);
            if (t > 90.0) break;
        }
        if (hit)
        {
            vec3 p = ro + rd * t;
            float e = 0.02 + t * 0.002;
            float h0 = waves(p.xz, 7);
            vec3 n = normalize(vec3(h0 - waves(p.xz + vec2(e, 0.0), 7), e, h0 - waves(p.xz + vec2(0.0, e), 7)));
            float fre = pow(1.0 - clamp(dot(n, -rd), 0.0, 1.0), 4.0);
            vec3 refl = reflect(rd, n);
            vec3 skyR = mix(C0 * 0.6, C1 * 0.7, smoothstep(0.35, -0.05, refl.y)) + C2 * pow(max(dot(refl, moonDir), 0.0), 60.0) * 2.0;
            vec3 water = mix(C0 * 0.4, C1 * 0.5, 0.3 + 0.4 * clamp(h0, 0.0, 1.0));
            col = mix(water, skyR, 0.2 + 0.8 * fre);
            // moon glitter on the facets (highs)
            float glit = pow(max(dot(refl, moonDir), 0.0), 900.0) * (0.6 + 3.0 * uHigh);
            col += C3 * glit;
            // foam on the crests: snare spray, PEAK bioluminescence
            float crest = smoothstep(0.9, 1.15, h0 / max(0.35 + 0.5 * uMacro.x, 0.1));
            float foamNoise = vnoise(p.xz * 3.0 + uHighTime);
            col += mix(C2, C3, 0.5) * crest * foamNoise * foamNoise * (0.06 + 0.5 * uSnare + 0.45 * peak * (0.3 + uMacro.y) + 0.3 * chaos);
            float fog = 1.0 - exp(-t * 0.025);
            col = mix(col, sky * 0.9 + C2 * 0.05, fog);
        }
    }

    col *= 1.0 - 0.1 * calm;
    col *= smoothstep(1.5, 0.45, length(uv * vec2(0.8, 1.0)));
    col *= uIntensity * 1.3 * mix(0.6, 1.0, uActivity);
    col *= 0.85;                                   // exposure matched to the other scenes (consistency pass)
    fragColor = vec4(col, 1.0);
}
