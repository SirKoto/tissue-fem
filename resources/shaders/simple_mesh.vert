#version 430 core
layout(location = 0) in vec3 iPos;
layout(location = 1) in vec3 iNorm;

layout(location = 0) uniform mat4 M;
layout(location = 1) uniform mat3 NormalMat;
layout(location = 2) uniform mat4 PV;

out vec3 worldPos;

void main() {
    vec4 world = M * vec4(iPos, 1.0);
    worldPos = world.xyz;
    gl_Position = PV * world;
}
