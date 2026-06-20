#include <glad/glad.h>
#include <stdio.h>
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>

#define Tscale 1.0E3
#define Mscale 1.0E22
#define Rscale 1.0E8
#define Ascale Rscale / Tscale / Tscale
#define Fscale Mscale *Ascale
#define G 6.67E-11 * Rscale * Rscale * Rscale / (Mscale * Tscale * Tscale)
#define TideScale 100
#define TimeAccel 100

const char *vert =
    "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "uniform mat4 uMVP;\n"
    "uniform mat3 uNormMat;\n"
    "out vec3 normal;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = uMVP * vec4(aPos, 1.0);\n"
    "    normal = normalize(uNormMat * aPos);\n"
    "}\n";

/*
const char *fragmentShaderSource =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec3 normal;\n"
    "uniform vec3 uLDir;\n"
    "void main()\n"
    "{\n"
    "    float diff = max(dot(normal, -uLDir), 0.0);\n"
    "    vec3 col = vec3(1.0, 0.0, 0.0);\n"
    "    col *= diff;\n"
    "    col += vec3(0.2, 0.0, 0.0);\n"
    "    FragColor = vec4(col, 1.0);\n"
    "}\n";
*/

const char *frag =
    "#version 330 core\n"
    "out vec4 fragCol;\n"
    "in vec3 normal;\n"
    "uniform vec4 uCol;\n"
    "uniform vec3 uLDir;\n"
    "void main()\n"
    "{\n"
    "    float diff = max(dot(normal, -uLDir), 0.0);\n"
    "    fragCol = uCol;\n"
    "   for(int i =0; i < 3; i++){\n"
    "       float minCol = fragCol[i]*0.3f;\n"
    "       fragCol[i] = fragCol[i]*0.7;\n"
    "        fragCol[i] *= diff;\n"
    "       fragCol[i] += minCol;\n"
    "     }\n"
    "}\n";

typedef struct
{
    GLuint VAO, VBO, EBO;
    GLfloat *vertices;
    GLuint *indices;
    unsigned int v_num, i_num;
    int stacks, slices;
    GLuint shader_prog;
    float mass, r;
    vec3 pos;
    mat4 mvp;
    mat3 nmodel;
    vec4 col;
    int mode;
} Obj;

enum RenderType
{
    SOLID = 0,
    FLUID = 1
};

void sendData(Obj object, GLenum usage);
void cleanup(GLuint VAO, GLuint VBO, GLuint EBO);
GLuint createShaderProgram(const char *vertexShaderSource, const char *fragmentShaderSource);
void DrawSphere(GLfloat *vertices, GLuint *indices, float r, int stacks, int slices);
void ShapeWaterLayer(Obj *planet, Obj *satelite, Obj *water);
void Modify(mat4 proj_view, Obj *o, float orb_angle, vec3 orb_axis);
void Render(Obj *o, vec3 ldir);
float AngVel(Obj *satelite, Obj *planet);
int main()
{
    // planet core:
    Obj core = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 597.2f, 0.06378f, {0.0f, 0.0f, 0.0f}, {0}, {0}, {1.0f, 0.0f, 0.0f, 1.0f}, SOLID};
    core.r = 0.3f;
    core.stacks = 36;
    core.slices = 36;
    core.v_num = (core.stacks + 1) * (core.slices + 1) * 3;
    core.i_num = core.stacks * core.slices * 6;
    core.vertices = malloc(sizeof(GLfloat) * core.v_num);
    core.indices = malloc(sizeof(GLuint) * core.i_num);
    DrawSphere(core.vertices, core.indices, core.r, core.stacks, core.slices);

    // ocean:
    Obj ocean = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.0f, 0.067f, {0.0f, 0.0f, 0.0f}, {0}, {0}, {0.0f, 0.0f, 1.0f, 0.4f}, FLUID};
    ocean.r = 0.38f;
    ocean.stacks = 42;
    ocean.slices = 42;
    ocean.v_num = (ocean.stacks + 1) * (ocean.slices + 1) * 3;
    ocean.i_num = ocean.stacks * ocean.slices * 6;
    ocean.vertices = malloc(sizeof(GLfloat) * ocean.v_num);
    ocean.indices = malloc(sizeof(GLuint) * ocean.i_num);
    DrawSphere(ocean.vertices, ocean.indices, ocean.r, ocean.stacks, ocean.slices);

    // previous x = 3.85
    Obj moon = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7.35f, 0.017374f, {3.85f, 0.0f, 0.0f}, {0}, {0}, {0.0f, 1.0f, 0.0f, 1.0f}, SOLID};
    moon.r = 0.1f;
    moon.stacks = 24;
    moon.slices = 24;
    moon.v_num = (moon.stacks + 1) * (moon.slices + 1) * 3;
    moon.i_num = moon.stacks * moon.slices * 6;
    moon.vertices = malloc(sizeof(GLfloat) * moon.v_num);
    moon.indices = malloc(sizeof(GLuint) * moon.i_num);
    DrawSphere(moon.vertices, moon.indices, moon.r, moon.stacks, moon.slices);

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow *window = glfwCreateWindow(800, 800, "my app", NULL, NULL);
    if (window == NULL)
    {
        printf("Error creating a window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    gladLoadGL();
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        printf("Failed to initialize GLAD\n");
        return -1;
    }
    glViewport(0, 0, 800, 800);
    glEnable(GL_DEPTH_TEST);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    core.shader_prog = createShaderProgram(vert, frag);
    ocean.shader_prog = createShaderProgram(vert, frag);
    moon.shader_prog = createShaderProgram(vert, frag);

    glGenVertexArrays(1, &core.VAO);
    glGenBuffers(1, &core.VBO);
    glGenBuffers(1, &core.EBO);

    sendData(core, GL_STATIC_DRAW);

    glGenVertexArrays(1, &ocean.VAO);
    glGenBuffers(1, &ocean.VBO);
    glGenBuffers(1, &ocean.EBO);

    sendData(ocean, GL_DYNAMIC_DRAW);

    glGenVertexArrays(1, &moon.VAO);
    glGenBuffers(1, &moon.VBO);
    glGenBuffers(1, &moon.EBO);

    sendData(moon, GL_STATIC_DRAW);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glfwSwapBuffers(window);

    bool reshape = true;

    mat4 view, proj, view_proj;
    glm_perspective(glm_rad(45.0f), 800 / 800, 0.1f, 100.0f, proj);
    glm_lookat(
        (vec3){0.0f, 0.0f, 17.5f},
        (vec3){0.0f, 0.0f, 0.0f},
        (vec3){0.0f, 1.0f, 0.0f},
        view);
    glm_mat4_mul(proj, view, view_proj);

    vec3 ldir = {1.0f, 0.0f, -1.0f};
    float angle = 0.0f;

    glfwSetTime(0);
    float dt = 0.0f;
    while (!glfwWindowShouldClose(window))
    {
        dt = glfwGetTime();
        // printf("delta time = %f\n", dt);
        glfwSetTime(0);

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (reshape)
        {
            ShapeWaterLayer(&core, &moon, &ocean);
            sendData(ocean, GL_DYNAMIC_DRAW);
        }
        double omega = AngVel(&moon, &core);
        // printf("ang. velocity = %f\n", omega);
        angle += TimeAccel * omega * dt * Tscale;
        // printf("angle = %f\n", angle);

        // Creating MVP's and normal matrices:
        Modify(view_proj, &core, 0.0f, (vec3){1.0f, 0.0f, 0.0f});
        Modify(view_proj, &ocean, 0.0f, (vec3){1.0f, 0.0f, 0.0f});
        Modify(view_proj, &moon, angle, (vec3){0.0f, 0.0f, 1.0f});

        Render(&core, ldir);
        Render(&ocean, ldir);
        Render(&moon, ldir);

        glfwSwapBuffers(window);
        // debug:
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    free(core.vertices);
    free(core.indices);

    free(ocean.vertices);
    free(ocean.indices);

    free(moon.vertices);
    free(moon.indices);

    return 0;
}

void DrawSphere(GLfloat *vertices, GLuint *indices, float r, int stacks, int slices)
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
            vertices[idx] = r * sp * ct;
            idx++;
            vertices[idx] = r * cp;
            idx++;
            vertices[idx] = r * sp * st;
            idx++;
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

void ShapeWaterLayer(Obj *planet, Obj *satelite, Obj *water)
{
    float d = sqrt((planet->pos[0] - satelite->pos[0]) * (planet->pos[0] - satelite->pos[0]) + (planet->pos[1] - satelite->pos[1]) * (planet->pos[1] - satelite->pos[1]) + (planet->pos[2] - satelite->pos[2]) * (planet->pos[2] - satelite->pos[2]));
    float h0 = water->r - planet->r;

    int vidx = 0;
    vec3 Fdir;
    vec3 diff = {
        satelite->pos[0] - planet->pos[0],
        satelite->pos[1] - planet->pos[1],
        satelite->pos[2] - planet->pos[2]};
    glm_normalize_to(diff, Fdir);

    for (int i = 0; i <= water->stacks; i++)
    {
        float phi = (float)i * M_PI / water->stacks;
        for (int j = 0; j <= water->slices; j++)
        {
            float theta = (float)j * 2.0f * M_PI / water->slices;
            vec3 normal = {sinf(phi) * cosf(theta), cosf(phi), sinf(phi) * sinf(theta)};
            glm_normalize(normal);
            float cos_gamma = glm_dot(Fdir, normal);
            float h = TideScale * 0.75f * satelite->mass / planet->mass * planet->r * planet->r * planet->r * planet->r / d / d / d * (cos_gamma * cos_gamma - 1);
            water->vertices[vidx++] = normal[0] * (water->r + h);
            water->vertices[vidx++] = normal[1] * (water->r + h);
            water->vertices[vidx++] = normal[2] * (water->r + h);
        }
    }
}

void Modify(mat4 proj_view, Obj *o, float orb_angle, vec3 orb_axis)
{
    mat4 model;
    glm_mat4_identity(model);
    if (orb_angle != 0.0f)
    {
        glm_normalize(orb_axis);
        glm_rotate(model, orb_angle, orb_axis);
    }
    glm_translate(model, o->pos);
    glm_mat4_mul(proj_view, model, o->mvp);
    mat4 tmp;
    glm_mat4_inv(model, tmp);
    glm_mat4_transpose(tmp);
    glm_mat4_pick3(tmp, o->nmodel);
}

float AngVel(Obj *satelite, Obj *planet)
{
    float d = sqrt((planet->pos[0] - satelite->pos[0]) * (planet->pos[0] - satelite->pos[0]) + (planet->pos[1] - satelite->pos[1]) * (planet->pos[1] - satelite->pos[1]) + (planet->pos[2] - satelite->pos[2]) * (planet->pos[2] - satelite->pos[2]));
    float omega = sqrt(G * planet->mass / d);
    return omega;
}

void Render(Obj *o, vec3 ldir)
{
    glUseProgram(o->shader_prog);
    glUniformMatrix4fv(glGetUniformLocation(o->shader_prog, "uMVP"), 1, GL_FALSE, (float *)o->mvp);
    glUniformMatrix3fv(glGetUniformLocation(o->shader_prog, "uNormMat"), 1, GL_FALSE, (float *)o->nmodel);
    glUniform4f(glGetUniformLocation(o->shader_prog, "uCol"), o->col[0], o->col[1], o->col[2], o->col[3]);
    glUniform3f(glGetUniformLocation(o->shader_prog, "uLDir"), ldir[0], ldir[1], ldir[2]);
    if (o->mode == SOLID)
    {
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
    }
    else if (o->mode == FLUID)
    {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
    }
    glBindVertexArray(o->VAO);
    glDrawElements(GL_TRIANGLES, o->i_num, GL_UNSIGNED_INT, 0);
}

GLuint createShaderProgram(const char *vertexShaderSource, const char *fragmentShaderSource)
{
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertex_shader);

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragment_shader);

    GLuint shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return shader_program;
}

void sendData(Obj object, GLenum usage)
{
    glBindVertexArray(object.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, object.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * object.v_num, object.vertices, usage);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void *)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * object.i_num, object.indices, usage);

    glBindVertexArray(0);
}

void cleanup(GLuint VAO, GLuint VBO, GLuint EBO)
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}