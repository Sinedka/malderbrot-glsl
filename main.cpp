#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>

// Размеры окна
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

// Параметры множества Мандельброта
struct MandelbrotParams {
    double centerX = -0.5;
    double centerY = 0.0;
    double zoom = 2.0;
    int maxIterations = 100;
} params;

// Vertex shader source
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
out vec2 fragCoord;

void main()
{
    fragCoord = aPos;
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
}
)";

// Fragment shader source
const char* fragmentShaderSource = R"(
#version 330 core
in vec2 fragCoord;
out vec4 FragColor;

uniform vec2 u_resolution;
uniform vec2 u_center;
uniform float u_zoom;
uniform int u_maxIterations;

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

int mandelbrot(vec2 c) {
    vec2 z = vec2(0.0);
    int iterations = 0;
    
    for (int i = 0; i < u_maxIterations; i++) {
        if (dot(z, z) > 4.0) break;
        z = vec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y) + c;
        iterations++;
    }
    
    return iterations;
}

void main() {
    // Преобразование координат экрана в комплексную плоскость
    vec2 uv = (fragCoord * 0.5 + 0.5) * u_resolution;
    vec2 c = (uv - u_resolution * 0.5) / min(u_resolution.x, u_resolution.y) * u_zoom + u_center;
    
    int iterations = mandelbrot(c);
    
    if (iterations == u_maxIterations) {
        // Точка принадлежит множеству - черный цвет
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        // Цветовая схема на основе количества итераций
        float t = float(iterations) / float(u_maxIterations);
        
        // HSV цвета для красивого градиента
        vec3 color = hsv2rgb(vec3(t * 6.0 + 0.1, 0.8, 1.0));
        FragColor = vec4(color, 1.0);
    }
}
)";

// Функция обработки ошибок GLFW
void glfwErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
}

// Функция обработки изменения размера окна
void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Функция обработки клавиатуры
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    
    double moveSpeed = params.zoom * 0.01;
    
    // Перемещение
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        params.centerY += moveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        params.centerY -= moveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        params.centerX -= moveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        params.centerX += moveSpeed;
    }
    
    // Масштабирование
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        params.zoom *= 0.95;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        params.zoom *= 1.05;
    }
    
    // Изменение количества итераций
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        params.maxIterations = std::min(params.maxIterations + 5, 1000);
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        params.maxIterations = std::max(params.maxIterations - 5, 10);
    }
    
    // Сброс параметров
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        params.centerX = -0.5;
        params.centerY = 0.0;
        params.zoom = 2.0;
        params.maxIterations = 100;
    }
}

// Функция компиляции шейдера
unsigned int compileShader(unsigned int type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation failed: " << infoLog << std::endl;
    }
    
    return shader;
}

// Функция создания шейдерной программы
unsigned int createShaderProgram() {
    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    int success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Shader program linking failed: " << infoLog << std::endl;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return shaderProgram;
}

int main() {
    // Инициализация GLFW
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    // Настройка GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // Создание окна
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Множество Мандельброта", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    
    // Инициализация GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
    // Создание шейдерной программы
    unsigned int shaderProgram = createShaderProgram();
    
    // Вершины для полноэкранного квада
    float vertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f,
        -1.0f, -1.0f
    };
    
    // Создание VAO и VBO
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Получение uniform локаций
    int resolutionLoc = glGetUniformLocation(shaderProgram, "u_resolution");
    int centerLoc = glGetUniformLocation(shaderProgram, "u_center");
    int zoomLoc = glGetUniformLocation(shaderProgram, "u_zoom");
    int maxIterationsLoc = glGetUniformLocation(shaderProgram, "u_maxIterations");
    
    // Основной цикл рендеринга
    while (!glfwWindowShouldClose(window)) {
        processInput(window);
        
        // Получение размеров окна
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        
        // Очистка экрана
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Использование шейдерной программы
        glUseProgram(shaderProgram);
        
        // Установка uniform переменных
        glUniform2f(resolutionLoc, (float)width, (float)height);
        glUniform2f(centerLoc, (float)params.centerX, (float)params.centerY);
        glUniform1f(zoomLoc, (float)params.zoom);
        glUniform1i(maxIterationsLoc, params.maxIterations);
        
        // Рендеринг
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // Очистка ресурсов
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    
    glfwTerminate();
    return 0;
}
