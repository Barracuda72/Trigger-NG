#version 100
precision mediump float;
varying vec2 tex_position;
varying vec4 color;

uniform sampler2D font;

void main() {
  gl_FragColor = texture2D(font, tex_position) * color;
}
