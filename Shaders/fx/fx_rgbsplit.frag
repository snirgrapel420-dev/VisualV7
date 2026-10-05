// RGB SPLIT — Amount: offset · P2: angle
uniform float uAmt, uP2;
void main()
{
    vec2 uv = screenUV();
    float a = uP2 * TAU;
    vec2 o = vec2(cos(a), sin(a)) * uAmt * 0.03 * vec2(uRes.y / uRes.x, 1.0);
    fragColor = vec4(texture(uTex, uv + o).r, texture(uTex, uv).g, texture(uTex, uv - o).b, 1.0);
}
