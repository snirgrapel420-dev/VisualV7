// BRIGHTNESS — Amount: gain (0.5 = neutral) · P2: lift shadows
uniform float uAmt, uP2;
void main()
{
    vec3 c = texture(uTex, screenUV()).rgb;
    c *= uAmt * 2.0;
    c += uP2 * 0.15 * (1.0 - clamp(c, 0.0, 1.0));
    fragColor = vec4(c, 1.0);
}
