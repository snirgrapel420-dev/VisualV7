// CHROMATIC ABERRATION — Amount: strength · P2: radial ↔ lateral
uniform float uAmt, uP2;
void main()
{
    vec2 uv = screenUV();
    vec2 d  = uv - 0.5;
    vec2 dir = mix(d * length(d) * 2.0, vec2(0.5 * length(d) + 0.1, 0.0), uP2);
    vec2 o  = dir * uAmt * 0.06;
    float r = texture(uTex, uv + o).r;
    float g = texture(uTex, uv).g;
    float b = texture(uTex, uv - o).b;
    fragColor = vec4(r, g, b, 1.0);
}
