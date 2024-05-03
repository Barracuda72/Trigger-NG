precision mediump float;
varying float v_alpha;

uniform vec4 v_color;

void main() {
  gl_FragColor = vec4(v_color.rgb, v_color.a * v_alpha);
}
