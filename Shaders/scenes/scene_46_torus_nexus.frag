// TORUS NEXUS — a dimension machine: nested rings turning on different axes around a living core.
//
// Self-driving: every part is automated from the music.
//   mids          the rings' rotation speeds
//   kick          every ring EXPANDS for a moment, the core flashes
//   snare         a ring jumps a step
//   sub / bass    the core breathes; slow orbit of the camera
//   highs         light runs along the engraved grooves of the rings
//   BUILD         the rings ALIGN into one plane: a portal opens around the core
//   PEAK          the core blazes, the grooves glow
//   CHAOS         the rings wobble erratically
//   colour        three colour families, the music moves between them
// Macros: A Rings · B Groove Light · C Spin Speed · D Colour Family

vec3 C0, C1, C2, C3;
void setPalette()
{
    vec3 w = familyWeights(colourFamily(uMacro.w));
    // A: void · brushed steel · electric blue · white   B: void · brass · amber · cream   C: void · obsidian violet · magenta · white
    vec3 a0 = hex(131843.0),  a1 = hex(7107208.0),  a2 = hex(3109375.0),  a3 = hex(16316671.0);
    vec3 b0 = hex(131586.0),  b1 = hex(11897395.0), b2 = hex(16757806.0), b3 = hex(16775392.0);
    vec3 c0 = hex(131330.0),  c1 = hex(3812431.0),  c2 = hex(15284943.0), c3 = hex(16774399.0);
    C0 = a0 * w.x + b0 * w.y + c0 * w.z; C1 = a1 * w.x + b1 * w.y + c1 * w.z;
    C2 = a2 * w.x + b2 * w.y + c2 * w.z; C3 = a3 * w.x + b3 * w.y + c3 * w.z;
}

float gRing; float gGroove;

float sdTorus(vec3 p, vec2 t) { return length(vec2(length(p.xz) - t.x, p.y)) - t.y; }

float map(vec3 p)
{
    float align = smoothstep(0.1, 0.9, uState.y);                     // BUILD: the rings align
    int rings = 4 + int(uMacro.x * 3.0 + 0.5);
    float d = 1e9;
    gRing = -1.0;
    for (int i = 0; i < 7; i++)
    {
        if (i >= rings) break;
        float fi = float(i);
        float R = (0.8 + 0.42 * fi) * (1.0 + 0.08 * uKick + 0.03 * uMid * fi);   // mids spread the rings
        float spin = uMidTime * (0.15 + 0.35 * uMacro.z) * (1.0 + 0.4 * fi) * (mod(fi, 2.0) < 0.5 ? 1.0 : -1.0)
                   + 0.5 * uSnare * step(0.5, hash12(vec2(fi, floor(uBeatClock))));
        float wob = uState.w * 0.3 * sin(uHighTime * (3.0 + fi) + fi);
        vec3 q = p;
        q.yz *= rot(mix(spin + fi * 0.7 + wob, 0.0, align));
        q.xy *= rot(mix(spin * 0.6 + fi * 1.3 + wob, 0.0, align));
        float tr = sdTorus(q, vec2(R, 0.055 + 0.012 * fi));
        if (tr < d) { d = tr; gRing = fi; gGroove = sin(atan(q.z, q.x) * (24.0 + 8.0 * fi) - uHighTime * 4.0); }
    }
    // the core
    float core = length(p) - (0.32 + 0.12 * uSub + 0.06 * uKick);
    if (core < d) { d = core; gRing = -1.0; }
    return d;
}

void main()
{
    setPalette();
    vec2 uv = (gl_FragCoord.xy - 0.5 * uRes) / uRes.y;
    float build = uState.y, peak = uState.z, chaos = uState.w, calm = uState.x;

    float orbit = uBassTime * 0.05 + uTime * 0.02;
    vec3 ro = 5.0 * vec3(sin(orbit), 0.35 + 0.25 * build, cos(orbit));
    vec3 fw = normalize(-ro);
    vec3 rt = normalize(cross(vec3(0.0, 1.0, 0.0), fw));
    vec3 up = cross(fw, rt);
    vec3 rd = normalize(uv.x * rt + uv.y * up + 1.8 * fw);
    rd = roleCamera(rd);                       // sub / mids / bassline move the camera

    float t = 0.0, coreGlow = 0.0;
    bool hit = false;
    for (int i = 0; i < 120; i++)
    {
        if (i >= stepBudget(120)) break;   // adaptive quality
        vec3 p = ro + rd * t;
        float d = map(p);
        coreGlow += exp(-max(length(p) - 0.3, 0.0) * 4.0) * 0.01;
        if (d < hitEps(t)) { hit = true; break; }
        t += d * 0.9;
        if (t > 14.0) break;
    }

    // starfield
    vec2 sp = uv * 160.0;
    vec3 col = sceneBackdrop(rd, C0, C1, C2);                 // a living space around the machine
    if (hit)
    {
        vec3 p = ro + rd * t;
        float ring = gRing, groove = gGroove;
        vec2 e = vec2(0.001, 0.0);
        vec3 n = normalize(vec3(map(p + e.xyy) - map(p - e.xyy), map(p + e.yxy) - map(p - e.yxy), map(p + e.yyx) - map(p - e.yyx)));
        vec3 ld = normalize(-p);                                       // lit by the core
        float coreLit = clamp(dot(n, ld), 0.0, 1.0) / (1.0 + dot(p, p) * 0.15);
        float key = clamp(dot(n, normalize(vec3(0.4, 0.8, 0.3))), 0.0, 1.0);
        float spec = pow(clamp(dot(reflect(rd, n), normalize(vec3(0.4, 0.8, 0.3))), 0.0, 1.0), 50.0);
        if (ring < 0.0)
        {
            col = mix(C2, C3, 0.5 + 0.5 * peak) * (0.6 + 0.8 * uSub + 1.5 * uKick + 0.8 * peak);
        }
        else
        {
            col = C1 * (0.05 + 0.45 * key) + C2 * coreLit * (0.6 + 1.2 * peak + uKick);
            col += C3 * spec * (0.4 + 0.8 * uHighMid);
            col += mix(C2, C3, 0.5) * smoothstep(0.9, 1.0, groove) * (0.1 + 0.8 * uMacro.y * (0.3 + peak) + 0.8 * uHigh);
        }
    }
    col += mix(C2, C3, 0.4) * coreGlow * (0.5 + 1.2 * peak + 1.5 * uKick + 1.0 * build);
    // BUILD: the portal plane lights up when the rings align
    col += C2 * build * 0.25 * exp(-abs(uv.y) * 12.0) * smoothstep(1.2, 0.2, abs(uv.x));

    col *= 1.0 - 0.1 * calm;
    col *= smoothstep(2.4, 0.6, length(uv * vec2(0.8, 1.0)));      // soft vignette: the frame stays full on a big screen
    col *= uIntensity * 1.35 * mix(0.6, 1.0, uActivity);
    fragColor = vec4(col, 1.0);
}
