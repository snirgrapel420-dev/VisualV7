// Composites the template layer over the scene. uTex = scene, uLayer = template.
uniform sampler2D uLayer;
uniform float uTMix;
uniform int   uTBlend;   // 0 Screen 1 Add 2 Mask 3 Replace

void main()
{
    vec2 uv = screenUV();
    vec3 s = texture(uTex, uv).rgb;
    vec4 t = texture(uLayer, uv);
    vec3 o;
    if      (uTBlend == 0) o = 1.0 - (1.0 - clamp(s, 0.0, 1.0)) * (1.0 - clamp(t.rgb, 0.0, 1.0)) + max(s - 1.0, 0.0);
    else if (uTBlend == 1) o = s + t.rgb;
    else if (uTBlend == 2) o = s * t.a * 1.6 + t.rgb * 0.25;
    else                   o = t.rgb + s * 0.15;
    fragColor = vec4(mix(s, o, uTMix), 1.0);
}
