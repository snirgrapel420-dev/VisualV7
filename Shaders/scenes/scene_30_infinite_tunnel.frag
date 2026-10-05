// 03 INFINITE TUNNEL — a colossal winding tunnel with engraved fractal walls (raymarched 3D).
//
// Self-driving: every part is automated from the music.
//   bass / sub    the throttle: forward speed, and the tunnel's slow breathing
//   kick          the tunnel BLOWS OPEN (radius punch) and a ring of light races ahead
//   snare         the wall carvings jolt (the fractal folds snap to a new angle)
//   mids          the path winds and banks; the carving pattern morphs
//   highs         sparks run along the grooves
//   BUILD         the tunnel narrows and accelerates toward a growing light at the end
//   PEAK          the grooves glow neon, full speed
//   CHAOS         the walls twist and shear, the camera shakes
//   colour        three colour families, the music moves between them
// Macros: A Wall Detail · B Ribs · C Speed · D Colour Family

vec3 C0, C1, C2, C3;
void setPalette()
{
    vec3 w = familyWeights(colourFamily(uMacro.w));
    // A: obsidian · steel blue · cyan · white-gold   B: void · deep red · orange · cream   C: black · ultraviolet · magenta · pale pink
    vec3 a0 = hex(197379.0),  a1 = hex(2109268.0),  a2 = hex(4440027.0),  a3 = hex(16773062.0);
    vec3 b0 = hex(131843.0),  b1 = hex(7213588.0),  b2 = hex(16737843.0), b3 = hex(16771535.0);
    vec3 c0 = hex(131586.0),  c1 = hex(3678615.0),  c2 = hex(14166205.0), c3 = hex(16768760.0);
    C0 = a0 * w.x + b0 * w.y + c0 * w.z; C1 = a1 * w.x + b1 * w.y + c1 * w.z;
    C2 = a2 * w.x + b2 * w.y + c2 * w.z; C3 = a3 * w.x + b3 * w.y + c3 * w.z;
}

vec2 path(float z)
{
    float wind = 0.8 + 0.6 * uMidMed;
    return vec2(sin(z * 0.11) * 1.6 + sin(z * 0.047) * 1.2, cos(z * 0.083) * 1.1) * wind;
}

float gPattern;   // groove value at the hit point

// engraved fractal pattern on the tube surface: a 2D kaleidoscopic fold in (angle, depth)
float carving(vec2 uv)
{
    float jolt = 0.4 * uSnare + 0.25 * uState.w * sin(uHighTime);
    vec2 q = uv;
    float acc = 0.0, s = 1.0;
    int iters = 3 + int(uMacro.x * 3.0 + 0.5);
    for (int i = 0; i < 6; i++)
    {
        if (i >= iters) break;
        q = abs(fract(q) - 0.5);
        q *= rot(0.6 + jolt + 0.1 * float(i));
        q *= 1.7;
        s *= 1.7;
        acc += smoothstep(0.06, 0.0, abs(q.x - 0.25) - 0.02) / float(i + 1);
    }
    return acc;
}

float mapFull(vec3 p, out float isRib)
{
    vec2 c = path(p.z);
    vec2 d = p.xy - c;
    float r = length(d);
    float a = atan(d.y, d.x) + uState.w * 0.4 * sin(p.z * 0.7 + uHighTime);
    float R = 2.1 * (1.0 + 0.18 * uKick + 0.05 * uSub) * (1.0 - 0.25 * uState.y);
    gPattern = carving(vec2(a / TAU * 6.0, p.z * 0.35));
    float wall = R - r - gPattern * 0.10;
    float ribGap = 6.0 - 3.0 * uMacro.y;
    float rz = mod(p.z, ribGap) - ribGap * 0.5;
    float rib = length(vec2(r - R, rz * 1.6)) - 0.28;
    isRib = step(rib, wall);
    return min(wall, rib);
}

void main()
{
    setPalette();
    vec2 uv = (gl_FragCoord.xy - 0.5 * uRes) / uRes.y;
    float build = uState.y, peak = uState.z, chaos = uState.w, calm = uState.x;

    // camera rides the path; bass is the throttle, BUILD accelerates
    float speed = (0.6 + 1.6 * uMacro.z) * (1.0 + 0.8 * build + 0.4 * peak);
    float z = uBassTime * speed + uTime * 0.2;
    vec3 ro = vec3(path(z), z);
    vec3 ta = vec3(path(z + 2.0), z + 2.0);
    vec3 fw = normalize(ta - ro);
    vec3 rt = normalize(cross(vec3(0.0, 1.0, 0.0), fw));
    vec3 up = cross(fw, rt);
    float bank = 0.25 * (path(z + 3.0).x - path(z).x) + 0.05 * chaos * sin(uAbsTime * 23.0);
    uv *= rot(bank);
    vec3 rd = normalize(uv.x * rt + uv.y * up + (1.3 - 0.25 * build) * fw);
    rd = roleCamera(rd);                       // sub / mids / bassline move the camera

    float t = 0.05, isRib = 0.0;
    float glowAcc = 0.0;
    bool hit = false;
    for (int i = 0; i < 150; i++)
    {
        if (i >= stepBudget(150)) break;   // adaptive quality
        vec3 p = ro + rd * t;
        float d = mapFull(p, isRib);
        glowAcc += exp(-d * 30.0) * 0.004;
        if (d < hitEps(t) * 1.5) { hit = true; break; }
        t += d * 0.75;
        if (t > 60.0) break;
    }

    // the light at the end of the tunnel (grows during BUILD)
    float endLight = exp(-t * 0.035) * (0.08 + 0.9 * build + 0.25 * peak);
    vec3 col = C0 * 0.6;
    if (hit)
    {
        vec3 p = ro + rd * t;
        float pat = gPattern;
        vec2 e = vec2(0.003, 0.0);
        float dum;
        vec3 n = normalize(vec3(mapFull(p + e.xyy, dum) - mapFull(p - e.xyy, dum),
                                mapFull(p + e.yxy, dum) - mapFull(p - e.yxy, dum),
                                mapFull(p + e.yyx, dum) - mapFull(p - e.yyx, dum)));
        // a light carried with the camera, slightly above: walls fall off into darkness
        vec3 ld = normalize(-rd + up * 0.6);
        float dif = clamp(dot(n, ld), 0.0, 1.0);
        float spec = pow(clamp(dot(reflect(rd, n), ld), 0.0, 1.0), 24.0);
        float cavity = 1.0 - smoothstep(0.2, 1.0, pat);                 // grooves sit in shadow
        vec3 stone = mix(C1, C2 * 0.6, 0.2 + 0.3 * pat);
        vec3 metal = mix(C1 * 0.5, C3 * 0.6, 0.3);
        vec3 base = mix(stone, metal, isRib);
        col = base * (0.06 + 0.5 * dif * dif) * (0.35 + 0.65 * cavity);
        col += mix(C2, C3, 0.6) * spec * (0.25 + 0.5 * isRib) * (0.4 + uHighMid);
        // grooves glow as lines only: neon in PEAK, sparks running along them on the highs
        float line = smoothstep(0.75, 1.0, pat) * smoothstep(1.4, 1.0, pat);
        float spark = pow(0.5 + 0.5 * sin(p.z * 3.0 - uHighTime * 6.0 + pat * 10.0), 16.0);
        col += mix(C2, C3, 0.4) * line * (0.12 + 0.6 * peak * (0.4 + uMid) + 0.3 * uLowMid + 1.2 * spark * uHigh);
        // the kick's ring of light racing ahead
        float ringZ = ro.z + 1.0 + (1.0 - uKick) * 22.0;
        col += mix(C2, C3, 0.6) * exp(-abs(p.z - ringZ) * 2.0) * uKick * (0.6 + 0.6 * line + 0.4 * isRib);
        float fog = 1.0 - exp(-t * 0.06);
        col = mix(col, C0 * 0.6 + C2 * endLight * 0.25, fog);
    }
    col += mix(C2, C3, 0.4) * endLight * smoothstep(0.35, 0.0, length(uv));
    col += C2 * glowAcc * (0.2 + 0.8 * peak + 0.6 * uKick);

    col *= 1.0 - 0.1 * calm;
    col *= smoothstep(1.5, 0.45, length(uv * vec2(0.8, 1.0)));
    col *= uIntensity * 1.3 * mix(0.6, 1.0, uActivity);
    col *= 1.6;                                   // exposure matched to the other scenes (consistency pass)
    fragColor = vec4(col, 1.0);
}
