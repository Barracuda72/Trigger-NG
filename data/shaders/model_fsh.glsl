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

void main() {
  gl_FragColor = texture2D(tex, tex_position);
}
