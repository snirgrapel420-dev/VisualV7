// ============================================================================
//  IMAGE TEMPLATE LAYER — turns an analysed image ("DNA") into generative
//  structure. DNA texture: R = luminance, G = edge strength, B = distance-to-
//  edge field, A = presence (foreground vs. background). uImgColor = source.
// ============================================================================
uniform sampler2D uDNA;
uniform sampler2D uImgColor;
uniform float uImgAspect;

uniform int   uTMode;        // 0 Kaleido 1 Mandala 2 Tunnel 3 Recursive 4 Rotating 5 Organic 6 Echo
uniform float uTScale, uTAngle, uTSym, uTSymCount, uTMirror, uTKaleido;
uniform float uTWarp, uTTwist, uTNoise, uTDistortion, uTFeedback, uTRecursion;
uniform float uTEdge, uTThreshold, uTLuminance, uTColorExtract, uTColorAmount;
uniform float uTDetail, uTComplexity, uTDepth, uTMotion, uTReact;

vec2 imageUV(vec2 q)
{
    vec2 size = uImgAspect >= 1.0 ? vec2(1.0, 1.0 / uImgAspect) : vec2(uImgAspect, 1.0);
    vec2 uv = q / size;
    return vec2(uv.x + 0.5, 0.5 - uv.y);
}

vec4 dnaSample(vec2 q, float layerPhase)
{
    vec2 uv = imageUV(q);
    if (uv.x < 0.0 || uv.y < 0.0 || uv.x > 1.0 || uv.y > 1.0) return vec4(0.0);

    vec4 dna = texture(uDNA, uv);
    vec3 src = texture(uImgColor, uv).rgb;

    float soft  = mix(0.18, 0.01, uTDetail);
    float thr   = uTThreshold;
    float val   = mix(dna.a, dna.r, uTLuminance);
    float fill  = smoothstep(thr - soft, thr + soft, val);
    float edge  = pow(clamp(dna.g * (1.0 + 3.0 * uTDetail), 0.0, 1.0), mix(1.5, 0.6, uTDetail));
    float shape = mix(fill, edge, uTEdge);

    // contour lines from the distance field — complexity adds iso-rings
    float rings = uTComplexity * exp(-(0.5 - abs(fract(dna.b * mix(4.0, 18.0, uTComplexity) - layerPhase) - 0.5)) * 18.0)
                * smoothstep(0.0, 0.05, dna.b) * (1.0 - fill * 0.7);
    shape = max(shape, rings * 0.8);

    // grain
    shape *= 1.0 - uTNoise * hash12(floor(q * 400.0) + floor(uAbsTime * 24.0)) * 0.8;

    vec3 palCol = palette(val * 0.8 + dna.b * 1.5 + layerPhase * 0.25 + uCentroid * 0.2);
    vec3 imgCol = src * 1.2;
    vec3 c = mix(palCol, imgCol, uTColorExtract);
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, uTColorAmount);
    return vec4(c * shape, shape);
}

// structural transform per mode, returns image-space coordinate
vec2 structure(vec2 p, float layer)
{
    float r = length(p);
    float a = atan(p.y, p.x);
    a += uTTwist * r * 4.0;

    float n  = max(1.0, floor(uTSymCount + 0.5));
    float sa = TAU / n;

    if (uTMode == 1) // Mandala: angular fold + logarithmic ring repetition
    {
        float fa = mod(a, sa);
        if (uTKaleido > 0.5) fa = abs(fa - 0.5 * sa);
        a = mix(a, fa, uTSym);
        float lr = log(max(r, 1e-3)) * mix(1.0, 3.0, uTComplexity) - uTMotion;
        r = (0.5 - abs(fract(lr) - 0.5)) * 0.9;
    }
    else if (uTMode == 2) // Tunnel: polar unwrap with depth travel
    {
        float u = a / TAU * n;
        float v = 0.25 / max(r, 1e-3) + uTMotion;
        vec2  t = vec2(fract(u) - 0.5, fract(v) - 0.5);
        if (uTKaleido > 0.5) t.x = abs(t.x) * 2.0 - 0.5;
        return t * 0.95;
    }
    else if (uTMode == 4) // Rotating geometry: n rotated copies, no fold
    {
        return p * rot(layer * sa + uTMotion * (mod(layer, 2.0) * 2.0 - 1.0));
    }
    else if (uTMode != 6) // Kaleido / Recursive / Organic use the fold
    {
        float fa = mod(a, sa);
        if (uTKaleido > 0.5) fa = abs(fa - 0.5 * sa);
        a = mix(a, fa, uTSym);
    }

    vec2 q = r * vec2(cos(a), sin(a));
    if (uTMirror > 0.5) q.x = abs(q.x);
    return q;
}

void main()
{
    vec2 p = centered();
    float pulse = 1.0 + (0.25 * uBass + 0.15 * uKick) * uTReact;
    p /= max(uTScale * pulse, 0.05);
    p *= rot(uTAngle);

    // warp & distortion (domain deformation)
    float wt = uTime * 0.2;
    p += uTWarp * 0.25 * (vec2(fbm(p * 2.0 + wt, 4), fbm(p * 2.0 + 7.3 - wt, 4)) - 0.5) * (1.0 + uMid * uTReact);
    p += uTDistortion * 0.08 * vec2(sin(p.y * 12.0 + uTime * 1.7), sin(p.x * 10.0 - uTime * 1.3));

    if (uTMode == 5) // Organic: heavy fluid-like domain warping
    {
        vec2 q = vec2(fbm(p * 1.5 + wt, 5), fbm(p * 1.5 + 3.1 - wt, 5));
        p += (q - 0.5) * (0.6 + 0.6 * uBass * uTReact);
    }

    int layers = 1 + int(uTRecursion * 5.0 + 0.5);
    if (uTMode == 4) layers = max(layers, int(max(1.0, floor(uTSymCount + 0.5))));
    layers = min(layers, 12);

    vec4 acc = vec4(0.0);
    for (int i = 0; i < 12; ++i)
    {
        if (i >= layers) break;
        float fi = float(i);
        vec2  lp = p;
        float w  = 1.0;
        if (uTMode != 4)
        {
            float s = pow(mix(1.0, 0.55, uTDepth), fi);
            lp = p / s * rot(fi * 0.35 * uTRecursion + fi * uTMotion * 0.2);
            w  = pow(mix(1.0, 0.6, uTDepth), fi);
        }
        vec4 smp = dnaSample(structure(lp, fi), fi * 0.15 + uTMotion * 0.1);
        acc = max(acc, smp * w);
    }

    // feedback: zoom-rotate echo of the template layer, burst on transients
    vec2 f = screenUV() - 0.5;
    f = rot(0.012) * f * (0.97 - 0.02 * uBass);
    vec4 prev = texture(uPrev, 0.5 + f);
    float fb = clamp(uTFeedback * (1.0 + 0.6 * uTransient * uTReact), 0.0, 0.985);
    if (uTMode == 6) fb = max(fb, 0.78);
    acc = max(acc, prev * fb);

    fragColor = vec4(clamp(acc.rgb * (1.0 + 0.5 * uHigh * uTReact), 0.0, 8.0), clamp(acc.a, 0.0, 1.0));
}
