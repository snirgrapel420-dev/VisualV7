// 05 GYROID CAVERNS — gliding through smooth organic caverns of a gyroid minimal surface.
//
// Self-driving: every part is automated from the music.
//   bass / sub    the flow: forward motion, and the cavern walls swell and breathe
//   kick          the bioluminescent veins PULSE outward from the camera
//   snare         a ripple runs over the walls
//   mids          the cavern morphs (a second gyroid blends in and out)
//   highs         the veins sparkle, fine pores open
//   BUILD         the caverns narrow, the veins dim, light gathers far ahead
//   PEAK          the veins blaze, the walls glow from within
//   CHAOS         the surface boils
//   colour        three colour families, the music moves between them
// Macros: A Cave Scale · B Veins · C Flow Speed · D Colour Family

vec3 C0, C1, C2, C3;
void setPalette()
{
    vec3 w = familyWeights(colourFamily(uMacro.w));
    // A: abyss · deep teal · cyan · pale aqua   B: black · moss · bio-lime · warm white   C: night · indigo · violet · pink
    vec3 a0 = hex(66568.0),   a1 = hex(800075.0),   a2 = hex(2416331.0),  a3 = hex(13434858.0);
    vec3 b0 = hex(65793.0),   a1b = hex(1981976.0), b2 = hex(11861312.0), b3 = hex(16774370.0);
    vec3 c0 = hex(131846.0),  c1 = hex(3021419.0), c2 = hex(9520361.0),  c3 = hex(16761036.0);
    C0 = a0 * w.x + b0 * w.y + c0 * w.z; C1 = a1 * w.x + a1b * w.y + c1 * w.z;
    C2 = a2 * w.x + b2 * w.y + c2 * w.z; C3 = a3 * w.x + b3 * w.y + c3 * w.z;
}

float gyroid(vec3 p) { return dot(sin(p), cos(p.yzx)); }

float gVein;

float map(vec3 p)
{
    float sc = 0.75 + 0.5 * uMacro.x;
    vec3 q = p * sc;
    float thick = 0.16 + 0.10 * uSub + 0.06 * uBassSlow + 0.08 * uState.y;   // membranes swell with the bass
    float g = gyroid(q);
    // mids blend in a second, rotated gyroid: the caverns morph
    float g2 = gyroid(q.zxy * 1.7 + vec3(0.0, uMidTime * 0.05, 0.0));
    g = mix(g, g + 0.35 * g2, smoothstep(0.2, 0.8, uMidMed));
    // snare ripple, chaos boil
    g += 0.08 * uSnare * sin(length(p) * 6.0 - uHighTime * 8.0);
    g += uState.w * 0.12 * vnoise(p.xy * 3.0 + uHighTime);
    float d = (abs(g) - thick) / (sc * 1.8);
    gVein = abs(gyroid(q * 3.1 + 1.3));                                      // vein network on the walls
    return d;
}

void main()
{
    setPalette();
    vec2 uv = (gl_FragCoord.xy - 0.5 * uRes) / uRes.y;
    float build = uState.y, peak = uState.z, chaos = uState.w, calm = uState.x;

    // the camera follows an open channel of the gyroid: along (pi/2, 0, z) the field is exactly 1,
    // far from the walls (|g| < thickness)
    float z = uBassTime * (0.4 + 1.2 * uMacro.z) + uTime * 0.1;
    vec3 ro = vec3(1.5708 + 0.18 * sin(z * 0.3), 0.18 * cos(z * 0.23), z);
    ro /= (0.75 + 0.5 * uMacro.x);
    vec3 fw = normalize(vec3(0.15 * sin(z * 0.2 + uMidTime * 0.05), 0.1 * cos(z * 0.17), 1.0));
    vec3 rt = normalize(cross(vec3(0.0, 1.0, 0.0), fw));
    vec3 up = cross(fw, rt);
    uv *= rot(0.2 * sin(z * 0.1));
    vec3 rd = normalize(uv.x * rt + uv.y * up + 1.2 * fw);
    rd = roleCamera(rd);                       // sub / mids / bassline move the camera

    float t = 0.02, glow = 0.0;
    bool hit = false;
    for (int i = 0; i < 140; i++)
    {
        if (i >= stepBudget(140)) break;   // adaptive quality
        vec3 p = ro + rd * t;
        float d = map(p);
        glow += exp(-abs(d) * 40.0) * 0.004;
        if (abs(d) < hitEps(t)) { hit = true; break; }
        t += abs(d) * 0.8;
        if (t > 22.0) break;
    }

    vec3 col = C0 * 0.4;
    if (hit)
    {
        vec3 p = ro + rd * t;
        float vein = gVein;
        vec2 e = vec2(0.002, 0.0);
        vec3 n = normalize(vec3(map(p + e.xyy) - map(p - e.xyy), map(p + e.yxy) - map(p - e.yxy), map(p + e.yyx) - map(p - e.yyx)));
        float head = clamp(dot(n, -rd), 0.0, 1.0);
        // key light from up-ahead reveals the curvature; a wet specular sheen; thin walls glow through
        vec3 kl = normalize(fw * 0.6 + up * 0.7 + rt * 0.3);
        float key = clamp(dot(n, kl), 0.0, 1.0);
        float spec = pow(clamp(dot(reflect(rd, n), kl), 0.0, 1.0), 30.0);
        float sss = pow(1.0 - head, 3.0);
        float ao = clamp(map(p + n * 0.15) / 0.15, 0.0, 1.0);
        vec3 flesh = mix(C1, C2 * 0.5, 0.25 * sss);
        col = flesh * (0.03 + 0.25 * head + 0.75 * key * key) * (0.25 + 0.75 * ao);
        col += mix(C2, C3, 0.6) * spec * ao * (0.5 + 0.8 * uHighMid);
        col += mix(C1, C2, 0.7) * sss * 0.35 * (0.3 + uLowMid + 0.5 * uSub);
        // bioluminescent veins; the kick sends a pulse outward from the camera
        float veinLine = smoothstep(0.16 - 0.06 * uMacro.y, 0.02, vein);
        float pulseR = (1.0 - uKick) * 12.0;
        float pulse = exp(-abs(t - pulseR) * 1.5) * uKick;
        float sparkle = pow(0.5 + 0.5 * sin(dot(p, vec3(13.0, 17.0, 11.0)) - uHighTime * 5.0), 20.0);
        col += mix(C2, C3, 0.4) * veinLine * (0.25 + 1.2 * peak + 3.0 * pulse + 1.4 * sparkle * uHigh) * (1.0 - 0.6 * build) * (0.5 + 0.5 * ao);
        float fog = 1.0 - exp(-t * 0.10);
        col = mix(col, C0 * 0.3 + C1 * 0.08, fog);
    }
    col += mix(C1, C2, 0.5) * glow * (0.3 + 0.8 * peak + 0.6 * uKick);
    col += C2 * build * 0.3 * exp(-length(uv) * 5.0);

    col *= 1.0 - 0.1 * calm;
    col *= smoothstep(2.4, 0.6, length(uv * vec2(0.8, 1.0)));      // soft vignette: the frame stays full on a big screen
    col *= uIntensity * 1.6 * mix(0.6, 1.0, uActivity);
    col *= 1.8;                                   // exposure matched to the other scenes (consistency pass)
    fragColor = vec4(col, 1.0);
}
