attribute vec2 tex_coord;
attribute vec3 position;

varying vec2 tex_position;

uniform mat4 mv;
uniform mat4 p;

void main() {
  tex_position = tex_coord;
  gl_Position = p * mv * vec4(position, 1.0);
}
