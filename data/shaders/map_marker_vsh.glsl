attribute vec2 position;
attribute float alpha;

varying float v_alpha;

uniform mat4 mv;

void main()
{
  v_alpha = alpha;
  gl_Position = gl_ProjectionMatrix * mv * vec4(position, 0.0, 1.0);
}
