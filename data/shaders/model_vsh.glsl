#version 100
attribute vec2 tex_coord;
attribute vec3 normal;
attribute vec3 position;

uniform mat3 n_mv;
uniform mat4 mv;
uniform mat4 p;

varying vec2 tex_position;
varying vec3 l_position;
varying vec3 l_normal;

void main() {
  tex_position = tex_coord;
  l_position = (mv * vec4(position, 1.0)).xyz;
  l_normal = n_mv * normal;
  gl_Position = p * mv * vec4(position, 1.0); 
}
