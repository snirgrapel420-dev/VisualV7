// CONTRAST — Amount: contrast (0.5 = neutral) · P2: pivot
uniform float uAmt, uP2;
void main()
{
    vec3 c = texture(uTex, screenUV()).rgb;
    float k = pow(4.0, uAmt * 2.0 - 1.0);
    fragColor = vec4(max((c - uP2) * k + uP2, 0.0), 1.0);
}
