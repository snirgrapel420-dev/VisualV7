// Scene transition as a DIMENSIONAL PASSAGE: uTex = outgoing world, uLayer = incoming world.
//   1. FALL      the old world is pulled into a vortex: it zooms into the centre, twists into a spiral,
//                folds into kaleidoscopic symmetry and stretches into speed streaks
//   2. CROSSING  a tunnel of light rings and a flash at the centre - the moment between dimensions,
//                both worlds layered
//   3. EMERGE    the new world unfolds out of the kaleidoscope, untwisting the other way, and settles
uniform sampler2D uLayer;
uniform float uMix;

vec2 kfold(vec2 q, float n)
{
    float a = atan(q.y, q.x), r = length(q), seg = TAU / n;
    a = abs(mod(a, seg) - 0.5 * seg);
    return r * vec2(cos(a), sin(a));
}

// sample a world through the passage: zoom, spiral twist, kaleidoscope, zoom-blur streaks, chromatic split
vec3 passage(sampler2D tex, vec2 p, float asp, float zoom, float twist, float kal, float blur, float ca)
{
    vec3 acc = vec3(0.0);
    for (int k = 0; k < 6; k++)
    {
        float s = 1.0 - blur * float(k) / 5.0;
        vec2 q = p * zoom * s;
        float r = length(q);
        q = rot(twist * (1.2 - min(r, 1.2))) * q;                  // spiral: the centre turns most
        q = mix(q, kfold(q, 6.0), kal);                            // the world folds into symmetry
        vec2 dir = normalize(q + 1e-5) * ca;
        vec2 uvc = q / vec2(asp, 1.0) + 0.5;
        uvc = 1.0 - abs(1.0 - mod(uvc, 2.0));                      // mirrored: never runs off the edge
        vec2 uvr = 1.0 - abs(1.0 - mod(uvc + dir, 2.0)), uvb = 1.0 - abs(1.0 - mod(uvc - dir, 2.0));
        acc += vec3(texture(tex, uvr).r, texture(tex, uvc).g, texture(tex, uvb).b);
    }
    return acc / 6.0;
}

void main()
{
    vec2 uv = screenUV();
    float asp = uRes.x / uRes.y;
    vec2 p = (uv - 0.5) * vec2(asp, 1.0);
    float r = length(p);
    float m = uMix;

    float fall = smoothstep(0.0, 0.55, m);
    float emerge = smoothstep(0.45, 1.0, m);
    float f2 = fall * fall, e2 = (1.0 - emerge) * (1.0 - emerge);

    // 1. the old world falls into the vortex
    vec3 a = passage(uTex, p, asp, mix(1.0, 0.18, f2), 3.2 * f2, fall, 0.3 * fall, 0.012 * fall);
    // 3. the new world emerges, untwisting the other way
    vec3 b = passage(uLayer, p, asp, mix(0.18, 1.0, emerge), -3.2 * e2, 1.0 - emerge, 0.3 * (1.0 - emerge), 0.012 * (1.0 - emerge));

    float w = smoothstep(0.35, 0.65, m);
    vec3 col = mix(a, b, w);

    // 2. the crossing: rings of light rushing out of the centre, and a flash between the worlds
    float bump = sin(PI * smoothstep(0.25, 0.75, m));
    float rings = pow(0.5 + 0.5 * sin(log(max(r, 1e-3)) * 18.0 - m * 60.0), 18.0) * exp(-r * 2.2);
    vec3 tint = 0.5 * (a + b) + 0.15;
    col += tint * rings * bump * 0.55;
    col += mix(tint, vec3(1.0), 0.5) * exp(-r * 7.0) * bump * bump * 0.9;
    // the edges darken while passing through, so the centre pulls the eye
    col *= 1.0 - 0.45 * bump * smoothstep(0.3, 1.0, r);
    fragColor = vec4(col, 1.0);
}
