// HUE SHIFT — Amount: hue offset · P2: continuous rotation speed
uniform float uAmt, uP2;
vec3 hueRot(vec3 c, float h)
{
    const vec3 k = vec3(0.57735);
    float a = h * TAU, ca = cos(a);
    return c * ca + cross(k, c) * sin(a) + k * dot(k, c) * (1.0 - ca);
}
void main()
{
    vec3 c = texture(uTex, screenUV()).rgb;
    fragColor = vec4(max(hueRot(c, uAmt + uTime * uP2 * 0.1), 0.0), 1.0);
}
