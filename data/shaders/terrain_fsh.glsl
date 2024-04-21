uniform sampler2D image;

varying vec2 tex_position;

void main() {
  gl_FragColor = texture2D(image, tex_position);
}
