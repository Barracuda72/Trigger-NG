attribute vec2 position;

varying vec2 tex_position;

uniform mat4 tex_transform;
uniform mat4 mv;

void main() {
  tex_position = (tex_transform * vec4(position, 0.0, 1.0)).xy;
  gl_Position = gl_ProjectionMatrix * mv * vec4(position, 0.0, 1.0);
}
