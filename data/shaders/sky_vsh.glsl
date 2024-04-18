attribute vec2 tex_coord;
attribute vec3 vert_coord;

varying vec2 tex_transformed;
varying float fog_factor;

uniform mat4 t_transform;

void main()
{
    tex_transformed = (t_transform * vec4(tex_coord, 0.0, 1.0)).xy;
    vec4 position = gl_ModelViewMatrix * vec4(vert_coord, 1.0);
    gl_Position = gl_ProjectionMatrix * position;

    gl_FogFragCoord = length(position.xyz);
	fog_factor = exp( -gl_Fog.density * gl_FogFragCoord );
	fog_factor = clamp(fog_factor, 0.0, 1.0);
}
