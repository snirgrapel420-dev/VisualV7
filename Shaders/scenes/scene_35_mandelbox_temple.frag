// MANDELBOX TEMPLE — flying inside a Mandelbox: a box-folding fractal whose negative scale
// opens vast architectural halls (raymarched).
//
// Self-driving: every part is automated from the music.
//   bass / sub    flight through the halls, slow breathing of the whole structure
//   kick          the SCALE pulses: every hall swells and settles (the temple breathes)
//   snare         an extra fold snaps through the walls
//   mids          the camera drifts and turns
//   highs         glints on the box edges
//   BUILD         the halls darken, a light opens far ahead
//   PEAK          the carved edges glow
//   CHAOS         the scale wavers: walls ripple between forms
//   colour        three colour families, the music moves between them
// Macros: A Scale · B Edge Glow · C Flight Speed · D Colour Family

vec3 C0, C1, C2, C3;
void setPalette()
{
    vec3 w = familyWeights(colourFamily(uMacro.w));
    // A: basalt · slate blue · sand · warm white   B: black · oxblood · brass · cream   C: ink · jade · pale mint · white
    vec3 a0 = hex(328965.0),  a1 = hex(3361142.0),  a2 = hex(13144458.0), a3 = hex(16774618.0);
    vec3 b0 = hex(197379.0),  b1 = hex(6693654.0),  b2 = hex(12945212.0), b3 = hex(16773076.0);
    vec3 c0 = hex(131843.0),  c1 = hex(1474408.0),  c2 = hex(11397582.0), c3 = hex(16252927.0);
    C0 = a0 * w.x + b0 * w.y + c0 * w.z; C1 = a1 * w.x + b1 * w.y + c1 * w.z;
    C2 = a2 * w.x + b2 * w.y + c2 * w.z; C3 = a3 * w.x + b3 * w.y + c3 * w.z;
}

float gTrap;

float mbox(vec3 p)
{
    float S = -1.75 - 0.2 * uMacro.x - 0.06 * uMid - 0.06 * uKick - 0.04 * uSub + 0.05 * uState.w * sin(uHighTime * 2.0);
    vec4 z = vec4(p, 1.0);
    vec3 c = p;
    gTrap = 1e9;
    for (int i = 0; i < 11; i++)
    {
        z.xyz = clamp(z.xyz, -1.0, 1.0) * 2.0 - z.xyz;                // box fold
        if (i == 3) z.xyz += 0.15 * uSnare * sin(z.zxy * 3.0);       // the snare's extra fold
        float r2 = dot(z.xyz, z.xyz);
        gTrap = min(gTrap, r2);
        float k = clamp(max(0.25 / r2, 0.25), 0.0, 1.0) * 4.0;        // sphere fold
        z *= k;
        z = vec4(z.xyz * S + c, z.w * abs(S) + 1.0);
    }
    return (length(z.xyz) - abs(S - 1.0)) / z.w - pow(abs(S), -10.0);
}

void main()
{
    setPalette();
    vec2 uv = (gl_FragCoord.xy - 0.5 * uRes) / uRes.y;
    float build = uState.y, peak = uState.z, chaos = uState.w, calm = uState.x;

    float t0 = uBassTime * (0.03 + 0.1 * uMacro.z) + uTime * 0.005;
    vec3 ro = vec3(1.2 * sin(t0), 0.5 * sin(t0 * 0.7 + 1.0), 1.2 * cos(t0)) * 3.4;
    vec3 ta = vec3(0.4 * sin(uMidTime * 0.03), 0.2 * cos(uMidTime * 0.02), 0.0);
    vec3 fw = normalize(ta - ro);
    vec3 rt = normalize(cross(vec3(0.0, 1.0, 0.0), fw));
    vec3 up = cross(fw, rt);
    vec3 rd = normalize(uv.x * rt + uv.y * up + 1.05 * fw);
    rd = roleCamera(rd);                       // sub / mids / bassline move the camera

    float t = 0.05, glow = 0.0;
    bool hit = false;
    for (int i = 0; i < 160; i++)
    {
        if (i >= stepBudget(160)) break;   // adaptive quality
        vec3 p = ro + rd * t;
        float d = mbox(p);
        glow += exp(-d * 60.0) * 0.004;
        if (d < hitEps(t)) { hit = true; break; }
        t += d * 0.8;
        if (t > 20.0) break;
    }

    vec3 col = mix(C0 * 0.3, C1 * 0.15, 0.5 + 0.5 * rd.y);
    if (hit)
    {
        float trap = gTrap;
        vec3 p = ro + rd * t;
        vec2 e = vec2(0.0008 * t + 0.0003, 0.0);
        vec3 n = normalize(vec3(mbox(p + e.xyy) - mbox(p - e.xyy), mbox(p + e.yxy) - mbox(p - e.yxy), mbox(p + e.yyx) - mbox(p - e.yyx)));
        float ao = clamp(mbox(p + n * 0.05) / 0.05, 0.0, 1.0);
        vec3 ld = normalize(vec3(0.3, 0.9, 0.2));
        float key = clamp(dot(n, ld), 0.0, 1.0);
        float head = clamp(dot(n, -rd), 0.0, 1.0);
        float spec = pow(clamp(dot(reflect(rd, n), ld), 0.0, 1.0), 30.0);
        vec3 stone = ramp4(0.2 + 0.6 * clamp(sqrt(trap), 0.0, 1.0), C0, C1, C2, C3);
        col = stone * (0.04 + 0.55 * key + 0.25 * head) * (0.15 + 0.85 * ao * ao);
        col += C3 * spec * ao * (0.3 + 1.2 * uHighMid + 0.6 * uHat);
        // carved edges glow (low trap = deep in the fold structure)
        float edge = smoothstep(0.12, 0.0, trap);
        col += mix(C2, C3, 0.5) * edge * (0.03 + 0.35 * peak * (0.3 + uMacro.y) + 0.45 * uKick);
        float fog = 1.0 - exp(-t * (0.07 + 0.1 * build));
        col = mix(col, C0 * 0.3 + C2 * 0.15 * build, fog);
    }
    col += mix(C1, C2, 0.5) * glow * (0.3 + 0.8 * peak + 0.6 * uKick);
    col += C2 * build * 0.3 * exp(-length(uv) * 5.0);

    col *= 1.0 - 0.1 * calm;
    col *= smoothstep(2.4, 0.6, length(uv * vec2(0.8, 1.0)));      // soft vignette: the frame stays full on a big screen
    col *= uIntensity * 1.45 * mix(0.6, 1.0, uActivity);
    fragColor = vec4(col, 1.0);
}
