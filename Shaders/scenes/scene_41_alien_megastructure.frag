// ALIEN MEGASTRUCTURE — flying through a city of colossal alien monoliths (raymarched).
//
// Self-driving: every part is automated from the music.
//   bass / sub    flight speed; the towers' lights breathe
//   kick          a WAVE OF LIGHT runs through the windows, from the camera into the distance
//   snare         bridges flash
//   mids          the camera banks between the towers
//   highs         window lights twinkle
//   BUILD         the camera rises above the city, the planet on the horizon brightens
//   PEAK          every window blazes
//   CHAOS         the lights flicker, the towers shudder
//   colour        three colour families, the music moves between them
// Macros: A Tower Height · B Window Light · C Flight Speed · D Colour Family

vec3 C0, C1, C2, C3;
void setPalette()
{
    vec3 w = familyWeights(colourFamily(uMacro.w));
    // A: smog black · gunmetal · sodium amber · white   B: night · teal steel · cyan · white   C: black · bruise violet · magenta · pink-white
    vec3 a0 = hex(197379.0),  a1 = hex(3488572.0),  a2 = hex(16754230.0), a3 = hex(16777192.0);
    vec3 b0 = hex(66049.0),   b1 = hex(2770502.0),  b2 = hex(3790847.0),  b3 = hex(15794175.0);
    vec3 c0 = hex(131586.0),  c1 = hex(3346999.0),  c2 = hex(16722377.0), c3 = hex(16773375.0);
    C0 = a0 * w.x + b0 * w.y + c0 * w.z; C1 = a1 * w.x + b1 * w.y + c1 * w.z;
    C2 = a2 * w.x + b2 * w.y + c2 * w.z; C3 = a3 * w.x + b3 * w.y + c3 * w.z;
}

const float CELL = 6.0;
float gWin;    // window pattern at the hit point
float gBridge;

float sdBox(vec3 p, vec3 b) { vec3 q = abs(p) - b; return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0); }

float city(vec3 p)
{
    vec2 id = floor(p.xz / CELL);
    vec3 q = p;
    q.xz = mod(q.xz, CELL) - CELL * 0.5;
    float h = hash12(id);
    // the flight corridor (column x = 0) stays open
    float open = step(abs(id.x + 0.5), 0.6);
    float height = (6.0 + 22.0 * h * h) * (0.6 + 0.8 * uMacro.x);
    float w = 1.2 + 1.0 * hash12(id + 7.0);
    q.x += uState.w * 0.05 * sin(uHighTime * 13.0 + h * 40.0);
    // tower: a stepped monolith
    float tower = sdBox(q - vec3(0.0, height * 0.5, 0.0), vec3(w, height * 0.5, w));
    float crown = sdBox(q - vec3(0.0, height + 1.0, 0.0), vec3(w * 0.6, 1.0, w * 0.6));
    tower = min(tower, crown);
    tower = mix(tower, 1e3, open);
    // windows: a grid on the faces
    gWin = step(0.5, fract(q.y * 1.5)) * step(0.35, fract((q.x + q.z) * 1.2)) * step(0.4, hash12(floor(vec2(q.y * 1.5, (q.x + q.z) * 1.2)) + id));
    // bridges between towers at random heights
    float bh = 4.0 + 14.0 * hash12(id + 3.0);
    float bridge = sdBox(q - vec3(0.0, bh, 0.0), vec3(CELL * 0.5, 0.18, 0.3));
    bridge = mix(bridge, 1e3, step(0.55, hash12(id + 11.0)) + open);
    gBridge = step(bridge, tower);
    float ground = p.y;
    return min(min(tower, bridge), ground);
}

void main()
{
    setPalette();
    vec2 uv = (gl_FragCoord.xy - 0.5 * uRes) / uRes.y;
    float build = uState.y, peak = uState.z, chaos = uState.w, calm = uState.x;

    float z = uBassTime * (1.5 + 4.0 * uMacro.z) + uTime * 0.6;
    float camY = 5.0 + 3.0 * sin(uBassTime * 0.07) + 14.0 * build + 1.2 * uSub;
    vec3 ro = vec3(-CELL * 0.5 + 0.8 * sin(z * 0.05), camY, z);
    vec3 rd = normalize(vec3(uv, 1.4));
    rd = roleCamera(rd);                       // sub / mids / bassline move the camera
    rd.yz *= rot(-0.08 + 0.42 * build);                 // BUILD: rise and look down over the city
    rd.xy *= rot(0.12 * sin(uMidTime * 0.04));

    float t = 0.1;
    bool hit = false;
    for (int i = 0; i < 160; i++)
    {
        if (i >= stepBudget(160)) break;   // adaptive quality
        vec3 p = ro + rd * t;
        float d = city(p);
        if (d < hitEps(t) * 2.0) { hit = true; break; }
        t += d * 0.85;
        if (t > 220.0) break;
    }

    // sky: dusk gradient + a giant planet on the horizon
    vec3 sky = mix(C1 * 0.35, C0 * 0.6, smoothstep(-0.05, 0.4, rd.y));
    vec3 haze = sky;                                      // fog colour without the planet
    vec3 pd = normalize(vec3(0.35, 0.12, 1.0));
    float planet = smoothstep(0.985, 0.986, dot(rd, pd));
    sky = mix(sky, mix(C1, C2 * 0.6, 0.5 + 0.5 * rd.y) * (0.5 + 0.6 * build), planet);
    sky += C2 * 0.15 * pow(max(dot(rd, pd), 0.0), 40.0) * (1.0 + build);
    vec3 col = sky;
    if (hit)
    {
        vec3 p = ro + rd * t;
        float win = gWin, br = gBridge;
        vec2 e = vec2(0.01 + t * 0.001, 0.0);
        vec3 n = normalize(vec3(city(p + e.xyy) - city(p - e.xyy), city(p + e.yxy) - city(p - e.yxy), city(p + e.yyx) - city(p - e.yyx)));
        float key = clamp(dot(n, normalize(vec3(0.4, 0.6, 0.7))), 0.0, 1.0);
        col = mix(C0, C1, 0.6) * (0.06 + 0.4 * key);
        if (p.y > 0.05)
        {
            // windows: twinkle on the highs, blaze in PEAK; the kick's wave runs into the distance
            float wave = exp(-abs(t - (1.0 - uKick) * 120.0) * 0.08) * uKick;
            float tw = mix(1.0, step(0.3, hash12(floor(p.xy * 3.0) + floor(uHighTime * 2.0))), 0.3 + 0.6 * uHigh);   // highs twinkle
            float flick = mix(1.0, step(0.4, hash12(floor(p.xz) + floor(uHighTime * 10.0))), chaos);
            col += mix(C2, C3, 0.3) * win * (0.12 + 0.6 * uMacro.y * (0.4 + peak) + 0.6 * uMid + 2.0 * wave) * tw * flick * (1.0 - br);
            col += mix(C2, C3, 0.5) * br * (0.1 + 1.5 * uSnare);
        }
        float fog = 1.0 - exp(-t * 0.012);
        col = mix(col, haze * 0.9 + C2 * 0.04, fog);
    }

    col *= 1.0 - 0.1 * calm;
    col *= smoothstep(1.5, 0.45, length(uv * vec2(0.8, 1.0)));
    col *= uIntensity * 1.4 * mix(0.6, 1.0, uActivity);
    col *= 1.55;                                   // exposure matched to the other scenes (consistency pass)
    fragColor = vec4(col, 1.0);
}
