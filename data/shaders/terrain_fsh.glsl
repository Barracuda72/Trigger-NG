#version 100
precision mediump float;
uniform sampler2D image;

varying vec2 tex_position;

void main() {
  vec4 t_color = texture2D(image, tex_position);
  if (t_color.a < 0.5) // TODO: not a proper alpha test but still better than nothing
    discard;
  gl_FragColor = t_color;
}
