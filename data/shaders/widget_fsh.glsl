uniform sampler2D widget;

varying vec2 tex_position;

void main() {
  gl_FragColor = texture2D(widget, tex_position);
}
