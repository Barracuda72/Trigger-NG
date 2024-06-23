#version 100
precision mediump float;
uniform sampler2D shadow;

varying vec2 tex_position;

void main() {
  vec4 color = texture2D(shadow, tex_position);
  gl_FragColor = vec4(color.rgb, color.a * 0.7);
}
