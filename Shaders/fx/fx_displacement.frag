// DISPLACEMENT — Amount: depth · P2: luminance ↔ colour-gradient drive
uniform float uAmt, uP2;
void main()
{
    vec2 uv = screenUV();
    vec2 px = 2.0 / uRes;
    float l  = dot(texture(uTex, uv).rgb, vec3(0.333));
    float lx = dot(texture(uTex, uv + vec2(px.x, 0.0)).rgb, vec3(0.333));
    float ly = dot(texture(uTex, uv + vec2(0.0, px.y)).rgb, vec3(0.333));
    vec2 grad = vec2(lx - l, ly - l) * 12.0;
    vec2 lumDir = (vec2(l) - 0.5) * vec2(0.7, -0.4);
    vec2 d = mix(lumDir, grad, uP2);
    fragColor = vec4(texture(uTex, uv + d * uAmt * 0.08).rgb, 1.0);
}
