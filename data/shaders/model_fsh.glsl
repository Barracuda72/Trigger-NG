precision mediump float;
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

  // Ambient
  vec3 ambient = light.ambient * material.ambient * t_color.rgb;

  // Diffuse
  vec3 v_normal = normalize(l_normal);
  vec3 v_light = normalize(light.position - l_position);
  float d_coeff = max(dot(v_normal, v_light), 0.0);
  vec3 diffuse = d_coeff * light.diffuse * material.diffuse * t_color.rgb;

  // Specular
  vec3 v_view = normalize(-l_position);
  vec3 v_reflected = reflect(-v_light, v_normal);
  float s_coeff = pow(max(dot(v_view, v_reflected), 0.0), material.shininess);
  vec3 specular = s_coeff * light.specular * material.specular * t_color.rgb;

  // Combined
  vec3 result = ambient + diffuse + specular;

  gl_FragColor = vec4(result, t_color.a);
}
