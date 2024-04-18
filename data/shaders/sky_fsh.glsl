uniform sampler2D tex;

varying vec2 tex_transformed;
varying float fog_factor;

void main() {
    vec3 color = texture2D(tex, tex_transformed).rgb;
    color = pow(color, vec3(1.0/2.2));

    gl_FragColor = vec4(mix(gl_Fog.color.rgb, color, fog_factor), 1.0);
}
