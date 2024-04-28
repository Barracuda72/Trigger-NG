uniform sampler2D particle;

varying vec2 tex_position;
varying vec4 v_color;

void main() {
  vec4 t_color = texture2D(particle, tex_position);
  gl_FragColor = t_color * v_color;
}
