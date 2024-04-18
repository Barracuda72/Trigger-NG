attribute vec2 tex_coord;
attribute vec3 vert_coord;

varying vec2 tex_transformed;

uniform mat4 t_transform;

void main()
{
    tex_transformed = (t_transform * vec4(tex_coord, 0.0, 1.0)).xy;
    gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * vec4(vert_coord, 1.0);
}
