precision mediump float;
uniform sampler2D dial;

varying vec2 tex_position;

void main() {
  gl_FragColor = texture2D(dial, tex_position);
}
