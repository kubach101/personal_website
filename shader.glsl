#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;
out vec3 normal;
out vec4 color;
uniform mat4 uMVP;
uniform mat3 uNormMat;
void main(){
    gl_Position = uMVP * vec4(aPos, 1.0);
    normal = normalize(uNormMat * aNorm);
}


#version 330 core
in vec3 normal;
out vec4 fragCol;
uniform vec4 uCol;
uniform vec3 uLdir;
void main(){

    vec3 minCol = 0.3*uCol;
    float diff = max(dot(normal, uLdir), 0.2);
    fragCol = max(uCol*diff, minCol);
}