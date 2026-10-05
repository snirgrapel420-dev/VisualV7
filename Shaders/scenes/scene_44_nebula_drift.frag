// NEBULA DRIFT — volumetric flight through glowing clouds of a stellar nursery.
//
// Self-driving: every part is automated from the music.
//   bass / sub    drift speed; the clouds swell and thin (density breathes)
//   kick          LIGHTNING: a flash lights the clouds from within
//   snare         a shock ripple through the gas
//   mids          the turbulence turns, filaments reshape
//   highs         young stars twinkle
//   BUILD         the clouds part ahead, revealing a blazing core
//   PEAK          emission at full strength, the gas glows on its own
//   CHAOS         storm turbulence, fast-moving filaments
//   colour        three colour families, the music moves between them
// Macros: A Density · B Emission · C Drift Speed · D Colour Family

vec3 C0, C1, C2, C3;
void setPalette()
{
    vec3 w = familyWeights(colourFamily(uMacro.w));
    // A: space · indigo · hydrogen rose · white   B: space · teal · oxygen cyan · white   C: space · rust · sulfur gold · white
    vec3 a0 = hex(66056.0),   a1 = hex(2167145.0),  a2 = hex(15686815.0), a3 = hex(16774399.0);
    vec3 b0 = hex(66056.0),   b1 = hex(1132658.0),  b2 = hex(4054527.0),  b3 = hex(15794175.0);
    vec3 c0 = hex(131330.0),  c1 = hex(7155246.0),  c2 = hex(16762424.0), c3 = hex(16777192.0);
    C0 = a0 * w.x + b0 * w.y + c0 * w.z; C1 = a1 * w.x + b1 * w.y + c1 * w.z;
    C2 = a2 * w.x + b2 * w.y + c2 * w.z; C3 = a3 * w.x + b3 * w.y + c3 * w.z;
}

float density(vec3 p)
{
    float chaos = uState.w;
    vec3 q = p * 0.35;
    q.xy *= rot(uMidTime * 0.01);
    // domain-warped fbm: filaments
    vec3 w = vec3(fbm(q.xy + q.z * 0.3, 2), fbm(q.yz + 3.1, 2), fbm(q.zx - 1.7, 2)) - 0.5;
    q += w * (1.2 + 1.5 * chaos + 0.8 * uMid) + 0.15 * uSnare * sin(q.zxy * 6.0);
    float d = fbm(q.xy * 1.3 + q.z * 0.7, 3) * fbm(q.yz * 0.9 - q.x * 0.4 + 2.0, 2) * 1.1;
    d = d * 2.2 - (0.72 - 0.3 * uMacro.x - 0.08 * uSub - 0.06 * uBassSlow);
    // BUILD: the clouds part along the flight line, ahead
    float tunnel = length(p.xy) / (0.5 + 3.0 * uState.y);
    d -= uState.y * 0.6 * smoothstep(1.0, 0.0, tunnel);
    return clamp(d, 0.0, 1.0);
}

void main()
{
    setPalette();
    vec2 uv = (gl_FragCoord.xy - 0.5 * uRes) / uRes.y;
    float build = uState.y, peak = uState.z, chaos = uState.w, calm = uState.x;

    float z = uBassTime * (0.4 + 1.2 * uMacro.z) * (1.0 + chaos) + uTime * 0.1;
    vec3 ro = vec3(0.0, 0.0, z);
    vec3 rd = normalize(vec3(uv, 1.2));
    rd = roleCamera(rd);                       // sub / mids / bassline move the camera
    rd.xy *= rot(0.05 * sin(uMidTime * 0.02));

    // stars behind everything
    vec2 sp = uv * 180.0;
    vec3 col = C0 * 0.4;
    float st = step(0.997, hash12(floor(sp)));
    col += C3 * st * (0.4 + 0.6 * step(0.5, hash12(floor(sp) + floor(uHighTime * 3.0)))) * (0.5 + uHigh);

    // volumetric march, front to back
    vec3 acc = vec3(0.0);
    float trans = 1.0;
    float flash = uKick * (0.4 + 0.6 * peak);
    vec3 flashPos = vec3(1.5 * sin(floor(uBeatClock) * 2.3), 1.0 * cos(floor(uBeatClock) * 1.7), z + 6.0);
    float t = 0.5;
    for (int i = 0; i < 48; i++)
    {
        if (i >= stepBudget(48)) break;   // adaptive quality
        vec3 p = ro + rd * t;
        float d = density(p);
        if (d > 0.001)
        {
            // self-emission + light from the core ahead + the kick's lightning
            float core = exp(-length(p.xy) * 0.8) * (0.08 + 0.8 * build + 0.15 * peak);
            float lightning = flash * 2.5 / (1.0 + dot(p - flashPos, p - flashPos) * 1.2);
            vec3 emit = ramp4(clamp(d * 1.6, 0.0, 1.0), C0, C1, C2, C3) * (0.22 + 0.5 * uMacro.y * (0.3 + peak) + 0.35 * uBassNote + 0.15 * uMid)   // the bassline pulses the gas
                      + C2 * core + C3 * lightning;
            float a = d * 0.16;
            acc += trans * a * emit;
            trans *= 1.0 - a;
            if (trans < 0.02) break;
        }
        t += 0.33 + t * 0.025;
    }
    col = col * trans + acc;
    // the core itself, glimpsed through the parting clouds during BUILD
    col += mix(C2, C3, 0.5) * build * 0.8 * exp(-length(uv) * 9.0) * trans;

    col *= 1.0 - 0.1 * calm;
    col *= smoothstep(1.5, 0.45, length(uv * vec2(0.8, 1.0)));
    col *= uIntensity * 1.4 * mix(0.6, 1.0, uActivity);
    col *= 0.85;                                   // exposure matched to the other scenes (consistency pass)
    fragColor = vec4(col, 1.0);
}
