// Final output: musical dynamics (build tension / drop impact), global colour
// grade, soft highlight roll-off and dither.
uniform float uHue, uSaturation, uBrightness, uContrast;
uniform float uDynamics;     // 0..1 amount of build / drop treatment
uniform float uBloom;        // 0..1 bloom amount (uTex has mipmaps)
uniform float uSharpen;      // 0..1 detail sharpening (crisp fractal detail at 4K / when upscaling)
uniform float uAutoFX;       // 0..1 automatic camera / lens effects driven by the music (every scene)

vec3 hueRotate(vec3 c, float h)
{
    const vec3 k = vec3(0.57735);
    float a = h * TAU, ca = cos(a);
    return c * ca + cross(k, c) * sin(a) + k * dot(k, c) * (1.0 - ca);
}

vec3 softClip(vec3 c)
{
    vec3 x = max(c - 0.8, 0.0);
    return min(c, vec3(0.8)) + 0.2 * (1.0 - exp(-x / 0.2));
}

void main()
{
    float build = uBuild * uDynamics;
    float drop  = uDrop * uDynamics;

    // drop: zoom punch (image jumps towards the viewer and settles)
    vec2 uv = (vUV - 0.5) * (1.0 - 0.06 * drop * drop) + 0.5;
    // build: slow breathing tunnel pull that tightens as tension grows
    uv = (uv - 0.5) * (1.0 - 0.025 * build * (0.5 + 0.5 * sin(uAbsTime * (2.0 + 6.0 * build)))) + 0.5;
    // ---- AUTO FX: the camera and lens react to the music in every scene ----------------------
    float fx = uAutoFX;
    float energetic = uState.z + uState.w;
    // kick: the camera punches in, a little more when the track is driving
    uv = (uv - 0.5) * (1.0 - fx * 0.035 * uKick * (0.6 + 0.4 * energetic)) + 0.5;
    // snare / kick / chaos: radial chromatic split of the lens
    vec2 dir = uv - 0.5;
    float ca = fx * (0.0025 * uKick + 0.005 * uSnare + 0.004 * uState.w) * (0.5 + length(dir) * 1.5);
    // peak / chaos with energy: zoom blur - speed through the frame
    float zb = fx * (0.012 * uState.z + 0.03 * uState.w) * uEnergyMed + fx * 0.02 * uDrop;
    vec3 c = vec3(0.0);
    for (int i = 0; i < 6; i++)
    {
        float k = 1.0 - zb * float(i) / 5.0;
        vec2 u = (uv - 0.5) * k + 0.5;
        c += vec3(texture(uTex, u + dir * ca).r, texture(uTex, u).g, texture(uTex, u - dir * ca).b);
    }
    c /= 6.0;
    // ---- ROLE LAYER: hats sparkle on the bright edges, highs make fine detail shimmer, the bassline throbs
    {
        vec3 detail = c - textureLod(uTex, uv, 1.5).rgb;
        float lum = dot(c, vec3(0.299, 0.587, 0.114));
        float sparkle = smoothstep(0.12, 0.6, lum) * smoothstep(0.008, 0.08, length(detail));
        c += c * sparkle * (1.8 * uHat + 0.6 * uHighMid) * fx;
        c += detail * (1.2 * uHigh + 0.8 * uHat) * fx;
        c *= 1.0 + 0.07 * uBassNote * fx;
        c *= 1.0 + 0.14 * uSnare * fx;                     // the snare/clap: a short crack of light
    }
    // FXAA-style edge smoothing: fractal edges shimmer and look jagged without it
    {
        vec2 px = 1.0 / vec2(textureSize(uTex, 0));
        vec3 lw = vec3(0.299, 0.587, 0.114);
        float lC = dot(c, lw);
        float lN = dot(texture(uTex, uv + vec2(0.0, px.y)).rgb, lw), lS = dot(texture(uTex, uv - vec2(0.0, px.y)).rgb, lw);
        float lE = dot(texture(uTex, uv + vec2(px.x, 0.0)).rgb, lw), lW = dot(texture(uTex, uv - vec2(px.x, 0.0)).rgb, lw);
        float lMin = min(lC, min(min(lN, lS), min(lE, lW))), lMax = max(lC, max(max(lN, lS), max(lE, lW)));
        float contrast = lMax - lMin;
        if (contrast > max(0.04, lMax * 0.12))
        {
            // blur along the edge, not across it
            vec2 dir = normalize(vec2(-(lN - lS), lE - lW) + 1e-5);
            vec3 a1 = texture(uTex, uv + dir * px * 0.5).rgb + texture(uTex, uv - dir * px * 0.5).rgb;
            vec3 a2 = texture(uTex, uv + dir * px * 1.5).rgb + texture(uTex, uv - dir * px * 1.5).rgb;
            vec3 aa = mix(a1 * 0.5, (a1 + a2) * 0.25, 0.5);
            c = mix(c, aa, smoothstep(0.04, 0.25, contrast) * 0.8);
        }
    }
    // sharpening: add back the detail above a slightly blurred copy (mip chain), limited to avoid halos
    vec3 soft = textureLod(uTex, uv, 1.25).rgb;
    c += clamp(c - soft, -0.15, 0.15) * uSharpen * 1.6;
    // bloom: wide, soft glow gathered from the mip chain (thresholded so darks stay deep)
    float lodShift = log2(max(textureSize(uTex, 0).y, 1) / 1080.0);       // same glow size at 1080p and 4K
    vec3 b1 = textureLod(uTex, uv, 2.0 + lodShift).rgb, b2 = textureLod(uTex, uv, 3.5 + lodShift).rgb,
         b3 = textureLod(uTex, uv, 5.0 + lodShift).rgb, b4 = textureLod(uTex, uv, 6.5 + lodShift).rgb;
    vec3 bloom = b1 * 0.30 + b2 * 0.30 + b3 * 0.25 + b4 * 0.15;
    bloom = max(bloom - 0.12, 0.0) * 1.6;
    c += bloom * uBloom * (1.0 + 0.8 * drop);

    // colour drifts with the brightness of the sound and swirls a little in CHAOS
    c = hueRotate(c, uHue + fx * (0.03 * (uCentroidSlow - 0.5) + 0.04 * uState.w * sin(uAbsTime * 0.6)));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    // tension drains colour and focuses the frame; the drop floods it back over-saturated
    float sat = uSaturation * (1.0 - 0.45 * build) * (1.0 + 0.35 * drop);
    c = mix(vec3(l), c, sat);
    c *= uBrightness * (1.0 + 0.55 * drop * drop);
    c = (c - 0.5) * uContrast * (1.0 + 0.25 * build) + 0.5;
    // build vignette closes in; drop adds a short white flash
    vec2 p = vUV - 0.5;
    c *= 1.0 - build * 0.55 * smoothstep(0.15, 0.75, length(p * vec2(uRes.x / uRes.y, 1.0)));
    c += vec3(0.12 * pow(drop, 4.0)) * (0.3 + l);
    c = aces(max(c, 0.0) * 1.1);
    c += (hash12(gl_FragCoord.xy + fract(uAbsTime) * 91.0) - 0.5) / 255.0;
    fragColor = vec4(clamp(c, 0.0, 1.0), 1.0);
}
