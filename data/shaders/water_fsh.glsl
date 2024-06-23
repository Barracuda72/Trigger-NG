#version 100
precision mediump float;
uniform sampler2D water;

varying vec2 tex_position;
varying float alpha;

void main() {
  vec4 color = texture2D(water, tex_position);
  gl_FragColor = vec4(color.rgb, alpha);
}
