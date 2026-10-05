// TWIST — Amount: strength · P2: radius
uniform float uAmt, uP2;
void main()
{
    vec2 p = screenUV() - 0.5; p.x *= uRes.x / uRes.y;
    float r = length(p);
    float rad = mix(0.15, 1.2, uP2);
    float k = smoothstepR(rad, 0.0, r) * uAmt * 6.0 * (1.0 + 0.5 * uBass);
    p = rot(k) * p;
    p.x *= uRes.y / uRes.x;
    fragColor = vec4(texture(uTex, 0.5 + p).rgb, 1.0);
}
