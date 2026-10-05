// NOISE — Amount: grain · P2: grain size
uniform float uAmt, uP2;
void main()
{
    vec2 uv = screenUV();
    vec3 c = texture(uTex, uv).rgb;
    float sz = mix(1.0, 6.0, uP2);
    float n = hash12(floor(gl_FragCoord.xy / sz) + floor(uAbsTime * 30.0) * 13.7) - 0.5;
    float l = dot(c, vec3(0.333));
    fragColor = vec4(max(c + n * uAmt * (0.25 + 0.5 * l), 0.0), 1.0);
}
