#version 100
precision mediump float;
uniform sampler2D needle;

varying vec2 tex_position;

void main() {
  gl_FragColor = texture2D(needle, tex_position);
}
