attribute vec2 tex_coord;
attribute vec3 normal;
attribute vec3 position;

uniform mat4 mv;
uniform mat4 p;

varying vec2 tex_position;

void main() {
  tex_position = tex_coord;
  gl_Position = p * mv * vec4(position + normal * 0.000001, 1.0); // TODO: use normal to calculate lighting. Now we just add it into equation
  // so compiler doesn't throw it away.
}
