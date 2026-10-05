// POSTERIZATION — Amount: strength (fewer levels) · P2: gamma
uniform float uAmt, uP2;
void main()
{
    vec3 c = texture(uTex, screenUV()).rgb;
    float levels = mix(16.0, 2.0, uAmt);
    float g = mix(0.5, 2.0, uP2);
    vec3 q = pow(floor(pow(clamp(c, 0.0, 1.0), vec3(g)) * levels + 0.5) / levels, vec3(1.0 / g));
    fragColor = vec4(mix(c, q, step(0.001, uAmt)), 1.0);
}
