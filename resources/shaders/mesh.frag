#version 430 core

out vec4 fragColor;

in vec3 normWorld;

layout(location = 3) uniform vec3 uColor;

const vec3 light_dir = vec3(0.57735, -0.57735, -0.57735);

void main() {
    vec3 color = uColor * (0.2 + 0.8 * max(dot(normWorld, -light_dir), 0.0));
    fragColor = vec4(color, 1.0);
}
