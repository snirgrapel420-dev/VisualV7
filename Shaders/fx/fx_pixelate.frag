// PIXELATION — Amount: block size · P2: mosaic ↔ dot matrix
uniform float uAmt, uP2;
void main()
{
    float bs = floor(mix(1.0, 48.0, uAmt * uAmt)) ;
    vec2 cell = floor(gl_FragCoord.xy / bs);
    vec2 c = (cell + 0.5) * bs / uRes;
    vec3 col = texture(uTex, c).rgb;
    vec2 f = fract(gl_FragCoord.xy / bs) - 0.5;
    float dotMask = smoothstepR(0.5, 0.35, length(f));
    fragColor = vec4(col * mix(1.0, dotMask * 1.3, uP2 * step(2.0, bs)), 1.0);
}
