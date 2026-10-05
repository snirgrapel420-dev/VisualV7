// TRAILS — Amount: decay length · P2: colour drift of the trail
uniform float uAmt, uP2;
vec3 hueRot(vec3 c, float h)
{
    const vec3 k = vec3(0.57735);
    float a = h * TAU, ca = cos(a);
    return c * ca + cross(k, c) * sin(a) + k * dot(k, c) * (1.0 - ca);
}
void main()
{
    vec2 uv = screenUV();
    vec3 cur = texture(uTex, uv).rgb;
    vec3 prev = max(hueRot(texture(uPrev, uv).rgb, uP2 * 0.02), 0.0);
    fragColor = vec4(max(cur, prev * uAmt * 0.985), 1.0);
}
