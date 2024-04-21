attribute vec2 tex_coord;
attribute vec3 position;

varying vec2 tex_position;
varying vec4 color;

uniform mat4 mv;

void main() {
  tex_position = tex_coord;
  color = gl_Color;
  gl_Position = gl_ProjectionMatrix * mv * vec4(position, 1.0);
}
