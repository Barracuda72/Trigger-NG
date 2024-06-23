#version 100
attribute vec2 tex_coord;
attribute vec4 color;
attribute vec3 normal;
attribute vec3 position;

varying vec2 tex_position;
varying float alpha;

uniform mat4 mv;
uniform mat4 p;

void main() {
  tex_position = tex_coord;
  alpha = color.a;
  gl_Position = p * mv * vec4(position + normal*0.00001, 1.0); // Normal is mixed to prevent it being thrown out by GLSL compiler
}
