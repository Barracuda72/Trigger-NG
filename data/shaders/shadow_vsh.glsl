attribute vec2 tex_coord;
attribute vec3 position;

varying vec2 tex_position;

void main() {
  tex_position = tex_coord;
  gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * vec4(position, 1.0);
}
