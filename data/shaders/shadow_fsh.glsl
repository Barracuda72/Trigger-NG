uniform sampler2D shadow;

varying vec2 tex_position;

void main() {
  gl_FragColor = texture2D(shadow, tex_position);
}
