#version 100
precision mediump float;
varying vec2 tex_position;

uniform sampler2D tex;
uniform vec4 color;

void main() {
  gl_FragColor = color * texture2D(tex, tex_position);
}
