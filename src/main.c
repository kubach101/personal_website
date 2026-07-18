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
void CreateCrystalScene();
GLFWwindow *window = NULL;
GLenum err;
GLuint CrystalProgram;
GLuint VAO, VBO, EBO;
GLuint uMVMLoc, uProjLoc, uNMLoc, uDisLoc, uFrasLoc;

vec3 ldir = {0.0f, 0.0f, -1.0f};
vec3 eye = {0.0f, 0.0f, 60.0f};
vec3 up = {0.0f, 1.0f, 0.0f};
mat4 view, proj, proj_view;
float dt = 0.0f;
float scale, angle;
unsigned int seed;
vec3 pos, axis, vx;

GLfloat crystal[] = {
    1.0f,
    -1.0f,
    -1.0f,
    -0.57735f,
    -0.57735f,
    -0.57735f,
    -1.0f,
    -1.0f,
    1.0f,
    -0.57735f,
    -0.57735f,
    -0.57735f,
    -1.0f,
    1.0f,
    -1.0f,
    -0.57735f,
    -0.57735f,
    -0.57735f,
    1.0f,
    1.0f,
    1.0f,
    -0.57735f,
    0.57735f,
    0.57735f,
    -1.0f,
    1.0f,
    -1.0f,
    -0.57735f,
    0.57735f,
    0.57735f,
    -1.0f,
    -1.0f,
    1.0f,
    -0.57735f,
    0.57735f,
    0.57735f,
    1.0f,
    1.0f,
    1.0f,
    -0.57735f,
    0.57735f,
    -0.57735f,
    -1.0f,
    -1.0f,
    1.0f,
    -0.57735f,
    0.57735f,
    -0.57735f,
    1.0f,
    -1.0f,
    -1.0f,
    -0.57735f,
    0.57735f,
    -0.57735f,
    1.0f,
    1.0f,
    1.0f,
    0.57735f,
    0.57735f,
    -0.57735f,
    1.0f,
    -1.0f,
    -1.0f,
    0.57735f,
    0.57735f,
    -0.57735f,
    -1.0f,
    1.0f,
    -1.0f,
    0.57735f,
    0.57735f,
    -0.57735f,
};

int v_num = 12;
int c_num = 300;
GLfloat *scene;

mat4 model, tmp, mvm, viewmodel;
mat3 normmodel;

void main_loop(void)
{
    dt = glfwGetTime();
    glfwSetTime(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, 1920, 1080);

    angle += dt * 30.0f * M_PI / 180.0f;
    glm_mat4_identity(model);
    glm_rotate(model, angle, (vec3){0.0f, 1.0f, 0.0f});
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
    glDrawArrays(GL_TRIANGLES, 15, v_num * c_num);
    glBindVertexArray(0);

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
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glfwSwapBuffers(window);

    seed = 36817936u;
    srand(seed);
    scene = malloc(sizeof(GLfloat) * v_num * 6 * c_num);
    CreateCrystalScene();
    for (int i = 0; i < 12 * 100; i++)
    {
        printf("%d: "
               "P=(%f %f %f) "
               "N=(%f %f %f)\n",
               i,
               scene[i * 6 + 0],
               scene[i * 6 + 1],
               scene[i * 6 + 2],
               scene[i * 6 + 3],
               scene[i * 6 + 4],
               scene[i * 6 + 5]);
    }

    CrystalProgram = createShaderProgram(CrystalVertexShader, CrystalFragmentShader);
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, v_num * c_num * 6 * sizeof(GLfloat), scene, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    uMVMLoc = glGetUniformLocation(CrystalProgram, "modelViewMatrix");
    uProjLoc = glGetUniformLocation(CrystalProgram, "projectionMatrix");
    uNMLoc = glGetUniformLocation(CrystalProgram, "normalMatrix");
    uDisLoc = glGetUniformLocation(CrystalProgram, "dispersion");
    uFrasLoc = glGetUniformLocation(CrystalProgram, "fresnelPow");

    glfwSetTime(0);
    glm_vec3_normalize(ldir);
    glm_perspective(glm_rad(45.0f), 1920.0f / 1080.0f, 2.0f, 150.0f, proj);
    glm_lookat(eye, (vec3){0.0f, 0.0f, -1.0f}, up, view);
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
float frand(float min, float max)
{
    return min + ((float)rand() / (float)RAND_MAX * (max - min));
}

void CreateCrystalScene()
{

    const float minShellRadius = 20.0f;
    const float maxShellRadius = 45.0f;

    vec3 placedPositions[64];
    float placedRadii[64];

    for (int c = 0; c < c_num; c++)
    {
        float myRadius;
        int attempts = 0;
        bool placedOk = false;

        while (!placedOk && attempts < 30)
        {
            scale = frand(0.5f, 1.0f);

            vec3 dir;
            dir[0] = frand(-1.0f, 1.0f);
            dir[1] = frand(-1.0f, 1.0f);
            dir[2] = frand(-1.0f, 1.0f);
            glm_normalize(dir);

            float shellRadius = frand(minShellRadius, maxShellRadius);

            pos[0] = dir[0] * shellRadius;
            pos[1] = dir[1] * shellRadius;
            pos[2] = dir[2] * shellRadius;

            myRadius = 1.73f * scale;

            placedOk = true;
            for (int prev = 0; prev < c; prev++)
            {
                float dx = pos[0] - placedPositions[prev][0];
                float dy = pos[1] - placedPositions[prev][1];
                float dz = pos[2] - placedPositions[prev][2];
                float dist = sqrtf(dx * dx + dy * dy + dz * dz);
                float minDist = myRadius + placedRadii[prev];

                if (dist < minDist)
                {
                    placedOk = false;
                    break;
                }
            }
            attempts++;
        }

        glm_vec3_copy(pos, placedPositions[c]);
        placedRadii[c] = myRadius;

        angle = frand(0.0f, 2.0f * M_PI);

        axis[0] = frand(-1.0f, 1.0f);
        axis[1] = frand(-1.0f, 1.0f);
        axis[2] = frand(-1.0f, 1.0f);
        glm_normalize(axis);

        for (int v = 0; v < 12; v++)
        {
            int srcBase = v * 6;
            int dstBase = c * 12 * 6 + v * 6;

            if (dstBase + 5 >= v_num * 6 * c_num)
            {
                printf("OUT OF BOUNDS\n");
                exit(1);
            }
            memcpy(vx, crystal + srcBase, 3 * sizeof(GLfloat));
            glm_vec3_rotate(vx, angle, axis);
            for (int p = 0; p < 3; p++)
                scene[dstBase + p] = vx[p] * scale + pos[p];
            memcpy(vx, crystal + srcBase + 3, 3 * sizeof(GLfloat));
            glm_vec3_rotate(vx, angle, axis);
            for (int p = 0; p < 3; p++)
                scene[dstBase + p + 3] = vx[p];
        }
    }
}