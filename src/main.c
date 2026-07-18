#include <GLES3/gl3.h>
#include <stdio.h>
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <float.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

const char *CrystalVertexShader =
    "#version 300 es\n"
    "layout(location = 0) in vec3 position;\n"
    "layout(location = 1) in vec3 normal;\n"
    "uniform mat4 modelViewMatrix;\n"
    "uniform mat4 projectionMatrix;\n"
    "uniform mat3 normalMatrix;\n"
    "out vec3 vNormal;\n"
    "out vec3 vViewPosition;\n"
    "void main() {\n"
    "  vNormal = normalize(normalMatrix * normal);\n"
    "  vec4 mvPosition = modelViewMatrix * vec4(position, 1.0);\n"
    "  vViewPosition = -mvPosition.xyz;\n"
    "  gl_Position = projectionMatrix * mvPosition;\n"
    "}\n";
const char *CrystalFragmentShader =
    "#version 300 es\n"
    "precision highp float;\n"
    "uniform float dispersion;\n"
    "uniform float fresnelPow;\n"
    "in vec3 vNormal;\n"
    "in vec3 vViewPosition;\n"
    "out vec4 fragColor;\n"
    "vec3 sky(vec3 dir){\n"
    "  float t = clamp(dir.y * 0.5 + 0.5, 0.0, 1.0);\n"
    "  vec3 horizon = vec3(0.85, 0.9, 1.0);\n"
    "  vec3 zenith  = vec3(0.06, 0.09, 0.18);\n"
    "  vec3 col = mix(horizon, zenith, t);\n"
    "  float sun = pow(max(dot(dir, normalize(vec3(0.4, 0.6, 0.3))), 0.0), 64.0);\n"
    "  col += vec3(1.0, 0.95, 0.8) * sun * 1.5;\n"
    "  return col;\n"
    "}\n"
    "void main(){\n"
    "  vec3 normal  = normalize(vNormal);\n"
    "  vec3 viewDir = normalize(vViewPosition);\n"
    "  vec3 I = -viewDir;\n"
    "  float fresnel = pow(1.0 - max(dot(normal, viewDir), 0.0), fresnelPow);\n"
    "  fresnel = mix(0.05, 1.0, fresnel);\n"
    "  vec3 reflColor = sky(reflect(I, normal));\n"
    "  float base = 1.0 / 1.15;\n"
    "  vec3 refrColor;\n"
    "  refrColor.r = sky(refract(I, normal, base)).r;\n"
    "  refrColor.g = sky(refract(I, normal, base - dispersion * 0.5)).g;\n"
    "  refrColor.b = sky(refract(I, normal, base - dispersion)).b;\n"
    "  vec3 color = mix(refrColor, reflColor, fresnel);\n"
    "  vec3 lightDir = normalize(vec3(0.4, 0.6, 0.3));\n"
    "  vec3 halfDir  = normalize(lightDir + viewDir);\n"
    "  float spec = pow(max(dot(normal, halfDir), 0.0), 200.0);\n"
    "  color += vec3(1.0) * spec;\n"
    "  color *= vec3(0.96, 0.99, 1.06);\n"
    "  fragColor = vec4(color, 1.0);\n"
    "}\n";
GLuint createShaderProgram(const char *vertexShaderSource, const char *fragmentShaderSource);

GLFWwindow *window = NULL;

GLuint CrystalProgram;
GLuint VAO, VBO, EBO;
GLuint uMVMLoc, uProjLoc, uNMLoc, uDisLoc, uFrasLoc;

vec3 ldir = {0.0f, 0.0f, -1.0f};
vec3 eye = {0.0f, 0.0f, 8.5f};
vec3 up = {0.0f, 1.0f, 0.0f};
mat4 view, proj, proj_view;
float dt = 0.0f;

GLfloat vertices[] = {2.0f, 0.0f, 2.0f, 2.0f, 0.0f, 2.0f,
                      2.0f, 0.0f, -2.0f, 2.0f, 0.0f, -2.0f,
                      -2.0f, 0.0f, -2.0f, 2.0f, 0.0f, -2.0f,
                      -2.0f, 0.0f, 2.0f, -2.0f, 0.0f, 2.0f,
                      0.0f, 4.0f, 0.0f, 0.0f, 2.0f, 0.0f};
GLuint indices[] = {0, 1, 4,
                    1, 2, 4,
                    2, 3, 4,
                    0, 1, 2,
                    2, 3, 0};

int v_num = 5;
int i_num = 15;

mat4 model, tmp, mvm, viewmodel;
mat3 normmodel;

void main_loop(void)
{
    dt = glfwGetTime();
    glfwSetTime(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, 1920, 1080);

    glm_mat4_identity(model);
    glm_mat4_mul(view, model, mvm);
    glm_mat4_inv(model, tmp);
    glm_mat4_transpose(tmp);
    glm_mat4_pick3(tmp, normmodel);
    glUseProgram(CrystalProgram);
    glUniformMatrix4fv(uMVMLoc, 1, GL_FALSE, (float *)mvm);
    glUniformMatrix4fv(uProjLoc, 1, GL_FALSE, (float *)proj);
    glUniformMatrix3fv(uNMLoc, 1, GL_FALSE, (float *)normmodel);
    glUniform1f(uDisLoc, 0.05f);
    glUniform1f(uFrasLoc, 3.0f);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, i_num, GL_UNSIGNED_INT, 0);

    glfwSwapBuffers(window);
    glfwPollEvents();
}

int main()
{

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(1920, 1080, "Tidal Simulation by Kuba Chmura", NULL, NULL);
    if (window == NULL)
    {
        printf("Error creating a window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glViewport(0, 0, 1920, 1080);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glfwSwapBuffers(window);

    CrystalProgram = createShaderProgram(CrystalVertexShader, CrystalFragmentShader);
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, v_num * 6 * sizeof(GLfloat), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, i_num * sizeof(GLuint), indices, GL_STATIC_DRAW);
    glBindVertexArray(0);

    uMVMLoc = glGetUniformLocation(CrystalProgram, "modelViewMatrix");
    uProjLoc = glGetUniformLocation(CrystalProgram, "projectionMatrix");
    uNMLoc = glGetUniformLocation(CrystalProgram, "normalMatrix");
    uDisLoc = glGetUniformLocation(CrystalProgram, "dispersion");
    uFrasLoc = glGetUniformLocation(CrystalProgram, "fresnelPow");

    glfwSetTime(0);
    glm_vec3_normalize(ldir);
    glm_perspective(glm_rad(45.0f), 1920.0f / 1080.0f, 0.1f, 20.0f, proj);
    glm_lookat(eye, (vec3){0.0f, 0.0f, 0.0f}, up, view);
    glm_mat4_mul(proj, view, proj_view);

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop, 0, 1);
#else
    while (!glfwWindowShouldClose(window))
        main_loop();

    glfwDestroyWindow(window);
    glfwTerminate();
#endif
    return 0;
}
GLuint createShaderProgram(const char *vertexShaderSource, const char *fragmentShaderSource)
{
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertex_shader);
    GLint vert_success;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &vert_success);
    if (!vert_success)
    {
        char log[512];
        glGetShaderInfoLog(vertex_shader, 512, NULL, log);
        printf("VERT ERROR: %s\n", log);
    }

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragment_shader);
    GLint frag_success;
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &frag_success);
    if (!frag_success)
    {
        char log[512];
        glGetShaderInfoLog(fragment_shader, 512, NULL, log);
        printf("FRAG ERROR: %s\n", log);
    }

    GLuint shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    GLint link_success;
    glGetProgramiv(shader_program, GL_LINK_STATUS, &link_success);
    if (!link_success)
    {
        char log[512];
        glGetProgramInfoLog(shader_program, 512, NULL, log);
        printf("LINK ERROR: %s\n", log);
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return shader_program;
}