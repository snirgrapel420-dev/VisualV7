// MANDELBULB BLOOM — the 3D Mandelbrot set floating in space, orbited by the camera (raymarched).
//
// Self-driving: every part is automated from the music.
//   bass / sub    the bulb breathes; the orbit glides with the bass clock
//   kick          the fractal POWER jumps: the whole form blossoms outward and settles
//   snare         the orbit jolts to a new angle
//   mids          orbit speed and height; the power drifts (the form slowly re-grows)
//   highs         glints on the crests
//   BUILD         the camera pulls back, the light dims to a rim
//   PEAK          the deep folds glow from within
//   CHAOS         the power oscillates: the form writhes
//   colour        three colour families, the music moves between them
// Macros: A Power · B Glow · C Orbit Speed · D Colour Family

vec3 C0, C1, C2, C3;
void setPalette()
{
    vec3 w = familyWeights(colourFamily(uMacro.w));
    // A: space blue · copper · gold · ivory   B: violet night · magenta · coral · peach   C: black · teal · ice · white
    vec3 a0 = hex(197642.0),  a1 = hex(9126944.0),  a2 = hex(14267441.0), a3 = hex(16773603.0);
    vec3 b0 = hex(1180714.0), b1 = hex(11150214.0), b2 = hex(16740187.0), b3 = hex(16767165.0);
    vec3 c0 = hex(65793.0),   c1 = hex(1080718.0),  c2 = hex(10939390.0), c3 = hex(16777215.0);
    C0 = a0 * w.x + b0 * w.y + c0 * w.z; C1 = a1 * w.x + b1 * w.y + c1 * w.z;
    C2 = a2 * w.x + b2 * w.y + c2 * w.z; C3 = a3 * w.x + b3 * w.y + c3 * w.z;
}

float gTrap;
float gPower;

float de(vec3 p)
{
    vec3 z = p;
    float dr = 1.0, r = 0.0;
    gTrap = 1e9;
    for (int i = 0; i < 7; i++)
    {
        r = length(z);
        if (r > 2.0) break;
        float th = acos(clamp(z.z / r, -1.0, 1.0)) * gPower;
        float ph = atan(z.y, z.x) * gPower;
        dr = pow(r, gPower - 1.0) * gPower * dr + 1.0;
        z = pow(r, gPower) * vec3(sin(th) * cos(ph), sin(ph) * sin(th), cos(th)) + p;
        gTrap = min(gTrap, dot(z, z));
    }
    return 0.5 * log(max(r, 1e-6)) * r / dr;
}

void main()
{
    setPalette();
    vec2 uv = (gl_FragCoord.xy - 0.5 * uRes) / uRes.y;
    float build = uState.y, peak = uState.z, chaos = uState.w, calm = uState.x;

    gPower = 6.0 + 3.0 * uMacro.x + 1.2 * sin(uMidTime * 0.03) + 0.9 * uKick + 0.6 * chaos * sin(uHighTime * 1.5) + 0.7 * uMid;                // mids re-grow the form

    float orbit = uBassTime * (0.04 + 0.14 * uMacro.z) + uMidTime * 0.01 + 0.6 * uSnare;
    float dist = 3.3 + 0.7 * build - 0.2 * peak - 0.12 * uKick - 0.35 * uSub;
    vec3 ro = vec3(dist * sin(orbit), 0.6 * sin(orbit * 0.7 + uMidTime * 0.02), dist * cos(orbit));
    vec3 fw = normalize(-ro);
    vec3 rt = normalize(cross(vec3(0.0, 1.0, 0.0), fw));
    vec3 up = cross(fw, rt);
    vec3 rd = normalize(uv.x * rt + uv.y * up + 2.0 * fw);
    rd = roleCamera(rd);                       // sub / mids / bassline move the camera

    // space: a faint nebula and stars behind the form
    vec3 col = sceneBackdrop(rd, C0, C1, C2);                 // a living space around the object
    vec2 sg = rd.xy / (abs(rd.z) + 0.5) * 120.0;
    col += C3 * step(0.996, hash12(floor(sg))) * smoothstep(0.35, 0.0, length(fract(sg) - 0.5)) * 0.6;

    float t = 0.0, glow = 0.0;
    bool hit = false;
    for (int i = 0; i < 110; i++)
    {
        if (i >= stepBudget(110)) break;   // adaptive quality
        vec3 p = ro + rd * t;
        float d = de(p);
        glow += exp(-d * 40.0) * 0.01;
        if (d < 0.0006 * t) { hit = true; break; }
        t += d * 0.85;
        if (t > 7.0) break;
    }
    if (hit)
    {
        vec3 p = ro + rd * t;
        float trap = gTrap;
        vec2 e = vec2(0.0006 * t, 0.0);
        vec3 n = normalize(vec3(de(p + e.xyy) - de(p - e.xyy), de(p + e.yxy) - de(p - e.yxy), de(p + e.yyx) - de(p - e.yyx)));
        float ao = clamp(de(p + n * 0.03) / 0.03, 0.0, 1.0);
        vec3 kl = normalize(vec3(0.6, 0.7, -0.4));
        float key = clamp(dot(n, kl), 0.0, 1.0);
        float head = clamp(dot(n, -rd), 0.0, 1.0);
        float fre = pow(1.0 - head, 3.0);
        float spec = pow(clamp(dot(reflect(rd, n), kl), 0.0, 1.0), 40.0);
        float tr = clamp((sqrt(trap) - 0.6) * 1.1, 0.0, 1.0);                    // spread the depths across the ramp
        vec3 surf = ramp4(0.15 + 0.85 * tr, C0, C1, C2, C3);
        float lightLvl = 1.0 - 0.55 * build;
        col = surf * (0.03 + 0.6 * key * lightLvl + 0.12 * head) * (0.15 + 0.85 * ao * ao);
        col += C3 * spec * ao * (0.4 + 1.3 * uHigh + 0.6 * uHat);
        col += mix(C2, C3, 0.5) * fre * (0.25 + 0.6 * build + 0.4 * uHighMid);         // rim light (strong in BUILD)
        col += mix(C1, C2, 0.5) * (1.0 - ao) * (0.05 + 0.6 * peak * (0.4 + uLowMid) + 0.5 * uKick);  // deep folds glow
    }
    col += mix(C1, C2, 0.5) * glow * (0.2 + 0.8 * uMacro.y) * (0.3 + 0.7 * peak + 0.8 * uKick);

    col *= 1.0 - 0.1 * calm;
    col *= smoothstep(2.4, 0.6, length(uv * vec2(0.8, 1.0)));      // soft vignette: the frame stays full on a big screen
    col *= uIntensity * 1.05 * mix(0.6, 1.0, uActivity);
    fragColor = vec4(col, 1.0);
}
