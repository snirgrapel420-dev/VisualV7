// NEON WIREFRAME — glowing 3D wireframe solids turning in space, mirrored into a symmetric sculpture.
//
// Self-driving: every part is automated from the music.
//   bass          the solids drift toward and away from the camera
//   kick          the shells punch outward, every edge flares
//   snare         the inner solid flips a quarter turn
//   hi-hat        sparks on the vertices
//   mids          the rotation
//   highs         light runs along the edges
//   BUILD         more shells unfold, the rotation winds up
//   PEAK          all shells blaze
//   CHAOS         the shells tumble on their own axes
//   colour        three colour families, the music moves between them
// Macros: A Symmetry · B Glow · C Spin Speed · D Colour Family

vec3 C0, C1, C2, C3;
void setPalette()
{
    vec3 w = familyWeights(colourFamily(uMacro.w));
    // A: magenta · gold · white      B: hot pink · amber · ice      C: violet · cyan · blue
    vec3 a0 = hex(1310740.0),  a1 = hex(16722902.0), a2 = hex(16758062.0), a3 = hex(16772567.0);
    vec3 b0 = hex(1311748.0),  b1 = hex(16727182.0), b2 = hex(16761402.0), b3 = hex(12579071.0);
    vec3 c0 = hex(393246.0),   c1 = hex(9063935.0),  c2 = hex(3073791.0),  c3 = hex(2976767.0);
    C0 = a0 * w.x + b0 * w.y + c0 * w.z; C1 = a1 * w.x + b1 * w.y + c1 * w.z;
    C2 = a2 * w.x + b2 * w.y + c2 * w.z; C3 = a3 * w.x + b3 * w.y + c3 * w.z;
}

// the 12 vertices of an icosahedron (edge length 2)
vec3 icoVert(int i)
{
    const float G = 1.61803399;
    int g = i / 4, k = i - g * 4;
    float a = (k == 0 || k == 1) ? 1.0 : -1.0;
    float b = (k == 0 || k == 2) ? G : -G;
    if (g == 0) return vec3(0.0, a, b);
    if (g == 1) return vec3(a, b, 0.0);
    return vec3(b, 0.0, a);
}

mat3 rotXYZ(vec3 a)
{
    float cx = cos(a.x), sx = sin(a.x), cy = cos(a.y), sy = sin(a.y), cz = cos(a.z), sz = sin(a.z);
    mat3 rx = mat3(1, 0, 0, 0, cx, sx, 0, -sx, cx);
    mat3 ry = mat3(cy, 0, -sy, 0, 1, 0, sy, 0, cy);
    mat3 rz = mat3(cz, sz, 0, -sz, cz, 0, 0, 0, 1);
    return rz * ry * rx;
}

// one glowing wireframe shell
vec3 shell(vec2 p, mat3 R, float scale, float w, vec3 edgeCol, float dist)
{
    vec3 col = vec3(0.0);
    vec2 pv[12]; float pz[12];
    for (int i = 0; i < 12; i++)
    {
        vec3 v = R * icoVert(i) * scale;
        float s = 1.4 / (dist + v.z);
        pv[i] = v.xy * s; pz[i] = v.z;
        // sparks on the vertices
        col += mix(edgeCol, vec3(1.0), 0.5) * exp(-length(p - pv[i]) / (w * 2.2)) * (0.25 + 1.4 * uHat);
    }
    for (int i = 0; i < 12; i++)
        for (int j = i + 1; j < 12; j++)
        {
            vec3 a = icoVert(i), b = icoVert(j);
            if (abs(dot(a - b, a - b) - 4.0) > 0.1) continue;          // icosahedron edges only (30 of 66 pairs)
            vec2 ba = pv[j] - pv[i];
            float h = clamp(dot(p - pv[i], ba) / max(dot(ba, ba), 1e-6), 0.0, 1.0);
            float d = length(p - pv[i] - ba * h);
            float z = mix(pz[i], pz[j], h) / max(scale, 1e-3);           // -1.9 near .. +1.9 far
            float lit = mix(1.15, 0.35, smoothstep(-1.9, 1.9, z));
            float runner = 0.75 + 0.5 * smoothstep(0.9, 1.0, sin(h * 9.0 - uHighTime * 4.0 + float(i + j)));
            col += neonTube(d, w * (1.25 - 0.25 * z / 1.9), edgeCol, 0.7) * lit * runner;
        }
    return col;
}

void main()
{
    setPalette();
    vec2 uv = centered();
    float build = uState.y, peak = uState.z, chaos = uState.w, calm = uState.x;

    // symmetry: none / mirrored left-right / mirrored four ways (macro A)
    vec2 p = uv;
    if (uMacro.x > 0.33) p.x = abs(p.x);
    if (uMacro.x > 0.66) p.y = abs(p.y);

    float spin = uMidTime * (0.12 + 0.3 * uMacro.z) * (1.0 + build) + uTime * 0.02;
    float dist = 4.6 - 0.5 * uBassSlow - 0.35 * sin(uBassTime * 0.07);
    float punch = 1.0 + 0.12 * uKick + 0.05 * uDrop;
    float w = 0.0042 * (1.0 + 0.5 * uKick + 0.3 * peak) * mix(0.8, 1.35, uMacro.y);
    float snareFlip = 0.6 * uSnare;

    vec3 col = vec3(0.0);
    int shells = 1 + int(build * 1.5 + peak * 1.2 + 0.5);
    for (int k = 0; k < 3; k++)
    {
        if (k >= shells) break;
        float fk = float(k);
        vec3 ang = vec3(spin * (0.7 + 0.3 * fk), spin * (1.0 - 0.45 * fk) * (mod(fk, 2.0) < 0.5 ? 1.0 : -1.0), 0.3 * fk)
                 + chaos * 0.8 * vec3(sin(uHighTime * (1.3 + fk)), cos(uHighTime * (1.1 + fk)), 0.0)
                 + vec3(0.0, fk == 0.0 ? snareFlip : 0.0, 0.0);
        float sc = (0.74 + 0.42 * fk) * punch;
        vec3 ec = fk == 0.0 ? C1 : (fk == 1.0 ? C2 : C3);
        col += shell(p, rotXYZ(ang), sc, w * (1.0 - 0.15 * fk), ec, dist) * (1.0 - 0.15 * fk);
    }

    // the core glow and the backdrop
    float r = length(uv);
    col += mix(C1, C2, 0.5) * exp(-r * 10.0) * (0.12 + 0.5 * uKick + 0.4 * peak);
    col += C0 * (1.0 - smoothstep(0.0, 1.0, r));

    col *= mix(0.65, 1.0, smoothstep(0.0, 0.6, uEnergyMed + peak)) * (1.0 + 0.25 * peak) * (1.0 - 0.15 * calm);
    col *= uIntensity * mix(0.55, 1.0, uActivity);
    fragColor = vec4(safeHDR(col), 1.0);
}
