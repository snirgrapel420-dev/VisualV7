// SATURATION — Amount: saturation (0.5 = neutral) · P2: vibrance
uniform float uAmt, uP2;
void main()
{
    vec3 c = texture(uTex, screenUV()).rgb;
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    float mx = max(c.r, max(c.g, c.b)), mn = min(c.r, min(c.g, c.b));
    float sat = mx - mn;
    float s = uAmt * 2.0 * (1.0 + uP2 * (1.0 - sat));
    fragColor = vec4(max(mix(vec3(l), c, s), 0.0), 1.0);
}
