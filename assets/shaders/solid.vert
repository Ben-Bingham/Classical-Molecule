#version 330 core

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUv;

uniform mat4 mvp;

void main() {
   gl_Position = mvp * vec4(inPos.x, inPos.y, inPos.z, 1.0);
}
