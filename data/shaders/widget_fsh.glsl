uniform sampler2D widget;
uniform vec4 color;

varying vec2 tex_position;

void main() {
  vec4 t_color = texture2D(widget, tex_position);
  gl_FragColor = t_color * color;
}
