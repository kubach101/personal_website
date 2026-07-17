#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;
out vec3 normal;
uniform mat4 uMVP;
uniform mat3 uNormMat;
uniform float uTideConst;
uniform vec3 uFDir;
uniform float uRadius;
void main(){
    normal = normalize(uNormMat * aNorm);
    float cos_gamma = dot(uFDir, normal);
    float tide = uTideConst * (3 * cos_gamma * cos_gamma - 1);
    vec3 position = tide * normal + (aPos * uRadius);
    gl_Position = uMVP * vec4(position, 1.0);
}