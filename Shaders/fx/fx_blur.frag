// BLUR — Amount: radius · P2: radial (zoom) blur mix
uniform float uAmt, uP2;
void main()
{
    vec2 uv = screenUV();
    float rad = uAmt * 12.0 / uRes.y;
    vec3 g = vec3(0.0); float wsum = 0.0;
    for (int i = -4; i <= 4; ++i)
    for (int j = -4; j <= 4; j += 2)
    {
        vec2 o = vec2(float(i), float(j) + 0.5 * float(i & 1)) * rad * 0.5;
        float w = exp(-dot(o, o) / (rad * rad * 4.0 + 1e-9));
        g += texture(uTex, uv + o * vec2(uRes.y / uRes.x, 1.0)).rgb * w; wsum += w;
    }
    g /= wsum;
    vec3 z = vec3(0.0);
    for (int i = 0; i < 12; ++i) z += texture(uTex, 0.5 + (uv - 0.5) * (1.0 - float(i) * uAmt * 0.01)).rgb;
    z /= 12.0;
    fragColor = vec4(mix(g, z, uP2), 1.0);
}
