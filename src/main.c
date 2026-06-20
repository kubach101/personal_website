#include <glad/glad.h>
#include <stdio.h>
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define Tscale 1.0E3
#define Mscale 1.0E22
#define Rscale 1.0E8
#define Ascale Rscale / Tscale / Tscale
#define Fscale Mscale *Ascale
#define G 6.67E-11 * Rscale * Rscale * Rscale / (Mscale * Tscale * Tscale)
#define TideAccel 50000
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
    vec3 pos0, pos;
    mat4 mvp;
    mat3 nmodel;
    vec4 col;
    int mode;
} Obj;

typedef struct
{
    vec3 pos;
    float d, ang, ang_v;
} SateliteInfo;

typedef struct
{
    float hmax, hmin;
    vec3 hmax_pos, hmin_pos;
    bool friction;
} OceanInfo;

typedef struct
{
    char top[60];
    char bottom[60];
} Tab;

enum RenderType
{
    SOLID = 0,
    FLUID = 1
};

void sendData(Obj object, GLenum usage);
void cleanup(GLuint VAO, GLuint VBO, GLuint EBO);
GLuint createShaderProgram(const char *vertexShaderSource, const char *fragmentShaderSource);
void DrawSphere(GLfloat *vertices, GLuint *indices, float r, int stacks, int slices);
void ShapeWaterLayer(Obj *planet, Obj *satelite, Obj *water, SateliteInfo *sinfo, OceanInfo *oinfo);
void Modify(mat4 proj_view, Obj *o);
void Render(Obj *o, vec3 ldir);
float AngVel(Obj *satelite, Obj *planet);
void GetPositionOrbit(Obj *o, float angle, vec3 axis, vec3 axis_pos);
void UpdateProjView(mat4 proj_view, float aspect, vec3 eye, vec3 up);
void RotateEye(vec3 eye, vec3 up, vec3 eye0, vec3 up0, vec2 ang_xy);
void printInfoTab(SateliteInfo *si, OceanInfo *oi);

int main()
{

    // planet core:
    Obj core = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 597.2f, 0.06378f, {0.0f, 0.0f, 0.0f}, {0}, {0}, {0}, {1.0f, 0.0f, 0.0f, 1.0f}, SOLID};
    core.r = 0.3f;
    core.stacks = 36;
    core.slices = 36;
    core.v_num = (core.stacks + 1) * (core.slices + 1) * 3;
    core.i_num = core.stacks * core.slices * 6;
    core.vertices = malloc(sizeof(GLfloat) * core.v_num);
    core.indices = malloc(sizeof(GLuint) * core.i_num);
    DrawSphere(core.vertices, core.indices, core.r, core.stacks, core.slices);

    // ocean:
    Obj ocean = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.0f, 0.167f, {0.0f, 0.0f, 0.0f}, {0}, {0}, {0}, {0.0f, 0.0f, 1.0f, 0.4f}, FLUID};
    ocean.r = 0.38f;
    ocean.stacks = 42;
    ocean.slices = 42;
    ocean.v_num = (ocean.stacks + 1) * (ocean.slices + 1) * 3;
    ocean.i_num = ocean.stacks * ocean.slices * 6;
    ocean.vertices = malloc(sizeof(GLfloat) * ocean.v_num);
    ocean.indices = malloc(sizeof(GLuint) * ocean.i_num);
    DrawSphere(ocean.vertices, ocean.indices, ocean.r, ocean.stacks, ocean.slices);

    // previous x = 3.85
    Obj moon = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7.35f, 0.017374f, {3.85f, 0.0f, 0.0f}, {0}, {0}, {0}, {0.0f, 1.0f, 0.0f, 1.0f}, SOLID};
    moon.r = 0.1f;
    moon.stacks = 24;
    moon.slices = 24;
    moon.v_num = (moon.stacks + 1) * (moon.slices + 1) * 3;
    moon.i_num = moon.stacks * moon.slices * 6;
    moon.vertices = malloc(sizeof(GLfloat) * moon.v_num);
    moon.indices = malloc(sizeof(GLuint) * moon.i_num);
    DrawSphere(moon.vertices, moon.indices, moon.r, moon.stacks, moon.slices);
    for (int i = 0; i < 3; i++)
    {
        moon.pos[i] = moon.pos0[i];
    }

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

    SateliteInfo sinfo;
    OceanInfo oinfo;

    bool reshape = true;
    bool update_vision = false;

    mat4 proj_view;
    vec3 eye0 = {0.0f, 0.0f, 15.0f};
    vec3 up0 = {0.0f, 1.0f, 0.0f};
    vec2 ang_xy = {0};
    vec3 eye, up;
    UpdateProjView(proj_view, 1.0f, eye0, up0);
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
            ShapeWaterLayer(&core, &moon, &ocean, &sinfo, &oinfo);
            sendData(ocean, GL_DYNAMIC_DRAW);
        }
        double omega = AngVel(&moon, &core);
        // printf("ang. vel. = %lf\n", omega);
        angle += omega * dt * Tscale * TimeAccel;
        // printf("angle = %f\n", angle);
        angle = fmod(angle, 2 * M_PI);
        GetPositionOrbit(&moon, angle, (vec3){0.0f, 1.0f, 0.0f}, (vec3){0.0f, 0.0f, 0.0f});
        // printf("moon_pos0: |%f|%f|%f|\n", moon.pos0[0], moon.pos0[1], moon.pos0[2]);
        // printf("moon_pos: |%f|%f|%f|\n", moon.pos[0], moon.pos[1], moon.pos[2]);
        if (update_vision)
        {
            ang_xy[0] = fmod(ang_xy[0], 2 * M_PI);
            ang_xy[1] = fmod(ang_xy[1], 2 * M_PI);

            RotateEye(eye, up, eye0, up0, ang_xy);
            UpdateProjView(proj_view, 1.0f, eye, up);
            update_vision = false;
        }

        // Creating MVP's and normal matrices:

        Modify(proj_view, &core);
        Modify(proj_view, &ocean);
        Modify(proj_view, &moon);

        Render(&core, ldir);
        Render(&ocean, ldir);
        Render(&moon, ldir);

        glfwSwapBuffers(window);

        sinfo.ang = angle;
        sinfo.ang_v = omega;
        sinfo.d = 0.0f;
        memcpy(sinfo.pos, moon.pos, 3 * sizeof(float));

        oinfo.friction = false;

        printInfoTab(&sinfo, &oinfo);

        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
        {
            eye0[2] -= 0.01f;
            // printf("recived: M\n");
            update_vision = true;
        }
        if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
        {
            eye0[2] += 0.01f;
            // printf("recived: N\n");
            update_vision = true;
        }
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        {
            ang_xy[1] -= 0.2f / 180.0f * M_PI;
            // printf("recived: arrowR\n");
            update_vision = true;
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        {
            ang_xy[1] += 0.2f / 180.0f * M_PI;
            // printf("recived: arrowL\n");
            update_vision = true;
        }
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        {
            ang_xy[0] -= 0.2f / 180.0f * M_PI;
            // printf("recived: arrowU\n");
            update_vision = true;
        }
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        {
            ang_xy[0] += 0.2f / 180.0f * M_PI;
            // printf("recived: arrowD\n");
            update_vision = true;
        }
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

void ShapeWaterLayer(Obj *planet, Obj *satelite, Obj *water, SateliteInfo *sinfo, OceanInfo *oinfo)
{
    float d = sqrt((planet->pos[0] - satelite->pos[0]) * (planet->pos[0] - satelite->pos[0]) + (planet->pos[1] - satelite->pos[1]) * (planet->pos[1] - satelite->pos[1]) + (planet->pos[2] - satelite->pos[2]) * (planet->pos[2] - satelite->pos[2]));
    float h0 = water->r - planet->r;
    sinfo->d = d;

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
            float h = TideAccel * 0.75f * satelite->mass / planet->mass * planet->r * planet->r * planet->r * planet->r / d / d / d * (cos_gamma * cos_gamma - 1);
            if (cos_gamma = 1.0f)
            {
                oinfo->hmax = h;
                oinfo->hmax_pos[0] = planet->r + h;
                oinfo->hmax_pos[1] = theta;
                oinfo->hmax_pos[2] = phi;
            }
            if (cos_gamma = 0, 707106781186547)
            {
                oinfo->hmin = h;
                oinfo->hmin_pos[0] = planet->r + h;
                oinfo->hmin_pos[1] = theta;
                oinfo->hmin_pos[2] = phi;
            }
            water->vertices[vidx++] = normal[0] * (water->r + h);
            water->vertices[vidx++] = normal[1] * (water->r + h);
            water->vertices[vidx++] = normal[2] * (water->r + h);
        }
    }
}

void Modify(mat4 proj_view, Obj *o)
{
    mat4 model;
    glm_mat4_identity(model);
    glm_translate(model, o->pos);
    glm_mat4_mul(proj_view, model, o->mvp);
    mat4 tmp;
    glm_mat4_inv(model, tmp);
    glm_mat4_transpose(tmp);
    glm_mat4_pick3(tmp, o->nmodel);
}

float AngVel(Obj *satelite, Obj *planet)
{
    /*for (int i = 0; i < 3; i++)
    {
        printf("pos/sat[%d]= %f | pos/plan[%d] = %f\n", i, satelite->pos[i], i, planet->pos[i]);
    }*/
    float d = sqrt((planet->pos[0] - satelite->pos[0]) * (planet->pos[0] - satelite->pos[0]) + (planet->pos[1] - satelite->pos[1]) * (planet->pos[1] - satelite->pos[1]) + (planet->pos[2] - satelite->pos[2]) * (planet->pos[2] - satelite->pos[2]));
    // printf("d = %f\n", d);
    float omega = sqrt(G * planet->mass / d);
    return omega;
}

void GetPositionOrbit(Obj *o, float angle, vec3 axis, vec3 axis_pos)
{

    for (int i = 0; i < 3; i++)
    {
        o->pos[i] = o->pos0[i] - axis_pos[i];
        // printf("pos[i] = %f\n", o->pos[i]);
    }
    glm_vec3_rotate(o->pos, angle, axis);

    for (int i = 0; i < 3; i++)
    {
        // printf("pos_rotated[i] = %f\n", o->pos[i]);
        o->pos[i] += axis_pos[i];
    }
}

void UpdateProjView(mat4 proj_view, float aspect, vec3 eye, vec3 up)
{
    mat4 view, proj;
    glm_perspective(glm_rad(45.0f), 800.0f / 800.0f, 0.1f, 100.0f, proj);
    glm_lookat(eye, (vec3){0.0f, 0.0f, 0.0f}, up, view);
    glm_mat4_mul(proj, view, proj_view);
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

void RotateEye(vec3 eye, vec3 up, vec3 eye0, vec3 up0, vec2 ang_xy)
{
    memcpy(eye, eye0, sizeof(vec3));
    memcpy(up, up0, sizeof(vec3));

    glm_vec3_rotate(eye, ang_xy[0], (vec3){1.0f, 0.0f, 0.0f});
    glm_vec3_rotate(up, ang_xy[0], (vec3){1.0f, 0.0f, 0.0f});

    glm_vec3_rotate(eye, ang_xy[1], (vec3){0.0f, 1.0f, 0.0f});
    glm_vec3_rotate(up, ang_xy[1], (vec3){0.0f, 1.0f, 0.0f});
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

void printInfoTab(SateliteInfo *sinfo, OceanInfo *oinfo)
{
    Tab stab = {{"Satelite\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\"},
                {"////////////////////////////////////////////"}};

    Tab otab = {{"Ocean\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\"},
                {"////////////////////////////////////////////"}};

    for (int i = 0; i < 14; i++)
    {
        printf("\033[1A\r");
        printf("                                                            ");
        fflush(stdin);
    }

    printf("\r%s\nposition: (%f;%f;%f)\ndistance: %f\nangle: %f\nangular velocity: %f\n%s\n\n", stab.top, sinfo->pos[0], sinfo->pos[1], sinfo->pos[2], sinfo->d, sinfo->ang, sinfo->ang_v, stab.bottom);
    fflush(stdin);
    printf("%s\nHmax: %f\nHmax-pos: (%f;%f;%f)\nHmin: %f\nHmin-pos:(%f;%f;%f)\nfriction: %s\n%s\n", otab.top, oinfo->hmax, oinfo->hmax_pos[0], oinfo->hmax_pos[1], oinfo->hmax_pos[2], oinfo->hmin, oinfo->hmin_pos[0], oinfo->hmin_pos[1], oinfo->hmin_pos[2], oinfo->friction ? "ON" : "OFF", otab.bottom);
    fflush(stdin);
}