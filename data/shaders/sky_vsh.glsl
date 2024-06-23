#version 100
attribute vec2 tex_coord;
attribute vec3 vert_coord;

varying vec2 tex_transformed;
varying float fog_factor;

uniform float fog_density;
uniform mat4 t_transform;
uniform mat4 mv;
uniform mat4 p;

void main()
{
  tex_transformed = (t_transform * vec4(tex_coord, 0.0, 1.0)).xy;
  vec4 position = mv * vec4(vert_coord, 1.0);
  gl_Position = p * position;

  float fog_distance = length(position.xyz);
	fog_factor = exp( -fog_density * fog_distance );
	fog_factor = clamp(fog_factor, 0.0, 1.0);
}
