// GLOW — Amount: strength · P2: threshold
uniform float uAmt, uP2;
void main()
{
    vec2 uv = screenUV();
    vec3 base = texture(uTex, uv).rgb;
    vec3 acc = vec3(0.0); float wsum = 0.0;
    for (int ring = 1; ring <= 3; ++ring)
    for (int k = 0; k < 8; ++k)
    {
        float a = float(k) * TAU / 8.0 + float(ring) * 0.4;
        vec2  o = vec2(cos(a), sin(a)) * float(ring) * 7.0 / uRes;
        vec3  s = texture(uTex, uv + o).rgb;
        float w = 1.0 / float(ring);
        acc += max(s - uP2, 0.0) * w; wsum += w;
    }
    fragColor = vec4(base + acc / wsum * uAmt * 3.0, 1.0);
}
