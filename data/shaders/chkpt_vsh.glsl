attribute vec4 d_color; // Only alpha value is used
attribute vec3 normal;  // Unused, should be 0
attribute vec3 position;

varying vec4 v_color;

uniform vec4 color;
uniform mat4 mv;

void main() {
    // Normal is mixed so it is considered "used" by shader compiler
    v_color = vec4(color.rgb + 0.00001 * normal, d_color.a * color.a); 
    gl_Position = gl_ProjectionMatrix * mv * vec4(position, 1.0);
}
