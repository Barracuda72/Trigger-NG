#version 100
attribute vec2 position;
attribute float alpha;

varying float v_alpha;

uniform mat4 mv;
uniform mat4 p;

void main()
{
  v_alpha = alpha;
  gl_Position = p * mv * vec4(position, 0.0, 1.0);
}
