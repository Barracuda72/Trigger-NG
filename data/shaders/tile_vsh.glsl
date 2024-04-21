attribute vec3 position;

uniform vec3 tex_gen_parameters;

varying vec2 tex_position;
varying vec2 det_position;

void main() {
  vec4 wo = vec4(position, 1.0);
  float t_x = dot(wo, vec4(tex_gen_parameters.z, 0.0, 0.0, tex_gen_parameters.x));
  float t_y = dot(wo, vec4(0.0, tex_gen_parameters.z, 0.0, tex_gen_parameters.y));
  tex_position = vec2(t_x, t_y);
  det_position = wo.xy * 0.05;
  gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * wo;
}
