#version 100
precision mediump float;
uniform sampler2D background;

varying vec2 tex_position;

void main() {
  gl_FragColor = texture2D(background, tex_position);
}
