// VIGNETTE — Amount: darkness · P2: softness
uniform float uAmt, uP2;
void main()
{
    vec2 uv = screenUV();
    vec2 p = (uv - 0.5) * vec2(uRes.x / uRes.y, 1.0);
    float inner = mix(0.85, 0.12, uAmt);
    float outer = inner + mix(0.04, 0.9, uP2);
    float v = 1.0 - smoothstep(inner, outer, length(p));
    fragColor = vec4(texture(uTex, uv).rgb * mix(1.0, v, uAmt), 1.0);
}
