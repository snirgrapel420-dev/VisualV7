// KIFS SPIRE — a kaleidoscopic IFS sculpture that keeps refolding itself (raymarched).
//
// Self-driving: every part is automated from the music.
//   mids          the fold angles turn: the sculpture continuously reshapes
//   kick          the folds JOLT: the whole spire rearranges for a moment
//   sub / bass    the orbit breathes, the scale swells
//   snare         a single fold flips
//   highs         glints on the edges
//   BUILD         the sculpture contracts to a tight core, lit from below
//   PEAK          the deep recesses glow
//   CHAOS         the folds flicker between configurations
//   colour        three colour families, the music moves between them
// Macros: A Fold Complexity · B Glow · C Orbit Speed · D Colour Family

vec3 C0, C1, C2, C3;
void setPalette()
{
    vec3 w = familyWeights(colourFamily(uMacro.w));
    // A: graphite · steel · chrome blue · white   B: black · burnt copper · gold · cream   C: void · emerald · lime · white
    vec3 a0 = hex(263172.0),  a1 = hex(4609114.0),  a2 = hex(9418201.0),  a3 = hex(16316671.0);
    vec3 b0 = hex(131586.0),  b1 = hex(9127187.0),  b2 = hex(15377455.0), b3 = hex(16775399.0);
    vec3 c0 = hex(65793.0),   c1 = hex(816731.0),   c2 = hex(11336260.0), c3 = hex(16777215.0);
    C0 = a0 * w.x + b0 * w.y + c0 * w.z; C1 = a1 * w.x + b1 * w.y + c1 * w.z;
    C2 = a2 * w.x + b2 * w.y + c2 * w.z; C3 = a3 * w.x + b3 * w.y + c3 * w.z;
}

float gTrap;

float kifs(vec3 p)
{
    float scale = 2.0 + 0.1 * uSub - 0.2 * uState.y;
    vec3 off = vec3(1.4, 0.95, 0.75);
    float a1 = 0.45 + 0.3 * sin(uMidTime * 0.05) + 0.2 * uMid + 0.25 * uKick + 0.1 * uState.w * sin(uHighTime * 4.0);
    float a2 = 0.35 + 0.25 * cos(uMidTime * 0.04) + 0.3 * uSnare;
    float s = 1.0;
    gTrap = 1e9;
    int iters = 7 + int(uMacro.x * 4.0 + 0.5);
    for (int i = 0; i < 11; i++)
    {
        if (i >= iters) break;
        p = abs(p);
        if (p.x < p.y) p.xy = p.yx;
        if (p.x < p.z) p.xz = p.zx;
        if (p.y < p.z) p.yz = p.zy;
        p.xy *= rot(a1);
        p.yz *= rot(a2);
        p = p * scale - off * (scale - 1.0);
        if (p.z < -0.5 * off.z * (scale - 1.0)) p.z += off.z * (scale - 1.0);
        s *= scale;
        gTrap = min(gTrap, length(p) / s);
    }
    vec3 q = abs(p) - vec3(0.55, 0.55, 0.55);
    return (length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0)) / s;
}

void main()
{
    setPalette();
    vec2 uv = (gl_FragCoord.xy - 0.5 * uRes) / uRes.y;
    float build = uState.y, peak = uState.z, chaos = uState.w, calm = uState.x;

    float orbit = uBassTime * (0.04 + 0.15 * uMacro.z) + uTime * 0.02;
    float dist = 3.5 - 0.3 * uBassSlow + 0.3 * calm - 0.5 * build;
    vec3 ro = dist * vec3(sin(orbit), 0.35 + 0.2 * sin(orbit * 0.6), cos(orbit));
    vec3 fw = normalize(-ro);
    vec3 rt = normalize(cross(vec3(0.0, 1.0, 0.0), fw));
    vec3 up = cross(fw, rt);
    vec3 rd = normalize(uv.x * rt + uv.y * up + 1.6 * fw);
    rd = roleCamera(rd);                       // sub / mids / bassline move the camera

    float t = 0.0, glow = 0.0;
    bool hit = false;
    for (int i = 0; i < 160; i++)
    {
        if (i >= stepBudget(160)) break;   // adaptive quality
        vec3 p = ro + rd * t;
        float d = kifs(p);
        glow += exp(-d * 50.0) * 0.004;
        if (d < hitEps(t) * 0.7) { hit = true; break; }
        t += d * 0.85;
        if (t > 12.0) break;
    }

    vec3 col = sceneBackdrop(rd, C0, C1, C2);                 // a living space around the sculpture
    if (hit)
    {
        float trap = gTrap;
        vec3 p = ro + rd * t;
        vec2 e = vec2(0.0005 * t + 0.0002, 0.0);
        vec3 n = normalize(vec3(kifs(p + e.xyy) - kifs(p - e.xyy), kifs(p + e.yxy) - kifs(p - e.yxy), kifs(p + e.yyx) - kifs(p - e.yyx)));
        float ao = clamp(kifs(p + n * 0.04) / 0.04, 0.0, 1.0);
        vec3 ld = normalize(mix(vec3(0.5, 0.8, 0.3), vec3(0.0, -1.0, 0.2), build));   // BUILD: lit from below
        float key = clamp(dot(n, ld), 0.0, 1.0);
        float head = clamp(dot(n, -rd), 0.0, 1.0);
        float spec = pow(clamp(dot(reflect(rd, n), ld), 0.0, 1.0), 40.0);
        vec3 surf = ramp4(0.25 + 0.65 * clamp(trap * 3.0, 0.0, 1.0), C0, C1, C2, C3);
        col = surf * (0.05 + 0.65 * key + 0.25 * head) * (0.2 + 0.8 * ao * ao);
        col += C3 * spec * ao * (0.4 + 1.2 * uHighMid + 0.6 * uHat);
        col += mix(C1, C2, 0.6) * (1.0 - ao) * (0.05 + 0.8 * peak + 0.6 * uKick);
    }
    col += mix(C1, C2, 0.5) * glow * (0.3 + 0.8 * uMacro.y) * (0.4 + 0.8 * peak + 0.6 * uKick);

    col *= 1.0 - 0.1 * calm;
    col *= smoothstep(2.4, 0.6, length(uv * vec2(0.8, 1.0)));      // soft vignette: the frame stays full on a big screen
    col *= uIntensity * 1.4 * mix(0.6, 1.0, uActivity);
    fragColor = vec4(col, 1.0);
}
