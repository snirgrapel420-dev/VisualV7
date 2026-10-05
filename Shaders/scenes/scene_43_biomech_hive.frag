// BIOMECH HIVE — flying through a hexagonal hive of biomechanical cells (raymarched).
//
// Self-driving: every part is automated from the music.
//   bass / sub    flight speed; the cell walls swell like breathing tissue
//   kick          a PULSE runs along the ribs, from the camera into the hive
//   snare         the membranes twitch
//   mids          the hive slowly twists around the flight axis
//   highs         wet glints on the ribs
//   BUILD         the openings close, the honey-light deep ahead grows
//   PEAK          the honey cores in every cell blaze
//   CHAOS         the tissue convulses
//   colour        three colour families, the music moves between them
// Macros: A Cell Size · B Honey Glow · C Flight Speed · D Colour Family

vec3 C0, C1, C2, C3;
void setPalette()
{
    vec3 w = familyWeights(colourFamily(uMacro.w));
    // A: chitin black · bronze · honey amber · cream   B: black · bone grey · bio-green · pale   C: black · wine · hot orange · gold
    vec3 a0 = hex(328451.0),  a1 = hex(6966308.0),  a2 = hex(16756544.0), a3 = hex(16774604.0);
    vec3 b0 = hex(131586.0),  b1 = hex(6118749.0),  b2 = hex(9436266.0),  b3 = hex(15727840.0);
    vec3 c0 = hex(131074.0),  c1 = hex(6227989.0),  c2 = hex(16737582.0), c3 = hex(16767334.0);
    C0 = a0 * w.x + b0 * w.y + c0 * w.z; C1 = a1 * w.x + b1 * w.y + c1 * w.z;
    C2 = a2 * w.x + b2 * w.y + c2 * w.z; C3 = a3 * w.x + b3 * w.y + c3 * w.z;
}

float gRib;
vec2 gCell;

// distance to the nearest hexagonal cell edge (Voronoi of a triangular lattice, spacing 1)
float hexEdge(vec2 p, out vec2 cid)
{
    vec2 s = vec2(1.0, 1.7320508);
    vec2 a = mod(p, s) - s * 0.5;
    vec2 b = mod(p - s * 0.5, s) - s * 0.5;
    vec2 q = dot(a, a) < dot(b, b) ? a : b;
    cid = floor(p - q + 0.5);
    float m = max(abs(q.x), max(abs(dot(q, vec2(0.5, 0.8660254))), abs(dot(q, vec2(-0.5, 0.8660254)))));
    return 0.5 - m;
}

float map(vec3 p)
{
    float cs = 3.0 + 2.0 * uMacro.x;
    vec3 q = p;
    q.xy *= rot(p.z * 0.02 + uMidTime * 0.01);
    vec2 cid;
    float e = hexEdge(q.xy / cs, cid) * cs;
    gCell = cid;
    float thick = 0.22 + 0.06 * uSub + 0.04 * uBassSlow + 0.06 * uMid + 0.1 * uState.y;
    float wall = e - thick;
    // organic tissue
    wall -= 0.05 * vnoise(q.xy * 2.0 + q.z * 0.5) + uState.w * 0.05 * sin(q.z * 4.0 + uHighTime * 6.0) + 0.03 * uSnare * sin(q.z * 9.0);
    // openings into the neighbour cells (shrink in BUILD)
    float zz = mod(q.z, 4.0) - 2.0;
    float hole = length(vec2(e - thick * 0.5, zz) * vec2(2.0, 1.0)) - (0.75 - 0.5 * uState.y);
    wall = max(wall, -hole);
    // ribs: rings around each cell every 2 units
    float rz = mod(q.z, 2.0) - 1.0;
    float rib = length(vec2(e - thick - 0.05, rz)) - 0.09;
    gRib = step(rib, wall);
    return smin(wall, rib, 0.08);
}

void main()
{
    setPalette();
    vec2 uv = (gl_FragCoord.xy - 0.5 * uRes) / uRes.y;
    float build = uState.y, peak = uState.z, chaos = uState.w, calm = uState.x;

    float z = uBassTime * (0.5 + 1.5 * uMacro.z) + uTime * 0.15;
    vec3 ro = vec3(0.08 * sin(z * 0.3), 0.08 * cos(z * 0.23), z);       // centre of a cell
    vec3 rd = normalize(vec3(uv, 1.1));
    rd = roleCamera(rd);                       // sub / mids / bassline move the camera
    rd.xy *= rot(0.1 * sin(uMidTime * 0.03));

    float t = 0.05;
    bool hit = false;
    for (int i = 0; i < 140; i++)
    {
        if (i >= stepBudget(140)) break;   // adaptive quality
        vec3 p = ro + rd * t;
        float d = map(p);
        if (d < hitEps(t)) { hit = true; break; }
        t += d * 0.8;
        if (t > 40.0) break;
    }

    float deep = (0.12 + 0.9 * build + 0.3 * peak) * exp(-length(uv) * 4.0);
    vec3 col = C0 * 0.3 + C2 * deep;
    if (hit)
    {
        vec3 p = ro + rd * t;
        float isRib = gRib;
        vec2 cid = gCell;
        vec2 e = vec2(0.002 + t * 0.0005, 0.0);
        vec3 n = normalize(vec3(map(p + e.xyy) - map(p - e.xyy), map(p + e.yxy) - map(p - e.yxy), map(p + e.yyx) - map(p - e.yyx)));
        float head = clamp(dot(n, -rd), 0.0, 1.0);
        float ao = clamp(map(p + n * 0.2) / 0.2, 0.0, 1.0);
        float wet = pow(clamp(dot(reflect(rd, n), -rd), 0.0, 1.0), 24.0);
        vec3 tissue = mix(C1 * 0.6, C1, isRib);
        col = tissue * (0.04 + 0.55 * head * head) * (0.25 + 0.75 * ao);
        col += C3 * wet * ao * (0.25 + 0.6 * uHigh);
        // honey cores glowing from inside the neighbouring cells (visible through the openings)
        float honey = hash12(cid);
        col += C2 * (1.0 - ao) * (0.06 + 0.9 * peak * (0.3 + uMacro.y) * honey);
        // the kick's pulse along the ribs
        float pulse = exp(-abs(t - (1.0 - uKick) * 24.0) * 0.8) * uKick;
        col += mix(C2, C3, 0.5) * isRib * pulse * 1.5;
        float fog = 1.0 - exp(-t * 0.07);
        col = mix(col, C0 * 0.3 + C2 * deep * 0.5, fog);
    }

    col *= 1.0 - 0.1 * calm;
    col *= smoothstep(1.5, 0.45, length(uv * vec2(0.8, 1.0)));
    col *= uIntensity * 1.45 * mix(0.6, 1.0, uActivity);
    col *= 1.7;                                   // exposure matched to the other scenes (consistency pass)
    fragColor = vec4(col, 1.0);
}
