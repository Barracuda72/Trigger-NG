varying vec2 tex_position;

uniform sampler2D offroad_sign;

void main() {
  gl_FragColor = texture2D(offroad_sign, tex_position);
}
