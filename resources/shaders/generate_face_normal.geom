#version 430 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec3 worldPos[];
out vec3 normWorld;

void main() {
    
    vec3 p0 = worldPos[0], 
        p1 = worldPos[1],
        p2 = worldPos[2];

    normWorld = normalize(cross(p1 - p0, p2 - p0));
    
    gl_Position = gl_in[0].gl_Position;
    EmitVertex();
    gl_Position = gl_in[1].gl_Position;
    EmitVertex();
    gl_Position = gl_in[2].gl_Position;
    EmitVertex();
    EndPrimitive();
}
