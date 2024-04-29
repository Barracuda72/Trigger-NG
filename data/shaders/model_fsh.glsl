struct Material {
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
  float shininess;
};

struct Light {
  vec3 position;
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

uniform sampler2D tex;
uniform Material material;
uniform Light light;

varying vec2 tex_position;
varying vec3 l_position;
varying vec3 l_normal;

void main() {
  vec4 t_color = texture2D(tex, tex_position);

  vec3 ambient = light.ambient * material.ambient * t_color.rgb;

  vec3 result = ambient + (l_normal + l_position)*0.0001;

  gl_FragColor = vec4(result, t_color.a);
}
