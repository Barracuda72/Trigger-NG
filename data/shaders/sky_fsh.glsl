uniform sampler2D tex;

varying vec2 tex_transformed;

void main() {
    gl_FragColor = texture2D(tex, tex_transformed);
}
