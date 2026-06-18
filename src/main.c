#include <glad/glad.h>
#include <stdio.h>
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>

#define Tscale 100
#define Mscale 1.0E20
#define Rscale 1.0E6
#define Ascale Rscale / Tscale / Tscale
#define Fscale Mscale *Ascale
#define G 6.67E-11 * Rscale * Rscale * Rscale / (Mscale * Tscale * Tscale)

const char *vertexShaderSource =
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

const char *fragmentShaderSource2 =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec3 normal;\n"
    "uniform vec3 uLDir;\n"
    "void main()\n"
    "{\n"
    "    float diff = max(dot(normal, -uLDir), 0.0);\n"
    "    vec3 col = vec3(0.0, 0.0, 0.6);\n"
    "    col *= diff;\n"
    "    col += vec3(0.0, 0.0, 0.4);\n"
    "    FragColor = vec4(col, 0.5);\n"
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
} Obj;

void sendData(Obj object, GLenum usage);
void cleanup(GLuint VAO, GLuint VBO, GLuint EBO);
GLuint createShaderProgram(const char *vertexShaderSource, const char *fragmentShaderSource);
void DrawSphere(GLfloat *vertices, GLuint *indices, float r, int stacks, int slices);
void getWaterHeight(Obj *planet, Obj *satelite, Obj *water);
int main()
{
    // planet core:
    Obj core = {0};
    core.r = 0.3f;
    core.stacks = 36;
    core.slices = 36;
    core.v_num = (core.stacks + 1) * (core.slices + 1) * 3;
    core.i_num = core.stacks * core.slices * 6;
    core.vertices = malloc(sizeof(GLfloat) * core.v_num);
    core.indices = malloc(sizeof(GLuint) * core.i_num);
    DrawSphere(core.vertices, core.indices, core.r, core.stacks, core.slices);
    core.mass = 14.5f;
    core.pos[0] = 0.0f;
    core.pos[1] = 0.0f;
    core.pos[2] = 0.0f;

    // ocean:
    Obj ocean = {0};
    ocean.r = 0.35f;
    ocean.stacks = 42;
    ocean.slices = 42;
    ocean.v_num = (ocean.stacks + 1) * (ocean.slices + 1) * 3;
    ocean.i_num = ocean.stacks * ocean.slices * 6;
    ocean.vertices = malloc(sizeof(GLfloat) * ocean.v_num);
    ocean.indices = malloc(sizeof(GLuint) * ocean.i_num);
    DrawSphere(ocean.vertices, ocean.indices, ocean.r, ocean.stacks, ocean.slices);
    ocean.pos[0] = 0.0f;
    ocean.pos[1] = 0.0f;
    ocean.pos[2] = 0.0f;
    ocean.mass = 0.5f;

    Obj moon = {0};
    moon.r = 0.08f;
    moon.stacks = 12;
    moon.slices = 12;
    moon.v_num = (moon.stacks + 1) * (moon.slices + 1) * 3;
    moon.i_num = moon.stacks * moon.slices * 6;
    moon.vertices = malloc(sizeof(GLfloat) * moon.v_num);
    moon.indices = malloc(sizeof(GLuint) * moon.i_num);
    DrawSphere(moon.vertices, moon.indices, moon.r, moon.stacks, moon.slices);
    moon.pos[0] = 3.0f;
    moon.pos[1] = 0.0f;
    moon.pos[2] = 0.0f;
    moon.mass = 50000.0f;

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

    core.shader_prog = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    ocean.shader_prog = createShaderProgram(vertexShaderSource, fragmentShaderSource2);
    moon.shader_prog = createShaderProgram(vertexShaderSource, fragmentShaderSource);

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
    float deltaH = 0.0f;
    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Reshape water
        if (reshape)
        {
            getWaterHeight(&core, &moon, &ocean);
            sendData(ocean, GL_DYNAMIC_DRAW);
        }

        mat4 model, mvp, view, proj, view_proj, tmp;
        glm_lookat(
            (vec3){0.0f, 0.0f, 2.5f},
            (vec3){0.0f, 0.0f, 0.0f},
            (vec3){0.0f, 1.0f, 0.0f},
            view);
        glm_perspective(glm_rad(45.0f), 800.0f / 800.0f, 0.1f, 100.0f, proj);
        glm_mat4_mul(proj, view, view_proj);
        float angle = 0.0f; //(float)glfwGetTime();
        glm_mat4_identity(model);
        glm_rotate(model, angle, (vec3){0.5774f, 0.5774f, 0.5774f});
        glm_mat4_mul(view_proj, model, mvp);

        mat3 nmodel;
        glm_mat4_inv(model, tmp);
        glm_mat4_transpose(tmp);
        glm_mat4_pick3(tmp, nmodel);

        vec3 ldir = {1.0f, 0.0f, -1.0f};
        glm_normalize(ldir);

        glUseProgram(core.shader_prog);
        glUniformMatrix4fv(glGetUniformLocation(core.shader_prog, "uMVP"), 1, GL_FALSE, (float *)mvp);
        glUniformMatrix4fv(glGetUniformLocation(core.shader_prog, "uModel"), 1, GL_FALSE, (float *)model);
        glUniformMatrix3fv(glGetUniformLocation(core.shader_prog, "uNormMat"), 1, GL_FALSE, (float *)nmodel);
        glUniform3f(glGetUniformLocation(core.shader_prog, "uLDir"), ldir[0], ldir[1], ldir[2]);
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        glBindVertexArray(core.VAO);
        glDrawElements(GL_TRIANGLES, core.i_num, GL_UNSIGNED_INT, 0);

        glUseProgram(ocean.shader_prog);
        glUniformMatrix4fv(glGetUniformLocation(ocean.shader_prog, "uMVP"), 1, GL_FALSE, (float *)mvp);
        glUniformMatrix4fv(glGetUniformLocation(ocean.shader_prog, "uModel"), 1, GL_FALSE, (float *)model);
        glUniformMatrix3fv(glGetUniformLocation(ocean.shader_prog, "uNormMat"), 1, GL_FALSE, (float *)nmodel);
        glUniform3f(glGetUniformLocation(ocean.shader_prog, "uLDir"), ldir[0], ldir[1], ldir[2]);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        glBindVertexArray(ocean.VAO);
        glDrawElements(GL_TRIANGLES, ocean.i_num, GL_UNSIGNED_INT, 0);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

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
            int A = i * (slices + 1) + j;
            int B = i * (slices + 1) + j + 1;
            int C = (i + 1) * (slices + 1) + j;
            int D = (i + 1) * (slices + 1) + j + 1;

            indices[iidx++] = A;
            indices[iidx++] = B;
            indices[iidx++] = C;

            indices[iidx++] = B;
            indices[iidx++] = D;
            indices[iidx++] = C;
        }
    }
}

void getWaterHeight(Obj *planet, Obj *satelite, Obj *water)
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
            // printf("normal = %f %f %f\n", normal[0], normal[1], normal[2]);
            glm_normalize(normal);
            float cos_gamma = glm_dot(Fdir, normal);
            float h = h0 * 0.75f * satelite->mass / planet->mass * planet->r * planet->r * planet->r * planet->r / d / d / d * (cos_gamma * cos_gamma - 1);
            water->vertices[vidx++] = normal[0] * (water->r + h);
            water->vertices[vidx++] = normal[1] * (water->r + h);
            water->vertices[vidx++] = normal[2] * (water->r + h);
            // printf("new height = %f| cos_gamma = %f\n", h, cos_gamma);
            // printf("H0 = %f", h0);
        }
    }
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