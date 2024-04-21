uniform sampler2D tile;
uniform sampler2D detail;

varying vec2 tex_position;
varying vec2 det_position;

void main() {
  vec4 t = texture2D(tile, tex_position);
  vec4 d = texture2D(detail, det_position);
  gl_FragColor = vec4(t.rgb + d.rgb - 0.5, t.a * d.a);
}
