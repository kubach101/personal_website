#include <glad/glad.h>
#include <stdio.h>
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>

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
    "    float diff = max(dot(normal, uLDir), 0.0);\n"
    "    vec3 c = vec3(0.0, 1.0, 0.6);\n"
    "    c *= diff;\n"
    "    c += vec3(0.0, 0.2, 0.15);\n"
    "    FragColor = vec4(c, 1.0);\n"
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
void getWaterHeight(float deltaH, GLfloat *vertices, int stacks, int slices, float r);
void updateNormals(GLfloat *vertices, GLfloat *normals, unsigned int num);

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    body core = {0};
    float r = 0.3f;
    int stacks = 50;
    int slices = 50;

    core.v_num = (stacks + 1) * (slices + 1) * 3;
    core.i_num = stacks * slices * 6;
    core.n_num = core.v_num;

    core.vertices = malloc(sizeof(GLfloat) * core.v_num);
    core.indices = malloc(sizeof(GLuint) * core.i_num);
    core.normals = malloc(sizeof(GLfloat) * core.n_num);

    DrawSphere(core.vertices, core.indices, core.normals, r, stacks, slices);
    printf("Sphere generated\n");
    getWaterHeight(0.1f, core.vertices, stacks, slices, r);
    GLFWwindow *window = glfwCreateWindow(800, 800, "my app", NULL, NULL);
    if (window == NULL)
    {
        printf("Error creating a window\n");
        glfwTerminate();
        return 1;
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
    core.shader_prog = createShaderProgram(vertexShaderSource, fragmentShaderSource);

    glGenVertexArrays(1, &core.VAO);
    glGenBuffers(1, &core.VBO);
    glGenBuffers(1, &core.EBO);
    glGenBuffers(1, &core.NAO);

    sendData(core.VAO, core.VBO, core.EBO, core.NAO,
             core.vertices, core.indices, core.normals,
             core.v_num, core.i_num, GL_STATIC_DRAW);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glfwSwapBuffers(window);

    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glfwPollEvents();
        mat4 view, proj, view_proj;
        glm_lookat(
            (vec3){0.0f, 0.0f, 2.5f},
            (vec3){0.0f, 0.0f, 0.0f},
            (vec3){0.0f, 1.0f, 0.0f},
            view);
        glm_perspective(glm_rad(45.0f), 800.0f / 800.0f, 0.1f, 100.0f, proj);
        glm_mat4_mul(proj, view, view_proj);
        float angle = (float)glfwGetTime();
        mat4 model;
        glm_mat4_identity(model);
        glm_rotate(model, angle, (vec3){0.577f, 0.577f, 0.577f});
        mat4 mvp;
        glm_mat4_mul(view_proj, model, mvp);
        mat4 tmp;
        mat3 nmodel;
        glm_mat4_inv(model, tmp);
        glm_mat4_transpose(tmp);
        glm_mat4_pick3(tmp, nmodel);

        vec3 ldir = {1.0f, 0.0f, 1.0f};
        glm_normalize(ldir);
        glUseProgram(core.shader_prog);
        glUniformMatrix4fv(glGetUniformLocation(core.shader_prog, "uMVP"), 1, GL_FALSE, (float *)mvp);
        glUniformMatrix4fv(glGetUniformLocation(core.shader_prog, "uModel"), 1, GL_FALSE, (float *)model);
        glUniformMatrix3fv(glGetUniformLocation(core.shader_prog, "uNormMat"), 1, GL_FALSE, (float *)nmodel);
        glUniform3f(glGetUniformLocation(core.shader_prog, "uLDir"), ldir[0], ldir[1], ldir[2]);
        glBindVertexArray(core.VAO);
        glDrawElements(GL_TRIANGLES, core.i_num, GL_UNSIGNED_INT, 0);
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    free(core.vertices);
    free(core.indices);

    return 0;
}

void DrawSphere(GLfloat *vertices, GLuint *indices, GLfloat *normals, float r, int stacks, int slices)
{

    int idx = 0;
    int iidx = 0;

    for (int i = 0; i <= slices; i++)
    {
        float phi = (float)i * M_PI / slices;
        float sa = sinf(phi);
        float ca = cosf(phi);

        for (int j = 0; j <= stacks; j++)
        {
            float theta = (float)j * 2.0f * M_PI / stacks;
            float sb = sinf(theta);
            float cb = cosf(theta);

            normals[idx] = ca * cb;
            vertices[idx] = r * normals[idx];
            idx++;
            normals[idx] = sb;
            vertices[idx] = r * normals[idx];
            idx++;
            normals[idx] = -sa * cb;
            vertices[idx] = r * normals[idx];
            idx++;
        }
    }

    for (int i = 0; i < slices; i++)
    {
        for (int j = 0; j < stacks; j++)
        {
            int A = i * (stacks + 1) + j;
            int B = i * (stacks + 1) + j + 1;
            int C = (i + 1) * (stacks + 1) + j;
            int D = (i + 1) * (stacks + 1) + j + 1;

            indices[iidx++] = A;
            indices[iidx++] = B;
            indices[iidx++] = C;

            indices[iidx++] = B;
            indices[iidx++] = D;
            indices[iidx++] = C;
        }
    }
}

void getWaterHeight(float deltaH, GLfloat *vertices, int stacks, int slices, float r)
{
    int vidx = 0;
    vec3 F = {0.0f, 1.0f, 0.0f};
    glm_normalize(F);
    for (int i = 0; i <= slices; i++)
    {
        float phi = (float)i * M_PI / slices;
        for (int j = 0; j <= stacks; j++)
        {
            float theta = (float)j * 2.0f * M_PI / stacks;
            vec3 normal = {cosf(phi) * cosf(theta), sinf(theta), -sinf(phi) * cosf(theta)};
            glm_normalize(normal);
            float cos_gamma = glm_dot(F, normal);
            float level_change = deltaH * (3 * cos_gamma * cos_gamma - 1) / 2;
            vertices[vidx++] += normal[0] * (r + level_change);
            vertices[vidx++] += normal[1] * (r + level_change);
            vertices[vidx++] += normal[2] * (r + level_change);
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