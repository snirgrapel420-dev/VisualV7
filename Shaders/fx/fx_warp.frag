// WARP — Amount: strength · P2: scale
uniform float uAmt, uP2;
void main()
{
    vec2 uv = screenUV();
    float s = mix(1.5, 12.0, uP2);
    vec2 d = vec2(fbm(uv * s + uTime * 0.2, 4), fbm(uv * s + 5.2 - uTime * 0.17, 4)) - 0.5;
    fragColor = vec4(texture(uTex, uv + d * uAmt * 0.12).rgb, 1.0);
}
