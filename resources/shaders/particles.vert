#version 430 core

layout(location = 0) in vec3 iVert;
layout(location = 1) in vec3 iNorm;
layout(location = 2) in vec3 iOffset;

out vec3 normWorld;

layout(location = 0) uniform mat4 M;
layout(location = 1) uniform mat3 NormalMat;
layout(location = 2) uniform mat4 PV;
layout(location = 3) uniform float Radius;

void main() {
    normWorld = normalize( NormalMat * iNorm );
    gl_Position = PV * M * vec4(iVert * Radius + iOffset, 1.0);
}
