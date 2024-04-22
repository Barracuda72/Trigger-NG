uniform sampler2D tex;

varying vec2 tex_position;

void main() {
  gl_FragColor = texture2D(tex, tex_position);
}
