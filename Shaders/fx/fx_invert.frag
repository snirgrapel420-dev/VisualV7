// INVERT — Amount: mix · P2: full ↔ luminance-only invert
uniform float uAmt, uP2;
void main()
{
    vec3 c = clamp(texture(uTex, screenUV()).rgb, 0.0, 1.0);
    vec3 full = 1.0 - c;
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    vec3 lumInv = c + (1.0 - 2.0 * l);
    fragColor = vec4(mix(c, mix(full, clamp(lumInv, 0.0, 1.0), uP2), uAmt), 1.0);
}
