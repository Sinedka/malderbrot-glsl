// clang-format off
#define CL_TARGET_OPENCL_VERSION 200
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <CL/cl.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <cmath>
// clang-format on

// --- Параметры окна и Мандельброта ---
const int WIDTH = 800;
const int HEIGHT = 600;
int MAX_ITER = 500;
double centerX = -0.5, centerY = 0.0, zoom = 2.0;

// --- OpenCL ядро ---
const char *mandelbrotKernel = R"(
#ifdef cl_khr_fp64
#pragma OPENCL EXTENSION cl_khr_fp64 : enable
#elif defined(cl_amd_fp64)
#pragma OPENCL EXTENSION cl_amd_fp64 : enable
#else
#error "Double precision floating point not supported by OpenCL implementation."
#endif

__kernel void mandelbrot(
    __global uchar4* image,
    const int width,
    const int height,
    const double centerX,
    const double centerY,
    const double zoom,
    const int maxIter)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
    double scale = zoom / (double)height;
    double real = centerX + (x - width/2.0) * scale;
    double imag = centerY + (y - height/2.0) * scale;
    double zr = 0.0, zi = 0.0;
    int iter = 0;
    while(zr*zr + zi*zi < 4.0 && iter < maxIter){
        double tmp = zr*zr - zi*zi + real;
        zi = 2.0*zr*zi + imag;
        zr = tmp;
        iter++;
    }
    float t = (float)iter / maxIter;
    uchar r = (uchar)(9*(1-t)*t*t*t*255);
    uchar g = (uchar)(15*(1-t)*(1-t)*t*t*255);
    uchar b = (uchar)(8.5*(1-t)*(1-t)*(1-t)*t*255);
    image[y*width + x] = (uchar4)(r,g,b,255);
}
)";

// --- Создание OpenGL текстуры ---
GLuint createTexture(int w, int h) {
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return tex;
}

// --- GLFW обработка ввода ---
void processInput(GLFWwindow *window) {
    double moveSpeed = zoom * 0.01;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) centerY += moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) centerY -= moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) centerX -= moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) centerX += moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) zoom *= 0.95;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) zoom *= 1.05;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) MAX_ITER = std::min(MAX_ITER + 5, 1000);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) MAX_ITER = std::max(MAX_ITER - 5, 10);
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        centerX = -0.5;
        centerY = 0.0;
        zoom = 2.0;
        MAX_ITER = 500;
    }
}

int main() {
    // --- GLFW + OpenGL ---
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Mandelbrot OpenCL+OpenGL", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    gladLoadGL();

    GLuint texture = createTexture(WIDTH, HEIGHT);

    // --- OpenCL init ---
    cl_int err;
    cl_platform_id platform;
    clGetPlatformIDs(1, &platform, nullptr);
    cl_device_id device;
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, nullptr);
    cl_context context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &err);
    cl_command_queue queue = clCreateCommandQueue(context, device, 0, &err);

    cl_program program = clCreateProgramWithSource(context, 1, &mandelbrotKernel, nullptr, &err);
    err = clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        size_t logSize;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize);
        std::string log(logSize, '\0');
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, logSize, &log[0], nullptr);
        std::cerr << log << std::endl;
        return -1;
    }

    cl_kernel kernel = clCreateKernel(program, "mandelbrot", &err);
    std::vector<cl_uchar4> buffer(WIDTH * HEIGHT);

    // --- Основной цикл ---
    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        cl_mem clBuffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_COPY_HOST_PTR,
                                         sizeof(cl_uchar4) * buffer.size(), buffer.data(), &err);

        clSetKernelArg(kernel, 0, sizeof(cl_mem), &clBuffer);
        clSetKernelArg(kernel, 1, sizeof(int), &WIDTH);
        clSetKernelArg(kernel, 2, sizeof(int), &HEIGHT);
        clSetKernelArg(kernel, 3, sizeof(double), &centerX);
        clSetKernelArg(kernel, 4, sizeof(double), &centerY);
        clSetKernelArg(kernel, 5, sizeof(double), &zoom);
        clSetKernelArg(kernel, 6, sizeof(int), &MAX_ITER);

        size_t global[2] = {WIDTH, HEIGHT};
        clEnqueueNDRangeKernel(queue, kernel, 2, nullptr, global, nullptr, 0, nullptr, nullptr);
        clEnqueueReadBuffer(queue, clBuffer, CL_TRUE, 0, sizeof(cl_uchar4) * buffer.size(), buffer.data(), 0, nullptr, nullptr);
        clReleaseMemObject(clBuffer);

        // --- Загрузка в OpenGL текстуру ---
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, buffer.data());

        // --- Рендеринг через современные OpenGL ---
        glClear(GL_COLOR_BUFFER_BIT);

        static GLuint vao = 0, vbo = 0;
        if (!vao) {
            float vertices[] = {
                // x, y, u, v
                -1.0f, -1.0f, 0.0f, 0.0f,
                 1.0f, -1.0f, 1.0f, 0.0f,
                -1.0f,  1.0f, 0.0f, 1.0f,
                 1.0f,  1.0f, 1.0f, 1.0f
            };
            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        }

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    return 0;
}

