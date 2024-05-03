precision mediump float;
uniform sampler2D tile;
uniform sampler2D detail;
uniform vec3 fog_color;

varying vec2 tex_position;
varying vec2 det_position;
varying float fog_factor;

void main() {
  vec4 t = texture2D(tile, tex_position);
  vec4 d = texture2D(detail, det_position);
  vec4 color = vec4(t.rgb + d.rgb - 0.5, t.a * d.a);
  gl_FragColor = vec4(mix(fog_color, color.rgb, fog_factor), color.a);
}
