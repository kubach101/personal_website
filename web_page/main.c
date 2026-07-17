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

#define Tunit 100
#define Munit 1.0E22
#define Runit 1.0E8
#define Aunit (Runit / Tunit / Tunit)
#define G (6.67E-11 * Munit * Tunit * Tunit / (Runit * Runit * Runit))
#define AVunit 1.0E-6

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

typedef struct
{
    GLuint VAO, VBO, EBO;
    GLfloat *vertices;
    GLuint *indices;
    int v_num, i_num;
} Geometry;

const char *OceanVertexShader =
    "#version 300 es\n"
    "precision highp float;\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec3 aNorm;\n"
    "out vec3 normal;\n"
    "uniform mat4 uMVP;\n"
    "uniform mat3 uNormMat;\n"
    "uniform float uTideConst;\n"
    "uniform vec3 uFDir;\n"
    "uniform float uRadius;\n"
    "void main(){\n"
    "    normal = normalize(uNormMat * aNorm);\n"
    "    float cos_gamma = dot(uFDir, normal);\n"
    "    float tide = uTideConst * (3.0 * cos_gamma * cos_gamma - 1.0);\n"
    "    vec3 position = tide * normal + (aPos * uRadius);\n"
    "    gl_Position = uMVP * vec4(position, 1.0);\n"
    "}\n";

const char *DefaultVertexShader =
    "#version 300 es\n"
    "precision highp float;\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec3 aNorm;\n"
    "out vec3 normal;\n"
    "uniform mat4 uMVP;\n"
    "uniform mat3 uNormMat;\n"
    "void main(){\n"
    "    gl_Position = uMVP * vec4(aPos, 1.0);\n"
    "    normal = normalize(uNormMat * aNorm);\n"
    "}\n";

const char *DefaultFragmentShader =
    "#version 300 es\n"
    "precision highp float;\n"
    "in vec3 normal;\n"
    "out vec4 fragCol;\n"
    "uniform vec4 uCol;\n"
    "uniform vec3 uLDir;\n"
    "void main(){\n"
    "    float diff = max(dot(normal, uLDir), 0.55);\n"
    "    fragCol = uCol*diff;\n"
    "}\n";

const char *AxisVertexShader =
    "#version 300 es\n"
    "precision highp float;\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec4 aCol;\n"
    "out vec4 color;\n"
    "uniform mat4 uMVP;"
    "void main(){\n"
    "    gl_Position = uMVP * vec4(aPos, 1.0);\n"
    "    color = aCol;\n"
    "}\n";

const char *AxisFragmentShader =
    "#version 300 es\n"
    "precision highp float;\n"
    "in vec4 color;\n"
    "out vec4 fragCol;\n"
    "void main(){\n"
    "    fragCol = color;\n"
    "}\n";

void CreateSphere(GLfloat *vertices, GLuint *indices, int stacks, int slices);
void CreateTrail(GLfloat *vertices, GLuint *indices, float end_angle, int resolution, vec3 color);
GLuint createShaderProgram(const char *vertexShaderSource, const char *fragmentShaderSource);
void KeyboardButtonCallback(GLFWwindow *window, int key, int scancode, int action, int mods);

GLFWwindow *window = NULL;
Geometry sphere = {0};
Geometry ocean = {0};
Geometry axis = {0};
Geometry trail = {0};

GLuint OceanProgram, DefaultProgram, AxisProgram;

unsigned int Tscale = 100000;
unsigned int OceanScale = 200;
float TideScale = 1000000.0;

float planet_mass = 597.2f;
float planet_rad = 0.06378f;
float satelite_mass = 7.35f;
float satelite_rad = 0.017374f;
float distance = 3.85f;
float base_h;

vec3 orb_axis = {0.0f, 1.0f, 0.0f};
float orb_ang = 0.0f;
float deviation;
float aspect;

bool hide_fdir = true;
bool update_fdir_visibility = true;

vec3 ldir = {0.0f, 0.0f, -1.0f};
float eye_ang[2] = {0.0f};
float eye_rad = 12.5f;
bool update_vision = true;
mat4 view, proj, proj_view;
mat4 axis_proj, axis_view, axis_proj_view;
float dt = 0.0f;
int ocean_stacks = 42;
int ocean_slices = 84;

float zoom_speed = 2.5f;
float rotate_speed = 25.0f * M_PI / 180.0f;

void main_loop(void)
{
    dt = glfwGetTime();
    glfwSetTime(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    float tide_const = TideScale * 0.5f * satelite_mass / planet_mass * planet_rad * planet_rad * planet_rad * planet_rad / distance / distance / distance;
    float ang_vel = 1 / AVunit * sqrt(G * planet_mass / distance / distance / distance) / Tunit;
    orb_ang += ang_vel * dt * Tscale * AVunit;
    // orb_ang -= deviation;
    orb_ang = fmod(orb_ang, 2 * M_PI);
    vec3 fdir = {1.0f, 0.0f, 0.0f};
    glm_vec3_rotate(fdir, orb_ang, orb_axis);

    if (update_vision)
    {
        vec3 eye = {0.0f, 0.0f, eye_rad};
        vec3 axis_eye = {0.0f, 0.0f, 2.0f};
        glm_vec3_rotate(eye, eye_ang[0], (vec3){1.0f, 0.0f, 0.0f});
        glm_vec3_rotate(eye, eye_ang[1], (vec3){0.0f, 1.0f, 0.0f});
        glm_vec3_rotate(axis_eye, eye_ang[0], (vec3){1.0f, 0.0f, 0.0f});
        glm_vec3_rotate(axis_eye, eye_ang[1], (vec3){0.0f, 1.0f, 0.0f});
        vec3 up = {0.0f, 1.0f, 0.0f};
        glm_vec3_rotate(up, eye_ang[0], (vec3){1.0f, 0.0f, 0.0f});
        glm_vec3_rotate(up, eye_ang[1], (vec3){0.0f, 1.0f, 0.0f});
        glm_perspective(glm_rad(45.0f), aspect, 0.1f, 30.0f, proj);
        glm_lookat(eye, (vec3){0.0f, 0.0f, 0.0f}, up, view);
        glm_mat4_mul(proj, view, proj_view);
        glm_perspective(glm_rad(45.0f), 1.0f, 0.05f, 5.0f, axis_proj);
        glm_lookat(axis_eye, (vec3){0.0f, 0.0f, 0.0f}, up, axis_view);
        glm_mat4_mul(axis_proj, axis_view, axis_proj_view);

        update_vision = false;
    }
    if (update_fdir_visibility)
    {
        if (hide_fdir)
        {
            trail.vertices[6] = 0.0f;
            trail.vertices[13] = 0.0f;
        }
        else
        {
            trail.vertices[6] = 1.0f;
            trail.vertices[13] = 1.0f;
        }
        glBindBuffer(GL_ARRAY_BUFFER, trail.VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, trail.v_num * 7 * sizeof(GLfloat), trail.vertices);
        update_fdir_visibility = false;
    }
    // rendering:

    glViewport(0, 0, 1200, 800);
    // satelite:
    mat4 model,
        tmp, mvp;
    mat3 norm_model;
    vec3 pos = {0};
    pos[0] = distance;
    glm_mat4_identity(model);
    glm_rotate(model, orb_ang, orb_axis);
    glm_translate(model, pos);
    glm_scale_uni(model, satelite_rad);
    glm_mat4_mul(proj_view, model, mvp);
    glm_mat4_inv(model, tmp);
    glm_mat4_transpose(tmp);
    glm_mat4_pick3(tmp, norm_model);

    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    glUseProgram(DefaultProgram);
    glUniformMatrix4fv(glGetUniformLocation(DefaultProgram, "uMVP"), 1, GL_FALSE, (float *)mvp);
    glUniformMatrix3fv(glGetUniformLocation(DefaultProgram, "uNormMat"), 1, GL_FALSE, (float *)norm_model);
    glUniform4f(glGetUniformLocation(DefaultProgram, "uCol"), 0.0f, 1.0f, 0.0f, 1.0f);
    glUniform3f(glGetUniformLocation(DefaultProgram, "uLDir"), ldir[0], ldir[1], ldir[2]);
    glBindVertexArray(sphere.VAO);
    glDrawElements(GL_TRIANGLES, sphere.i_num, GL_UNSIGNED_INT, 0);

    // core:
    mat3 ident;
    glm_mat3_identity(ident);
    glm_mat4_identity(model);
    glm_scale_uni(model, planet_rad);
    glm_mat4_mul(proj_view, model, mvp);
    glUseProgram(DefaultProgram);
    glUniformMatrix4fv(glGetUniformLocation(DefaultProgram, "uMVP"), 1, GL_FALSE, (float *)mvp);
    glUniformMatrix3fv(glGetUniformLocation(DefaultProgram, "uNormMat"), 1, GL_FALSE, (float *)ident);
    glUniform4f(glGetUniformLocation(DefaultProgram, "uCol"), 1.0f, 0.0f, 0.0f, 1.0f);
    glUniform3f(glGetUniformLocation(DefaultProgram, "uLDir"), ldir[0], ldir[1], ldir[2]);
    glBindVertexArray(sphere.VAO);
    glDrawElements(GL_TRIANGLES, sphere.i_num, GL_UNSIGNED_INT, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // trail:
    glm_mat4_identity(model);
    glm_rotate(model, orb_ang, orb_axis);
    glm_scale_uni(model, distance);
    glm_mat4_mul(proj_view, model, mvp);
    glUseProgram(AxisProgram);
    glUniformMatrix4fv(glGetUniformLocation(AxisProgram, "uMVP"), 1, GL_FALSE, (float *)mvp);
    glBindVertexArray(trail.VAO);
    glDrawElements(GL_LINE_STRIP, trail.i_num, GL_UNSIGNED_INT, 0);

    // ocean:
    glm_mat4_identity(model);
    glm_mat4_mul(proj_view, model, mvp);
    glUseProgram(OceanProgram);
    glUniformMatrix4fv(glGetUniformLocation(OceanProgram, "uMVP"), 1, GL_FALSE, (float *)mvp);
    glUniformMatrix3fv(glGetUniformLocation(OceanProgram, "uNormMat"), 1, GL_FALSE, (float *)ident);
    glUniform1f(glGetUniformLocation(OceanProgram, "uTideConst"), tide_const);
    glUniform3f(glGetUniformLocation(OceanProgram, "uFDir"), fdir[0], fdir[1], fdir[2]);
    glUniform1f(glGetUniformLocation(OceanProgram, "uRadius"), planet_rad + base_h);
    glUniform4f(glGetUniformLocation(OceanProgram, "uCol"), 0.0f, 0.0f, 1.0f, 0.5f);
    glUniform3f(glGetUniformLocation(OceanProgram, "uLDir"), ldir[0], ldir[1], ldir[2]);
    glBindVertexArray(ocean.VAO);
    glDrawElements(GL_TRIANGLES, ocean.i_num, GL_UNSIGNED_INT, 0);

    // axis:
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glViewport(1200 - 225, 800 - 225, 200, 200);
    glm_mat4_identity(model);
    glm_scale_uni(model, 0.5f);
    glm_mat4_mul(axis_proj_view, model, mvp);
    glUseProgram(AxisProgram);
    glUniformMatrix4fv(glGetUniformLocation(AxisProgram, "uMVP"), 1, GL_FALSE, (float *)mvp);
    glBindVertexArray(axis.VAO);
    glDrawElements(GL_LINES, axis.i_num, GL_UNSIGNED_INT, 0);

    glfwSwapBuffers(window);
    glfwPollEvents();
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
    {
        eye_rad -= zoom_speed * dt;
        update_vision = true;
    }
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
    {
        eye_rad += zoom_speed * dt;
        update_vision = true;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
    {
        eye_ang[1] -= rotate_speed * dt;
        update_vision = true;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
    {
        eye_ang[1] += rotate_speed * dt;
        update_vision = true;
    }
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
    {
        eye_ang[0] -= rotate_speed * dt;
        update_vision = true;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
    {
        eye_ang[0] += rotate_speed * dt;
        update_vision = true;
    }
}

int main()
{
    sphere.v_num = (32 + 1) * (64 + 1);
    sphere.i_num = 32 * 64 * 6;
    sphere.vertices = malloc(sizeof(GLfloat) * sphere.v_num * 10);
    sphere.indices = malloc(sizeof(GLuint) * sphere.i_num);
    CreateSphere(sphere.vertices, sphere.indices, 32, 64);

    ocean.v_num = (42 + 1) * (84 + 1);
    ocean.i_num = 42 * 84 * 6;
    ocean.vertices = malloc(sizeof(GLfloat) * ocean.v_num * 10);
    ocean.indices = malloc(sizeof(GLuint) * ocean.i_num);
    CreateSphere(ocean.vertices, ocean.indices, 42, 84);

    GLfloat axis_vertices[] = {
        // Z axis:
        0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
        // X axis:
        1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        // Y axis:
        0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f};

    GLuint axis_indices[] = {0, 1, 2, 3, 4, 5};

    axis.vertices = axis_vertices;
    axis.indices = axis_indices;
    axis.v_num = 6;
    axis.i_num = 6;

    trail.v_num = 15 + 2;
    trail.i_num = trail.v_num;
    trail.vertices = malloc(sizeof(GLfloat) * trail.v_num * 7);
    trail.indices = malloc(sizeof(GLuint) * trail.i_num);
    CreateTrail(trail.vertices, trail.indices, 55.0f * M_PI / 180.0f, 15, (vec3){1.0f, 1.0f, 1.0f});

    base_h = 3700.0f * OceanScale / Runit;
    deviation = 15.0f / 180.0f * M_PI;
    aspect = 1200.0f / 800.0f;

    // Interactive parameter/scale entry is a blocking, terminal-only feature.
    // It is kept for native desktop builds only; the emscripten (browser) build
    // always uses the compiled-in defaults above so the main loop can start
    // immediately without waiting on stdin that the browser can't provide.
#ifndef __EMSCRIPTEN__
    char ans = '\0';
    printf("Set custom paramters(y/n):");
    scanf("%c", &ans);
    printf("\033[1A\033[J");
    if (ans == 'y' || ans == 'Y')
    {
        printf("Input satelite distance(e22 m):");
        scanf(" %f", &distance);
        printf("\033[1A\033[J");
        printf("Input planet mass(e8 kg):");
        scanf(" %f", &planet_mass);
        printf("\033[1A\033[J");
        printf("Input satelite mass(e8 kg):");
        scanf("%f", &satelite_mass);
        printf("\033[1A\033[J");
        printf("Input planet radius(e22 m):");
        scanf(" %f", &planet_rad);
        printf("\033[1A\033[J");
        printf("Input satelite radius(e22 m):");
        scanf(" %f", &satelite_rad);
        printf("\033[1A\033[J");
        printf("Input ocean base height(m):");
        scanf(" %f", &base_h);
        printf("\033[1A\033[J");
        printf("Input the oceans deviation(deg):");
        scanf(" %f", &deviation);
        base_h /= Runit;
        deviation *= M_PI / 180;
    }
    ans = '\0';
    printf("Set custom scaling(y/n):");
    scanf(" %c", &ans);
    printf("\033[1A\033[J");
    if (ans == 'y' || ans == 'Y')
    {
        printf("Input time acceleration:");
        scanf(" %u", &Tscale);
        printf("\033[1A\033[J");
        printf("Input rendered ocean scale:");
        scanf(" %u", &OceanScale);
        printf("\033[1A\033[J");
        printf("Input visual tide scale:");
        scanf(" %f", &TideScale);
        printf("\033[1A\033[J");
    }
#endif

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(1200, 800, "Tidal Simulation by Kuba Chmura", NULL, NULL);
    if (window == NULL)
    {
        printf("Error creating a window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, KeyboardButtonCallback);

    glViewport(0, 0, 1200, 800);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glfwSwapBuffers(window);

    OceanProgram = createShaderProgram(OceanVertexShader, DefaultFragmentShader);
    DefaultProgram = createShaderProgram(DefaultVertexShader, DefaultFragmentShader);
    AxisProgram = createShaderProgram(AxisVertexShader, AxisFragmentShader);

    glGenVertexArrays(1, &sphere.VAO);
    glGenBuffers(1, &sphere.VBO);
    glGenBuffers(1, &sphere.EBO);

    glGenVertexArrays(1, &ocean.VAO);
    glGenBuffers(1, &ocean.VBO);
    glGenBuffers(1, &ocean.EBO);

    glGenVertexArrays(1, &axis.VAO);
    glGenBuffers(1, &axis.VBO);
    glGenBuffers(1, &axis.EBO);

    glGenVertexArrays(1, &trail.VAO);
    glGenBuffers(1, &trail.VBO);
    glGenBuffers(1, &trail.EBO);

    // sphere:
    glBindVertexArray(sphere.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, sphere.VBO);
    glBufferData(GL_ARRAY_BUFFER, sphere.v_num * 6 * sizeof(GLfloat), sphere.vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphere.i_num * sizeof(GLuint), sphere.indices, GL_STATIC_DRAW);
    glBindVertexArray(0);

    // ocean:
    glBindVertexArray(ocean.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, ocean.VBO);
    glBufferData(GL_ARRAY_BUFFER, ocean.v_num * 6 * sizeof(GLfloat), ocean.vertices, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ocean.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, ocean.i_num * sizeof(GLuint), ocean.indices, GL_DYNAMIC_DRAW);
    glBindVertexArray(0);

    // axis:
    glBindVertexArray(axis.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, axis.VBO);
    glBufferData(GL_ARRAY_BUFFER, axis.v_num * 7 * sizeof(GLfloat), axis.vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (void *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, axis.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, axis.i_num * sizeof(GLuint), axis.indices, GL_STATIC_DRAW);
    glBindVertexArray(0);

    // trail:
    glBindVertexArray(trail.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, trail.VBO);
    glBufferData(GL_ARRAY_BUFFER, trail.v_num * 7 * sizeof(GLfloat), trail.vertices, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (void *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, trail.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, trail.i_num * sizeof(GLuint), trail.indices, GL_STATIC_DRAW);
    glBindVertexArray(0);

    glfwSetTime(0);
    glm_vec3_normalize(ldir);

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop, 0, 1);
#else
    while (!glfwWindowShouldClose(window))
        main_loop();

    glfwDestroyWindow(window);
    glfwTerminate();
    free(sphere.vertices);
    free(sphere.indices);
    free(ocean.vertices);
    free(ocean.indices);
    free(trail.vertices);
    free(trail.indices);
#endif
    return 0;
}
void CreateSphere(GLfloat *vertices, GLuint *indices, int stacks, int slices)
{

    int idx = 0;
    int iidx = 0;
    for (int i = 0; i <= stacks; i++)
    {
        float phi = (float)i * M_PI / stacks;
        float sp = sin(phi);
        float cp = cos(phi);
        for (int j = 0; j <= slices; j++)
        {
            float theta = (float)j * 2 * M_PI / slices;
            float st = sin(theta);
            float ct = cos(theta);

            vertices[idx] = sp * ct;
            vertices[idx + 3] = vertices[idx];
            idx++;
            vertices[idx] = cp;
            vertices[idx + 3] = vertices[idx];
            idx++;
            vertices[idx] = sp * st;
            vertices[idx + 3] = vertices[idx];
            idx++;

            idx += 3;
        }
    }

    for (int i = 0; i < stacks; i++)
    {
        for (int j = 0; j < slices; j++)
        {
            int a = i * (slices + 1) + j;
            int b = i * (slices + 1) + j + 1;
            int c = (i + 1) * (slices + 1) + j;
            int d = (i + 1) * (slices + 1) + j + 1;

            indices[iidx++] = a;
            indices[iidx++] = b;
            indices[iidx++] = c;

            indices[iidx++] = b;
            indices[iidx++] = d;
            indices[iidx++] = c;
        }
    }
}
void CreateTrail(GLfloat *vertices, GLuint *indices, float end_angle, int resolution, vec3 color)
{
    int vidx = 0;
    for (int i = 0; i < 3; i++)
    {
        vertices[vidx++] = 0.0f;
    }
    vertices[vidx++] = color[0];
    vertices[vidx++] = color[1];
    vertices[vidx++] = color[2];
    vertices[vidx++] = 1.0f;

    vertices[vidx++] = 1.0f;
    vertices[vidx++] = 0.0f;
    vertices[vidx++] = 0.0f;

    vertices[vidx++] = color[0];
    vertices[vidx++] = color[1];
    vertices[vidx++] = color[2];
    vertices[vidx++] = 1.0f;
    for (int i = 0; i < resolution; i++)
    {
        float angle = end_angle / resolution * i;
        vertices[vidx++] = cos(-angle);
        vertices[vidx++] = 0.0f;
        vertices[vidx++] = -sin(-angle);
        vertices[vidx++] = color[0];
        vertices[vidx++] = color[1];
        vertices[vidx++] = color[2];
        float t = (float)(resolution - i) / (float)resolution;
        vertices[vidx++] = sqrtf(t);
    }
    int iidx = 0;
    for (int i = 0; i < resolution + 2; i++)
    {
        indices[iidx++] = i;
    }
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
void KeyboardButtonCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    switch (key)
    {
    case GLFW_KEY_SPACE:
        if (action == GLFW_PRESS)
        {
            hide_fdir = !hide_fdir;
            update_fdir_visibility = true;
        }
        break;
    default:
        break;
    }
}