attribute vec2 tex_coord;
attribute vec3 position;

varying vec2 tex_position;

void main() {
  tex_position = (gl_TextureMatrix[0] * vec4(tex_coord, 0.0, 1.0)).xy;
  gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * vec4(position, 1.0);
}
