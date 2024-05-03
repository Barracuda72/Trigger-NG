precision mediump float;
uniform sampler2D snow;
uniform float alpha;

varying vec2 tex_position;

void main() {
  vec4 color = texture2D(snow, tex_position);
  gl_FragColor = vec4(color.rbg, alpha * color.a);
}
