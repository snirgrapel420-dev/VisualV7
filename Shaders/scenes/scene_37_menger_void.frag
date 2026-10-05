// MENGER VOID — flying through the corridors of an endless Menger sponge (raymarched).
// Square holes within square holes, forever; light pours in through the openings.
//
// Self-driving: every part is automated from the music.
//   bass / sub    flight speed; the walls breathe (the sponge's holes widen)
//   kick          a PULSE OF LIGHT runs down the corridor and through the side openings
//   snare         the corridor rotates a quarter-step (the camera rolls)
//   mids          the camera drifts between the hole levels
//   highs         dust glitters in the shafts
//   BUILD         the light at the end of the corridor grows, the walls darken
//   PEAK          the edges of every hole glow
//   CHAOS         the hole size jitters: the sponge flickers between levels
//   colour        three colour families, the music moves between them
// Macros: A Depth (levels) · B Edge Light · C Flight Speed · D Colour Family

vec3 C0, C1, C2, C3;
void setPalette()
{
    vec3 w = familyWeights(colourFamily(uMacro.w));
    // A: concrete black · cold grey · ice blue · white   B: black · rust · amber · cream   C: void · violet · cyan · white
    vec3 a0 = hex(263172.0),  a1 = hex(5000268.0),  a2 = hex(9618423.0),  a3 = hex(16316671.0);
    vec3 b0 = hex(197379.0),  b1 = hex(8730157.0),  b2 = hex(16756531.0), b3 = hex(16774370.0);
    vec3 c0 = hex(131845.0),  c1 = hex(5121959.0),  c2 = hex(4317151.0),  c3 = hex(16777215.0);
    C0 = a0 * w.x + b0 * w.y + c0 * w.z; C1 = a1 * w.x + b1 * w.y + c1 * w.z;
    C2 = a2 * w.x + b2 * w.y + c2 * w.z; C3 = a3 * w.x + b3 * w.y + c3 * w.z;
}

float gEdge;

float menger(vec3 p)
{
    p = mod(p + 1.0, 2.0) - 1.0;                                    // endless lattice of sponges
    float d = 1e9;
    float hole = 1.0 + 0.06 * uSub + 0.04 * uBassSlow + 0.05 * uMid + 0.05 * uState.w * sin(uHighTime * 3.0);
    float s = 1.0;
    gEdge = 1e9;
    int levels = 3 + int(uMacro.x * 2.0 + 0.5);
    float box = max(max(abs(p.x), abs(p.y)), abs(p.z)) - 1.0;
    d = box;
    for (int i = 0; i < 5; i++)
    {
        if (i >= levels) break;
        vec3 a = mod(p * s, 2.0) - 1.0;
        s *= 3.0;
        vec3 r = abs(1.0 - 3.0 * abs(a));
        float da = max(r.x, r.y), db = max(r.y, r.z), dc = max(r.z, r.x);
        float c = (min(da, min(db, dc)) - hole) / s;
        d = max(d, c);
    }
    return d;
}

void main()
{
    setPalette();
    vec2 uv = (gl_FragCoord.xy - 0.5 * uRes) / uRes.y;
    float build = uState.y, peak = uState.z, chaos = uState.w, calm = uState.x;

    float z = uBassTime * (0.15 + 0.5 * uMacro.z) * (1.0 + 0.6 * build) + uTime * 0.04;
    // the central corridor (0, 0, z) passes through the big holes of every cube
    vec3 ro = vec3(0.12 * sin(uMidTime * 0.05), 0.12 * cos(uMidTime * 0.04), z);
    vec3 fw = normalize(vec3(0.15 * sin(uMidTime * 0.03), 0.1 * cos(uMidTime * 0.025), 1.0));
    vec3 rt = normalize(cross(vec3(0.0, 1.0, 0.0), fw));
    vec3 up = cross(fw, rt);
    float roll = 0.15 * sin(uMidTime * 0.02) + 1.5708 * floor(uSnare * 0.0) + 0.4 * uSnare;
    uv *= rot(roll);
    vec3 rd = normalize(uv.x * rt + uv.y * up + 1.2 * fw);
    rd = roleCamera(rd);                       // sub / mids / bassline move the camera

    float t = 0.02, shafts = 0.0;
    bool hit = false;
    for (int i = 0; i < 150; i++)
    {
        if (i >= stepBudget(150)) break;   // adaptive quality
        vec3 p = ro + rd * t;
        float d = menger(p);
        shafts += exp(-abs(d) * 40.0) * 0.0012;
        if (d < hitEps(t)) { hit = true; break; }
        t += d * 0.9;
        if (t > 30.0) break;
    }

    float endLight = (0.15 + 1.0 * build + 0.3 * peak) * exp(-length(uv) * 3.5);
    vec3 col = C0 * 0.3 + C2 * endLight;
    if (hit)
    {
        vec3 p = ro + rd * t;
        vec2 e = vec2(0.001 * t + 0.0005, 0.0);
        vec3 n = normalize(vec3(menger(p + e.xyy) - menger(p - e.xyy), menger(p + e.yxy) - menger(p - e.yxy), menger(p + e.yyx) - menger(p - e.yyx)));
        vec3 ld = normalize(vec3(0.3, 0.7, 0.6));
        float key = clamp(dot(n, ld), 0.0, 1.0);
        float head = clamp(dot(n, -rd), 0.0, 1.0);
        float ao = clamp(menger(p + n * 0.04) / 0.04, 0.0, 1.0);
        col = mix(C1, C2, 0.35) * (0.06 + 0.6 * key + 0.35 * head) * (0.2 + 0.8 * ao) * (1.0 - 0.4 * build) * (1.0 + 0.3 * peak);
        // edges of the holes: glow in PEAK; the kick's pulse travels down the corridor
        // true edges: where two faces meet, the (numerical) normal leaves the axis directions
        vec3 an = abs(n);
        float rim = smoothstep(0.93, 0.75, max(an.x, max(an.y, an.z)));
        float pulse = exp(-abs(p.z - (ro.z + 0.5 + (1.0 - uKick) * 18.0)) * 1.2) * uKick;
        col += mix(C2, C3, 0.5) * rim * (0.04 + 0.35 * peak * (0.3 + uMacro.y) + 1.2 * pulse);
        col += C2 * pulse * 0.12;
        float fog = 1.0 - exp(-t * 0.09);
        col = mix(col, C0 * 0.3 + C2 * 0.25 * build, fog);
    }
    // light shafts and dust
    // shafts belong to open space: rays grazing the corridor walls would pile them up into a white veil
    col += mix(C2, C3, 0.5) * shafts * (hit ? 0.12 : 1.0) * (0.3 + 0.5 * peak + 0.4 * uKick);
    vec2 sg = uv * 40.0 + vec2(uMidTime * 0.3, uHighTime);
    col += C3 * step(0.993 - 0.005 * uHigh, hash12(floor(sg))) * smoothstep(0.25, 0.0, length(fract(sg) - 0.5)) * (0.25 + 0.8 * uHigh);

    col *= 1.0 - 0.1 * calm;
    col *= smoothstep(2.4, 0.6, length(uv * vec2(0.8, 1.0)));      // soft vignette: the frame stays full on a big screen
    col *= uIntensity * 1.45 * mix(0.6, 1.0, uActivity);
    fragColor = vec4(col, 1.0);
}
