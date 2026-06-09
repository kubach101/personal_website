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
    "uniform mat4 uMVP;\n"
    "uniform mat4 uModel;\n"
    "out vec3 vPos;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = uMVP * vec4(aPos, 1.0);\n"
    "    vPos = vec3(uModel * vec4(aPos, 1.0));\n"
    "}\n";

const char *fragmentShaderSource =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec3 vPos;\n"
    "uniform vec3 uLDir;\n"
    "void main()\n"
    "{\n"
    "    vec3 n = normalize(cross(dFdx(vPos), dFdy(vPos)));\n"
    "    float diff = max(dot(n, uLDir), 0.0);\n"
    "    vec3 c = vec3(0.0, 0.6, 1.0);\n"
    "    FragColor = vec4(c * diff, 1.0);\n"
    "}\n";

typedef struct
{
    float mass;
    vec3 pos, v, a;
    GLuint VAO, VBO, EBO;
    GLfloat *vertices;
    GLuint *indices;
    unsigned int v_num, i_num;
    GLuint shader_prog;
} body;
void sendData(GLuint VAO, GLuint VBO, GLuint EBO, GLfloat *vertices, GLuint *indices, unsigned int v_num, unsigned int i_num, GLenum usage);
void cleanup(GLuint VAO, GLuint VBO, GLuint EBO);
GLuint createShaderProgram(const char *vertexShaderSource, const char *fragmentShaderSource);
void DrawSphere(GLfloat **vertices, GLuint **indices, float r, int stacks, int slices, unsigned int *v_num, unsigned int *i_num);

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    body core;
    float r = 0.2f;
    int stacks = 16;
    int slices = 16;

    DrawSphere(&core.vertices, &core.indices, r, stacks, slices, &core.v_num, &core.i_num);

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

    sendData(core.VAO, core.VBO, core.EBO,
             core.vertices, core.indices,
             core.v_num, core.i_num, GL_STATIC_DRAW);

    glClearColor(0.54, 0.17, 0.89, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glfwSwapBuffers(window);

    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.54, 0.17, 0.89, 1.0f);
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
        glm_rotate(model, angle, (vec3){1.0f, 1.0f, 1.0f});

        mat4 mvp;
        glm_mat4_mul(view_proj, model, mvp);
        glUseProgram(core.shader_prog);
        glUniformMatrix4fv(glGetUniformLocation(core.shader_prog, "uMVP"), 1, GL_FALSE, (float *)mvp);
        glUniformMatrix4fv(glGetUniformLocation(core.shader_prog, "uModel"), 1, GL_FALSE, (float *)model);
        glUniform3f(glGetUniformLocation(core.shader_prog, "uLDir"), 0.577f, 0.577f, 0.577f);
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

void DrawSphere(GLfloat **vertices, GLuint **indices, float r, int stacks, int slices, unsigned int *v_num, unsigned int *i_num)
{
    *v_num = (stacks + 1) * (slices + 1) * 3;
    *i_num = stacks * slices * 6;

    *vertices = malloc(sizeof(GLfloat) * (*v_num));
    *indices = malloc(sizeof(GLuint) * (*i_num));

    int vidx = 0;
    int iidx = 0;

    for (int i = 0; i <= slices; i++)
    {
        float alpha = (float)i * M_PI / slices;
        float sa = sinf(alpha);
        float ca = cosf(alpha);

        for (int j = 0; j <= stacks; j++)
        {
            float beta = (float)j * 2.0f * M_PI / stacks;
            float sb = sinf(beta);
            float cb = cosf(beta);

            (*vertices)[vidx++] = r * sa * cb;
            (*vertices)[vidx++] = r * ca;
            (*vertices)[vidx++] = r * sa * sb;
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

            (*indices)[iidx++] = A;
            (*indices)[iidx++] = B;
            (*indices)[iidx++] = C;

            (*indices)[iidx++] = B;
            (*indices)[iidx++] = D;
            (*indices)[iidx++] = C;
        }
    }
}

GLuint createShaderProgram(const char *vertexShaderSource, const char *fragmentShaderSource)
{
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertex_shader);

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER); // fragment
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

void sendData(GLuint VAO, GLuint VBO, GLuint EBO, GLfloat *vertices, GLuint *indices, unsigned int v_num, unsigned int i_num, GLenum usage)
{
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * v_num, vertices, usage);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * i_num, indices, usage);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void *)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void cleanup(GLuint VAO, GLuint VBO, GLuint EBO)
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}