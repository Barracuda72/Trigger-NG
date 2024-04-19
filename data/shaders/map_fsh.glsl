uniform sampler2D map_texture;

varying vec2 tex_position;

void main() {
    gl_FragColor = vec4(texture2D(map_texture, tex_position).rgb, 0.7);
}
