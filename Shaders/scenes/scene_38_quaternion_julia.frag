// QUATERNION JULIA — a Julia set in four dimensions, seen as a 3D cross-section (raymarched).
// The 4D constant travels with the music: the form is reborn continuously.
//
// Self-driving: every part is automated from the music.
//   bass clock    the 4D constant travels: the whole shape morphs between forms
//   kick          a jump of the constant: the form snaps into a new shape and settles
//   sub           the orbit breathes in and out
//   mids          the 3D slice rotates through the fourth dimension
//   highs         glints on the thinnest filaments
//   BUILD         the morph slows to a crawl, the shape is lit only from behind (silhouette)
//   PEAK          the inner layers glow through the surface
//   CHAOS         the constant shivers: the form flickers
//   colour        three colour families, the music moves between them
// Macros: A Morph Range · B Glow · C Orbit Speed · D Colour Family

vec3 C0, C1, C2, C3;
void setPalette()
{
    vec3 w = familyWeights(colourFamily(uMacro.w));
    // A: ink · deep teal · seafoam · pearl   B: void · magenta · orange · cream   C: black · gold · white gold · white
    vec3 a0 = hex(197642.0),  a1 = hex(946031.0),   a2 = hex(8382118.0),  a3 = hex(15924214.0);
    vec3 b0 = hex(131586.0),  b1 = hex(13047173.0), b2 = hex(16748077.0), b3 = hex(16775645.0);
    vec3 c0 = hex(65793.0),   c1 = hex(10518800.0), c2 = hex(15387772.0), c3 = hex(16777215.0);
    C0 = a0 * w.x + b0 * w.y + c0 * w.z; C1 = a1 * w.x + b1 * w.y + c1 * w.z;
    C2 = a2 * w.x + b2 * w.y + c2 * w.z; C3 = a3 * w.x + b3 * w.y + c3 * w.z;
}

vec4 gC;
float gTrap;

vec4 qsqr(vec4 a) { return vec4(a.x * a.x - dot(a.yzw, a.yzw), 2.0 * a.x * a.yzw); }

float julia(vec3 p)
{
    // the 3D slice through 4D space turns with the mids
    vec4 z = vec4(p, 0.0);
    z.xw *= rot(0.3 * sin(uMidTime * 0.05) + 0.25 * uMid);         // mids turn the 4D slice
    float md2 = 1.0, mz2 = dot(z, z);
    gTrap = 1e9;
    for (int i = 0; i < 11; i++)
    {
        md2 *= 4.0 * mz2;
        z = qsqr(z) + gC;
        mz2 = dot(z, z);
        gTrap = min(gTrap, abs(z.x) + 0.5 * length(z.yz));
        if (mz2 > 4.0) break;
    }
    return 0.25 * sqrt(mz2 / md2) * log(mz2);
}

void main()
{
    setPalette();
    vec2 uv = (gl_FragCoord.xy - 0.5 * uRes) / uRes.y;
    float build = uState.y, peak = uState.z, chaos = uState.w, calm = uState.x;

    float m = uBassTime * (0.06 + 0.1 * uMacro.x) * (1.0 - 0.8 * build) + 0.35 * uKick + chaos * 0.03 * sin(uHighTime * 7.0);
    // around a classic constant that gives delicate filaments; the music moves it within a safe range
    gC = vec4(-0.125, -0.256, 0.847, 0.0895) + (0.06 + 0.06 * uMacro.x) * cos(vec4(0.5, 3.9, 1.4, 1.1) + m * vec4(1.2, 1.7, 1.3, 2.5));

    float orbit = uBassTime * (0.04 + 0.15 * uMacro.z) + uTime * 0.02;
    float dist = 2.75 - 0.25 * uSub + 0.25 * calm;
    vec3 ro = dist * vec3(sin(orbit), 0.3 * sin(orbit * 0.6), cos(orbit));
    vec3 fw = normalize(-ro);
    vec3 rt = normalize(cross(vec3(0.0, 1.0, 0.0), fw));
    vec3 up = cross(fw, rt);
    vec3 rd = normalize(uv.x * rt + uv.y * up + 1.7 * fw);
    rd = roleCamera(rd);                       // sub / mids / bassline move the camera

    float t = 0.0, glow = 0.0;
    bool hit = false;
    for (int i = 0; i < 160; i++)
    {
        if (i >= stepBudget(160)) break;   // adaptive quality
        vec3 p = ro + rd * t;
        float d = julia(p);
        glow += exp(-d * 30.0) * 0.006;
        if (d < hitEps(t) * 0.5) { hit = true; break; }
        t += d * 0.8;
        if (t > 6.0) break;
    }

    vec3 col = mix(C0 * 0.35, C1 * 0.12, 0.5 + 0.5 * uv.y);
    if (hit)
    {
        float trap = gTrap;
        vec3 p = ro + rd * t;
        vec2 e = vec2(0.0005 * t + 0.0002, 0.0);
        vec3 n = normalize(vec3(julia(p + e.xyy) - julia(p - e.xyy), julia(p + e.yxy) - julia(p - e.yxy), julia(p + e.yyx) - julia(p - e.yyx)));
        // BUILD: back-light only (silhouette); otherwise key + head light
        vec3 ld = normalize(mix(vec3(0.5, 0.8, 0.4), -fw, build));
        float key = clamp(dot(n, ld), 0.0, 1.0);
        float head = clamp(dot(n, -rd), 0.0, 1.0);
        float fre = pow(1.0 - head, 3.0);
        float spec = pow(clamp(dot(reflect(rd, n), ld), 0.0, 1.0), 50.0);
        vec3 surf = ramp4(0.2 + 0.7 * clamp(trap * 1.5, 0.0, 1.0), C0, C1, C2, C3);
        col = surf * (0.05 + 0.7 * key + 0.25 * head * (1.0 - build));
        col += mix(C2, C3, 0.5) * fre * (0.3 + 0.6 * uHighMid + 1.2 * build);
        col += C3 * spec * (0.4 + 1.2 * uHigh + 0.6 * uHat);
        col += mix(C1, C2, 0.5) * smoothstep(0.3, 0.0, trap) * (0.08 + 0.9 * peak + 0.7 * uKick);
    }
    col += mix(C1, C2, 0.5) * glow * (0.3 + 0.8 * uMacro.y) * (0.4 + 0.8 * peak + 0.7 * uKick);

    col *= 1.0 - 0.1 * calm;
    col *= smoothstep(2.4, 0.6, length(uv * vec2(0.8, 1.0)));      // soft vignette: the frame stays full on a big screen
    col *= uIntensity * 1.4 * mix(0.6, 1.0, uActivity);
    col *= 0.48;                                   // exposure matched to the other scenes (consistency pass)
    fragColor = vec4(col, 1.0);
}
