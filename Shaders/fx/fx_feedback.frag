// FEEDBACK — Amount: persistence · P2: zoom / rotate (0.5 = static)
uniform float uAmt, uP2;
void main()
{
    vec2 uv = screenUV();
    vec3 cur = texture(uTex, uv).rgb;
    float z = (uP2 - 0.5);
    vec2 f = rot(z * 0.06) * (uv - 0.5) * (1.0 - z * 0.06);
    vec3 prev = texture(uPrev, 0.5 + f).rgb;
    fragColor = vec4(max(cur, prev * uAmt * 0.97), 1.0);
}
