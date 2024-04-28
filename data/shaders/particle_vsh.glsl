attribute vec2 tex_coord;
attribute vec4 color;
attribute vec3 normal;
attribute vec3 position;

varying vec2 tex_position;
varying vec4 v_color;

uniform mat4 mv;
uniform mat4 p;

void main() {
  tex_position = tex_coord;
  v_color = color;
  gl_Position = p * mv * vec4(position + 0.0001*normal, 1.0);
}
