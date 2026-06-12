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
    "layout (location = 1) in vec3 aNorm;\n"
    "uniform mat4 uMVP;\n"
    "uniform mat3 uNormMat;\n"
    "out vec3 normal;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = uMVP * vec4(aPos, 1.0);\n"
    "    normal = normalize(uNormMat * aNorm);\n"
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
    "    vec3 col = vec3(0.0, 0.0, 0.8);\n"
    "    col *= diff;\n"
    "    col += vec3(0.0, 0.0, 0.2);\n"
    "    FragColor = vec4(col, 0.4);\n"
    "}\n";

typedef struct
{
    float mass;
    vec3 pos, v, a;
    GLuint VAO, VBO, EBO, NAO;
    GLfloat *vertices;
    GLuint *indices;
    GLfloat *normals;
    unsigned int v_num, i_num, n_num;
    GLuint shader_prog;
} body;
void sendData(GLuint VAO, GLuint VBO, GLuint EBO, GLuint NAO, GLfloat *vertices, GLuint *indices, GLfloat *normals, unsigned int v_num, unsigned int i_num, GLenum usage);
void cleanup(GLuint VAO, GLuint VBO, GLuint EBO);
GLuint createShaderProgram(const char *vertexShaderSource, const char *fragmentShaderSource);
void DrawSphere(GLfloat *vertices, GLuint *indices, GLfloat *normals, float r, int stacks, int slices);
void getWaterHeight(float deltaH, vec3 FDir, GLfloat *vertices, int stacks, int slices, float r);
void updateNormals(GLfloat *vertices, GLfloat *normals, unsigned int num);

int main()
{
    // planet core:
    body core = {0};
    float r = 0.3f;
    int stacks = 24;
    int slices = 24;
    core.v_num = (stacks + 1) * (slices + 1) * 3;
    core.i_num = stacks * slices * 6;
    core.n_num = core.v_num;
    core.vertices = malloc(sizeof(GLfloat) * core.v_num);
    core.indices = malloc(sizeof(GLuint) * core.i_num);
    core.normals = malloc(sizeof(GLfloat) * core.n_num);
    DrawSphere(core.vertices, core.indices, core.normals, r, stacks, slices);

    // ocean:
    body ocean = {0};
    r = 0.5f;
    stacks = 32;
    slices = 32;
    vec3 F = {1.0f, 0.0f, 0.0f};
    glm_normalize(F);
    ocean.v_num = (stacks + 1) * (slices + 1) * 3;
    ocean.i_num = stacks * slices * 6;
    ocean.n_num = ocean.v_num;
    ocean.vertices = malloc(sizeof(GLfloat) * ocean.v_num);
    ocean.indices = malloc(sizeof(GLuint) * ocean.i_num);
    ocean.normals = malloc(sizeof(GLfloat) * ocean.n_num);
    DrawSphere(ocean.vertices, ocean.indices, ocean.normals, r, stacks, slices);
    // getWaterHeight(0.15f, F, ocean.vertices, stacks, slices, r);
    // updateNormals(ocean.vertices, ocean.normals, ocean.n_num);

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

    glGenVertexArrays(1, &core.VAO);
    glGenBuffers(1, &core.VBO);
    glGenBuffers(1, &core.EBO);
    glGenBuffers(1, &core.NAO);

    sendData(core.VAO, core.VBO, core.EBO, core.NAO,
             core.vertices, core.indices, core.normals,
             core.v_num, core.i_num, GL_STATIC_DRAW);

    glGenVertexArrays(1, &ocean.VAO);
    glGenBuffers(1, &ocean.VBO);
    glGenBuffers(1, &ocean.EBO);
    glGenBuffers(1, &ocean.NAO);

    sendData(ocean.VAO, ocean.VBO, ocean.EBO, ocean.NAO,
             ocean.vertices, ocean.indices, ocean.normals,
             ocean.v_num, ocean.i_num, GL_STATIC_DRAW);

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
            DrawSphere(ocean.vertices, ocean.indices, ocean.normals, r, stacks, slices);
            getWaterHeight(deltaH, F, ocean.vertices, stacks, slices, r);
            updateNormals(ocean.vertices, ocean.normals, ocean.n_num);
            sendData(ocean.VAO, ocean.VBO, ocean.EBO, ocean.NAO,
                     ocean.vertices, ocean.indices, ocean.normals,
                     ocean.v_num, ocean.i_num, GL_DYNAMIC_DRAW);
            deltaH += 0.000002f;
            printf("\rdeltaH = %f pos:[%f|%f|%f]", deltaH, ocean.vertices[0], ocean.vertices[1], ocean.vertices[2]);
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
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    free(core.vertices);
    free(core.indices);
    free(core.normals);

    free(ocean.vertices);
    free(ocean.indices);
    free(ocean.normals);

    return 0;
}

void DrawSphere(GLfloat *vertices, GLuint *indices, GLfloat *normals, float r, int stacks, int slices)
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
            normals[idx] = sp * ct;
            vertices[idx] = r * normals[idx];
            idx++;
            normals[idx] = cp;
            vertices[idx] = r * normals[idx];
            idx++;
            normals[idx] = sp * st;
            vertices[idx] = r * normals[idx];
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

void getWaterHeight(float deltaH, vec3 FDir, GLfloat *vertices, int stacks, int slices, float r)
{
    int vidx = 0;
    for (int i = 0; i <= slices; i++)
    {
        float phi = (float)i * M_PI / slices;
        for (int j = 0; j <= stacks; j++)
        {
            float theta = (float)j * 2.0f * M_PI / stacks;
            vec3 normal = {sin(phi) * cos(theta), cos(phi), sin(phi) * sin(theta)};
            glm_normalize(normal);
            float cos_gamma = glm_dot(FDir, normal);
            float level_change = deltaH * (3 * cos_gamma * cos_gamma - 1) / 2;
            vertices[vidx++] = normal[0] * (r + level_change);
            vertices[vidx++] = normal[1] * (r + level_change);
            vertices[vidx++] = normal[2] * (r + level_change);
        }
    }
}

void updateNormals(GLfloat *vertices, GLfloat *normals, unsigned int num)
{
    for (int i = 0; i < num; i += 3)
    {
        vec3 v;
        for (int j = 0; j < 3; j++)
        {
            v[j] = vertices[i + j];
        }
        glm_normalize(v);
        for (int j = 0; j < 3; j++)
        {
            normals[i + j] = v[j];
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

void sendData(GLuint VAO, GLuint VBO, GLuint EBO, GLuint NAO, GLfloat *vertices, GLuint *indices, GLfloat *normals, unsigned int v_num, unsigned int i_num, GLenum usage)
{
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * v_num, vertices, usage);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void *)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * i_num, indices, usage);

    glBindBuffer(GL_ARRAY_BUFFER, NAO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * v_num, normals, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void *)0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void cleanup(GLuint VAO, GLuint VBO, GLuint EBO)
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}