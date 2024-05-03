precision mediump float;
uniform sampler2D sign_tex;

varying vec2 tex_position;

uniform float alpha;

void main() {
  vec4 color = texture2D(sign_tex, tex_position);
  gl_FragColor = vec4(color.xyz, alpha * color.a);
}
