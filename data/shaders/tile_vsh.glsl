#version 100
attribute vec3 position;

uniform vec3 tex_gen_parameters;
uniform mat4 mv;
uniform mat4 p;
uniform float fog_density;

varying vec2 tex_position;
varying vec2 det_position;
varying float fog_factor;

void main() {
  vec4 wo = vec4(position, 1.0);
  float t_x = dot(wo, vec4(tex_gen_parameters.z, 0.0, 0.0, tex_gen_parameters.x));
  float t_y = dot(wo, vec4(0.0, tex_gen_parameters.z, 0.0, tex_gen_parameters.y));
  tex_position = vec2(t_x, t_y);
  det_position = wo.xy * 0.05;
  float fog_distance = length((mv * wo).xyz);
  fog_factor = exp( -fog_density * fog_distance );
  fog_factor = clamp(fog_factor, 0.0, 1.0);
  gl_Position = p * mv * wo;
}
