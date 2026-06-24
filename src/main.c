#include <glad/glad.h>
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
    "    for(int i =0; i < 3; i++){\n"
    "        float minCol = fragCol[i]*0.3;\n"
    "        fragCol[i] = fragCol[i]*0.7;\n"
    "        fragCol[i] *= diff;\n"
    "        fragCol[i] += minCol;\n"
    "     }\n"
    "}\n";

typedef struct
{
    GLfloat *vertices;
    GLuint *indices;
    int res;
    GLuint VAO, VBO, EBO;
} Shape;

typedef struct
{
    float mass, dist, rad, ang, ang_vel;
    vec3 orb_axis;
    vec4 color;
    Shape *shape;
} Satelite;

typedef struct
{
    float rad, base_h, h_max, h_min, friction, deviation;
    vec4 color;
    Shape shape;
} Ocean;

typedef struct
{
    float mass, rad;
    Shape *shape;
    vec4 color;
} Planet;

typedef struct
{
    char top[128];
    char bottom[128];
} Tab;

enum RenderType
{
    SOLID = 0,
    FLUID = 1
};

void sendData(Shape *s, GLenum usage);
void resendData(Shape *s);
void cleanup(GLuint VAO, GLuint VBO, GLuint EBO);
GLuint createShaderProgram(const char *vertexShaderSource, const char *fragmentShaderSource);
void CreateSphere(Shape *Shape);
void ShapeWaterLayer(Ocean *ocean, Satelite *satelite, Planet *planet, unsigned int OceanScale, unsigned int TideScale);
void UpdateProjView(mat4 proj_view, float aspect, vec3 eye, vec3 up);
void RotateEye(vec3 eye, vec3 up, vec3 eye0, vec3 up0, vec2 eye_ang);
void printInfoTab(Satelite *satelite, Ocean *ocean, Planet *planet, float time);
void Render(Satelite *satelite, Ocean *ocean, Planet *planet, GLuint shader_prog, vec3 ldir, mat4 proj_view);

int main()
{
    unsigned int Tscale = 10000;
    unsigned int OceanScale = 200;
    unsigned int TideScale = 10000;

    Shape sphere = {0};
    sphere.res = 64;
    size_t v_size = sizeof(GLfloat) * (sphere.res + 1) * (2 * sphere.res + 1) * 3;
    size_t i_size = sizeof(GLuint) * sphere.res * sphere.res * 2 * 6;
    sphere.vertices = malloc(v_size);
    sphere.indices = malloc(i_size);
    CreateSphere(&sphere);

    Satelite satelite = {7.35f, 3.85f, 0.017374f, 0.0f, 0.0f, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.333f, 1.0f}, &sphere};
    Planet planet = {597.2f, 0.06378f, &sphere, {1.0f, 0.0f, 0.0f, 1.0f}};
    Ocean ocean = {planet.rad, 3700.0f / Runit, 0.0f, 0.0f, 0.0f, 0.0f, {0.0f, 0.0f, 1.0f, 0.5f}, {0}};
    ocean.rad += ocean.base_h;
    ocean.shape.res = 64;
    v_size = sizeof(GLfloat) * (ocean.shape.res + 1) * (2 * ocean.shape.res + 1) * 3;
    i_size = sizeof(GLuint) * ocean.shape.res * ocean.shape.res * 2 * 6;
    ocean.shape.vertices = malloc(v_size);
    ocean.shape.indices = malloc(i_size);
    CreateSphere(&ocean.shape);

    char ans = '\0';
    printf("Set custom paramters(y/n):");
    scanf("%c", &ans);
    printf("\033[1A\033[J");
    if (ans == 'y' || ans == 'Y')
    {
        printf("Input planet mass(e8 kg):");
        scanf("%f", &planet.mass);
        printf("\033[1A\033[J");
        printf("Input satelite mass(e8 kg):");
        scanf("%f", &satelite.mass);
        printf("\033[1A\033[J");
        printf("Input planet radius(e22 m):");
        scanf("%f", &planet.rad);
        printf("\033[1A\033[J");
        printf("Input satelite radius(e22 m):");
        scanf("%f", &satelite.rad);
        printf("\033[1A\033[J");
        printf("Input ocean base height(m):");
        scanf("%f", &ocean.base_h);
        printf("\033[1A\033[J");
        ocean.base_h /= Runit;
        ocean.rad = planet.rad + ocean.base_h;
    }
    ans = '\0';
    printf("Set custom scaling(y/n):");
    scanf(" %c", &ans);
    printf("\033[1A\033[J");
    if (ans == 'y' || ans == 'Y')
    {
        printf("Input time acceleration:");
        scanf("%u", &Tscale);
        printf("\033[1A\033[J");
        printf("Input rendered ocean scale:");
        scanf("%u", &OceanScale);
        printf("\033[1A\033[J");
        printf("Input visual tide scale:");
        scanf("%u", &TideScale);
        printf("\033[1A\033[J");
    }

    printf("base_h = %f\n", ocean.base_h);
    printf("plant: rad = %f, mass = %f\n", planet.rad, planet.mass);
    printf("satelite: rad = %f, mass = %f\n", satelite.rad, satelite.mass);

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow *window = glfwCreateWindow(800, 800, "Tidal Simulation by Kuba Chmura", NULL, NULL);
    if (window == NULL)
    {
        printf("Error creating a window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        printf("Failed to initialize GLAD\n");
        return -1;
    }
    glViewport(0, 0, 800, 800);
    glEnable(GL_DEPTH_TEST);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glfwSwapBuffers(window);

    glGenVertexArrays(1, &sphere.VAO);
    glGenBuffers(1, &sphere.VBO);
    glGenBuffers(1, &sphere.EBO);
    sendData(&sphere, GL_STATIC_DRAW);

    glGenVertexArrays(1, &ocean.shape.VAO);
    glGenBuffers(1, &ocean.shape.VBO);
    glGenBuffers(1, &ocean.shape.EBO);
    sendData(&ocean.shape, GL_DYNAMIC_DRAW);

    GLuint shader_prog = createShaderProgram(vert, frag);

    bool reshape = true;
    bool update_vision = false;

    mat4 proj_view;
    vec3 eye0 = {0.0f, 0.0f, 15.0f};
    vec3 up0 = {0.0f, 1.0f, 0.0f};
    vec2 eye_ang = {0};
    vec3 eye, up;
    UpdateProjView(proj_view, 1.0f, eye0, up0);

    vec3 ldir = {1.0f, 1.0f, 0.0f};
    glm_normalize(ldir);

    glfwSetTime(0);
    float dt = 0.0f;
    float time = 0.0f;
    while (!glfwWindowShouldClose(window))
    {
        dt = glfwGetTime();
        glfwSetTime(0);

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (reshape)
        {
            ShapeWaterLayer(&ocean, &satelite, &planet, OceanScale, TideScale);
            resendData(&ocean.shape);
        }
        satelite.ang_vel = 1 / AVunit * sqrt(G * planet.mass / satelite.dist / satelite.dist / satelite.dist) / Tunit;
        satelite.ang += satelite.ang_vel * dt * Tscale * AVunit;
        satelite.ang = fmod(satelite.ang, 2 * M_PI);

        if (update_vision)
        {

            eye_ang[0] = fmod(eye_ang[0], 2 * M_PI);
            eye_ang[1] = fmod(eye_ang[1], 2 * M_PI);

            RotateEye(eye, up, eye0, up0, eye_ang);
            UpdateProjView(proj_view, 1.0f, eye, up);
            update_vision = false;
        }

        Render(&satelite, &ocean, &planet, shader_prog, ldir, proj_view);

        glfwSwapBuffers(window);

        time += dt / Tunit;
        // printInfoTab(&satelite, &ocean, &planet, time);

        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
        {
            eye0[2] -= 0.005f;
            update_vision = true;
        }
        if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
        {
            eye0[2] += 0.005f;
            update_vision = true;
        }
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        {
            eye_ang[1] -= 0.3f / 180.0f * M_PI;
            update_vision = true;
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        {
            eye_ang[1] += 0.3f / 180.0f * M_PI;
            update_vision = true;
        }
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        {
            eye_ang[0] -= 0.3f / 180.0f * M_PI;
            update_vision = true;
        }
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        {
            eye_ang[0] += 0.3f / 180.0f * M_PI;
            update_vision = true;
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    free(sphere.vertices);
    free(sphere.indices);

    free(ocean.shape.vertices);
    free(ocean.shape.indices);

    return 0;
}

void CreateSphere(Shape *s)
{

    int idx = 0;
    int iidx = 0;
    int stacks = s->res;
    int slices = 2 * s->res;
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
            s->vertices[idx] = sp * ct;
            idx++;
            s->vertices[idx] = cp;
            idx++;
            s->vertices[idx] = sp * st;
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

            s->indices[iidx++] = a;
            s->indices[iidx++] = b;
            s->indices[iidx++] = c;

            s->indices[iidx++] = b;
            s->indices[iidx++] = d;
            s->indices[iidx++] = c;
        }
    }
}

void ShapeWaterLayer(Ocean *ocean, Satelite *satelite, Planet *planet, unsigned int OceanScale, unsigned int TideScale)
{
    int vidx = 0;
    vec3 fdir = {0};
    fdir[0] = -satelite->dist;
    glm_normalize(fdir);
    glm_vec3_rotate(fdir, satelite->ang, satelite->orb_axis);

    ocean->h_min = FLT_MAX;
    ocean->h_max = -FLT_MAX;
    for (int i = 0; i <= ocean->shape.res; i++)
    {
        float phi = (float)i * M_PI / ocean->shape.res;
        for (int j = 0; j <= ocean->shape.res * 2; j++)
        {
            float theta = (float)j * 2.0f * M_PI / ocean->shape.res / 2;
            vec3 normal = {sinf(phi) * cosf(theta), cosf(phi), sinf(phi) * sinf(theta)};
            glm_normalize(normal);
            float cos_gamma = glm_dot(fdir, normal);
            float tide = 0.5f * satelite->mass / planet->mass * planet->rad * planet->rad * planet->rad * planet->rad / satelite->dist / satelite->dist / satelite->dist * (3 * cos_gamma * cos_gamma - 1);
            float h = ocean->base_h + tide;
            if (h > ocean->h_max)
            {
                ocean->h_max = h;
            }
            if (h < ocean->h_min)
            {
                ocean->h_min = h;
            }
            float render_h = ocean->base_h * OceanScale + (tide * TideScale * OceanScale);
            ocean->shape.vertices[vidx++] = normal[0] * (planet->rad + render_h);
            ocean->shape.vertices[vidx++] = normal[1] * (planet->rad + render_h);
            ocean->shape.vertices[vidx++] = normal[2] * (planet->rad + render_h);
        }
    }
    ocean->h_max *= Runit;
    ocean->h_min *= Runit;
}

void UpdateProjView(mat4 proj_view, float aspect, vec3 eye, vec3 up)
{
    mat4 view, proj;
    glm_perspective(glm_rad(45.0f), 800.0f / 800.0f, 0.1f, 25.0f, proj);
    glm_lookat(eye, (vec3){0.0f, 0.0f, 0.0f}, up, view);
    glm_mat4_mul(proj, view, proj_view);
}

void Render(Satelite *satelite, Ocean *ocean, Planet *planet, GLuint shader_prog, vec3 ldir, mat4 proj_view)
{
    mat4 model, tmp, mvp;
    mat3 norm_model;
    vec3 pos = {0};
    pos[0] = satelite->dist;
    glm_mat4_identity(model);
    glm_rotate(model, satelite->ang, satelite->orb_axis);
    glm_translate(model, pos);
    glm_scale_uni(model, satelite->rad);
    glm_mat4_mul(proj_view, model, mvp);
    glm_mat4_inv(model, tmp);
    glm_mat4_transpose(tmp);
    glm_mat4_pick3(tmp, norm_model);

    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    glUseProgram(shader_prog);
    glUniformMatrix4fv(glGetUniformLocation(shader_prog, "uMVP"), 1, GL_FALSE, (float *)mvp);
    glUniformMatrix3fv(glGetUniformLocation(shader_prog, "uNormMat"), 1, GL_FALSE, (float *)norm_model);
    glUniform4f(glGetUniformLocation(shader_prog, "uCol"), satelite->color[0], satelite->color[1], satelite->color[2], satelite->color[3]);
    glUniform3f(glGetUniformLocation(shader_prog, "uLDir"), ldir[0], ldir[1], ldir[2]);
    glBindVertexArray(satelite->shape->VAO);
    glDrawElements(GL_TRIANGLES, satelite->shape->res * 2 * satelite->shape->res * 6, GL_UNSIGNED_INT, 0);

    mat3 ident;
    glm_mat3_identity(ident);

    glm_mat4_identity(model);
    glm_scale_uni(model, planet->rad);
    glm_mat4_mul(proj_view, model, mvp);
    glUseProgram(shader_prog);
    glUniformMatrix4fv(glGetUniformLocation(shader_prog, "uMVP"), 1, GL_FALSE, (float *)mvp);
    glUniformMatrix3fv(glGetUniformLocation(shader_prog, "uNormMat"), 1, GL_FALSE, (float *)ident);
    glUniform4f(glGetUniformLocation(shader_prog, "uCol"), planet->color[0], planet->color[1], planet->color[2], planet->color[3]);
    glUniform3f(glGetUniformLocation(shader_prog, "uLDir"), ldir[0], ldir[1], ldir[2]);
    glBindVertexArray(planet->shape->VAO);
    glDrawElements(GL_TRIANGLES, planet->shape->res * 2 * planet->shape->res * 6, GL_UNSIGNED_INT, 0);

    glm_mat4_identity(model);
    // glm_scale_uni(model, ocean->rad);
    glm_mat4_mul(proj_view, model, mvp);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // glEnable(GL_POLYGON_OFFSET_FILL);
    // glPolygonOffset(-1.0f, -1.0f);
    glUseProgram(shader_prog);
    glUniformMatrix4fv(glGetUniformLocation(shader_prog, "uMVP"), 1, GL_FALSE, (float *)mvp);
    glUniformMatrix3fv(glGetUniformLocation(shader_prog, "uNormMat"), 1, GL_FALSE, (float *)ident);
    glUniform4f(glGetUniformLocation(shader_prog, "uCol"), ocean->color[0], ocean->color[1], ocean->color[2], ocean->color[3]);
    glUniform3f(glGetUniformLocation(shader_prog, "uLDir"), ldir[0], ldir[1], ldir[2]);
    glBindVertexArray(ocean->shape.VAO);
    glDrawElements(GL_TRIANGLES, ocean->shape.res * 2 * ocean->shape.res * 6, GL_UNSIGNED_INT, 0);
    // glDisable(GL_POLYGON_OFFSET_FILL);
}

void RotateEye(vec3 eye, vec3 up, vec3 eye0, vec3 up0, vec2 eye_ang)
{
    memcpy(eye, eye0, sizeof(vec3));
    memcpy(up, up0, sizeof(vec3));

    glm_vec3_rotate(eye, eye_ang[0], (vec3){1.0f, 0.0f, 0.0f});
    glm_vec3_rotate(up, eye_ang[0], (vec3){1.0f, 0.0f, 0.0f});

    glm_vec3_rotate(eye, eye_ang[1], (vec3){0.0f, 1.0f, 0.0f});
    glm_vec3_rotate(up, eye_ang[1], (vec3){0.0f, 1.0f, 0.0f});
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

    GLint frag_succes;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &frag_succes);
    if (!vert_success)
    {
        char log[512];
        glGetShaderInfoLog(vertex_shader, 512, NULL, log);
        printf("VERT ERROR: %s\n", log);
    }

    GLuint shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return shader_program;
}

void sendData(Shape *s, GLenum usage)
{
    size_t v_size = sizeof(GLfloat) * (s->res + 1) * (2 * s->res + 1) * 3;
    size_t i_size = sizeof(GLuint) * s->res * s->res * 2 * 6;
    glBindVertexArray(s->VAO);

    glBindBuffer(GL_ARRAY_BUFFER, s->VBO);
    glBufferData(GL_ARRAY_BUFFER, v_size, s->vertices, usage);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void *)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, i_size, s->indices, usage);

    glBindVertexArray(0);
}
void resendData(Shape *s)
{

    glBindBuffer(GL_ARRAY_BUFFER, s->VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat) * (s->res + 1) * (2 * s->res + 1) * 3, s->vertices);

    glBindVertexArray(0);
}

void cleanup(GLuint VAO, GLuint VBO, GLuint EBO)
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void printInfoTab(Satelite *satelite, Ocean *ocean, Planet *planet, float time)
{
    static const char sep[] = "////////////////////////////////////////////";
    static const char satelite_title[] = "Satelite\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\";
    static const char ocean_title[] = "Ocean\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\";
    static const char planet_title[] = "Planet\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\";
    static char buf[3072];
    int len = 0;

    len += snprintf(buf + len, sizeof(buf) - len, "\033[20A\033[J");

    len += snprintf(buf + len, sizeof(buf) - len,
                    "\033[93m%s\033[0m\n"
                    "distance: %.2fe8 m\n"
                    "angle: %f rad\n"
                    "angular velocity: %.4fe-6 rad/s\n"
                    "\033[93m%s\033[0m\n\n",
                    satelite_title,
                    satelite->dist, satelite->ang, satelite->ang_vel, sep);

    len += snprintf(buf + len, sizeof(buf) - len,
                    "\033[96m%s\033[0m\n"
                    "Hmax: %.2f m\n"
                    "Hmin: %.2f m\n"
                    "friction: %s\n"
                    "\033[96m%s\033[0m\n\n",
                    ocean_title, ocean->h_max,
                    ocean->h_min,
                    ocean->friction ? "ON" : "OFF", sep);

    len += snprintf(buf + len, sizeof(buf) - len,
                    "\033[95m%s\033[0m\n"
                    "radius: %fe8 m\n"
                    "g: %f N/kg\n"
                    "\033[95m%s\033[0m\n",
                    planet_title, planet->rad, planet->mass * G * Aunit / planet->rad / planet->rad, sep);

    len += snprintf(buf + len, sizeof(buf) - len, "\nTime passed: %fe6 s\n", time);
    fwrite(buf, 1, len, stdout);
    fflush(stdout);
}