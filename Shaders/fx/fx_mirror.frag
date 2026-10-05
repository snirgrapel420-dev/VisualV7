// MIRROR — Amount: mix · P2: mode (horizontal / vertical / quad)
uniform float uAmt, uP2;
void main()
{
    vec2 uv = screenUV();
    vec2 m = uv;
    if (uP2 < 0.34)      m.x = 0.5 - abs(uv.x - 0.5);
    else if (uP2 < 0.67) m.y = 0.5 - abs(uv.y - 0.5);
    else                 m = 0.5 - abs(uv - 0.5);
    fragColor = vec4(mix(texture(uTex, uv).rgb, texture(uTex, m).rgb, uAmt), 1.0);
}
