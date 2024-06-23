#version 100
attribute vec2 position;

varying vec2 tex_position;

uniform mat4 tex_transform;
uniform mat4 mv;
uniform mat4 p;

void main() {
  tex_position = (tex_transform * vec4(position, 0.0, 1.0)).xy;
  gl_Position = p * mv * vec4(position, 0.0, 1.0);
}
