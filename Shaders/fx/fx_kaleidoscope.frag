// KALEIDOSCOPE — Amount: mix · P2: segments (2..16)
uniform float uAmt, uP2;
void main()
{
    vec2 uv = screenUV();
    vec2 p = uv - 0.5; p.x *= uRes.x / uRes.y;
    float n = floor(mix(2.0, 16.0, uP2) + 0.5);
    float sa = TAU / n;
    float a = mod(atan(p.y, p.x) + uTime * 0.05, sa);
    a = abs(a - 0.5 * sa);
    vec2 q = length(p) * vec2(cos(a), sin(a));
    q.x *= uRes.y / uRes.x;
    vec3 k = texture(uTex, 0.5 + q).rgb;
    fragColor = vec4(mix(texture(uTex, uv).rgb, k, uAmt), 1.0);
}
