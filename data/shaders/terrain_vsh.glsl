#version 100
attribute vec2 tex_coord;
attribute vec3 position;

varying vec2 tex_position;
varying float fog_factor;

uniform mat4 mv;
uniform mat4 p;
uniform float fog_density;

void main() {
  //tex_position = (gl_TextureMatrix[0] * vec4(tex_coord, 0.0, 1.0)).xy;
  tex_position = tex_coord;

  vec4 wo = vec4(position, 1.0);

  float fog_distance = length((mv * wo).xyz);
  fog_factor = exp( -fog_density * fog_distance );
  fog_factor = clamp(fog_factor, 0.0, 1.0);
  
  gl_Position = p * mv * wo;
}
