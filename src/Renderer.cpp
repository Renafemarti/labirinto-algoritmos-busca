#define STB_TRUETYPE_IMPLEMENTATION
#define _USE_MATH_DEFINES
#include "Renderer.hpp"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <vector>
#include <string>

// Shaders 3D com Iluminação (Luz Direcional)
const char* vertexShader3D = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;

    out vec3 FragPos;
    out vec3 Normal;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main() {
        FragPos = vec3(model * vec4(aPos, 1.0));
        Normal = mat3(transpose(inverse(model))) * aNormal;  
        gl_Position = projection * view * vec4(FragPos, 1.0);
    }
)";

const char* fragmentShader3D = R"(
    #version 330 core
    out vec4 FragColor;

    in vec3 FragPos;
    in vec3 Normal;

    uniform vec4 objectColor;
    uniform vec3 lightDir;
    uniform vec3 viewPos;

    void main() {
        float ambientStrength = 0.4;
        vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0);
        
        vec3 norm = normalize(Normal);
        vec3 lightDirN = normalize(-lightDir);
        float diff = max(dot(norm, lightDirN), 0.0);
        vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);
        
        vec3 result = (ambient + diffuse) * objectColor.rgb;
        FragColor = vec4(result, objectColor.a);
    }
)";

// Shaders para renderizar as letras coladas no chão
const char* textVertexShader = R"(
    #version 330 core
    layout (location = 0) in vec4 vertex; // <x, z, u, v>
    out vec2 TexCoords;

    uniform mat4 view;
    uniform mat4 projection;

    void main() {
        // Altura y=0.031 deixa o texto pairando milímetros acima do tapete colorido
        gl_Position = projection * view * vec4(vertex.x, 0.031, vertex.y, 1.0);
        TexCoords = vertex.zw;
    }
)";

const char* textFragmentShader = R"(
    #version 330 core
    in vec2 TexCoords;
    out vec4 color;

    uniform sampler2D text;
    uniform vec3 textColor;

    void main() {
        // A textura do STB só tem 1 canal de cor (Red), usamos ele como opacidade (Alpha)
        vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
        color = vec4(textColor, 1.0) * sampled;
    }
)";

Renderer::Renderer(int screenWidth, int screenHeight, int marginPx)
    : screenWidth_(screenWidth), screenHeight_(screenHeight), margin_(marginPx),
      cameraYaw(45.0f), cameraPitch(45.0f), mazeWidth3D_(0), mazeDepth3D_(0) {}

void Renderer::initOpenGLResources() {
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShader3D, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShader3D, NULL);
    glCompileShader(fragmentShader);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    setupCube();
    setupSphere(18, 18);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Inicia os Shaders e texturas de fonte
    initText(); 
}

void Renderer::setupCube() {
    float vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
    };

    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void Renderer::setupSphere(int sectorCount, int stackCount) {
    std::vector<float> vertices;
    std::vector<int> indices;
    float x, y, z, xy;
    float nx, ny, nz, lengthInv = 1.0f;
    float sectorStep = 2 * M_PI / sectorCount;
    float stackStep = M_PI / stackCount;
    float sectorAngle, stackAngle;

    for(int i = 0; i <= stackCount; ++i) {
        stackAngle = M_PI / 2 - i * stackStep;        
        xy = 0.5f * cosf(stackAngle);              
        z = 0.5f * sinf(stackAngle);               
        for(int j = 0; j <= sectorCount; ++j) {
            sectorAngle = j * sectorStep;            
            x = xy * cosf(sectorAngle);              
            y = xy * sinf(sectorAngle);              
            
            nx = x * lengthInv; ny = y * lengthInv; nz = z * lengthInv;
            vertices.push_back(x); vertices.push_back(y); vertices.push_back(z);
            vertices.push_back(nx); vertices.push_back(ny); vertices.push_back(nz);
        }
    }

    int k1, k2;
    for(int i = 0; i < stackCount; ++i) {
        k1 = i * (sectorCount + 1);     
        k2 = k1 + sectorCount + 1;      
        for(int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
            if(i != 0) { indices.push_back(k1); indices.push_back(k2); indices.push_back(k1 + 1); }
            if(i != (stackCount-1)) { indices.push_back(k1 + 1); indices.push_back(k2); indices.push_back(k2 + 1); }
        }
    }
    sphereIndexCount = indices.size();

    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);
    
    glBindVertexArray(sphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), indices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void Renderer::applyCameraAndLight() const {
    glUseProgram(shaderProgram);

    float maxDim = std::max(mazeWidth3D_, mazeDepth3D_);
    float radius = maxDim * 1.2f * cameraZoom; 
    
    float camX = radius * cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
    float camY = radius * sin(glm::radians(cameraPitch));
    float camZ = radius * sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));

    glm::vec3 cameraPos = glm::vec3(camX, camY, camZ);
    glm::vec3 cameraTarget = glm::vec3(mazeWidth3D_ / 2.0f, 0.0f, mazeDepth3D_ / 2.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::mat4 view = glm::lookAt(cameraPos + cameraTarget, cameraTarget, cameraUp);
    
    float farPlane = maxDim * 5.0f < 1000.0f ? 1000.0f : maxDim * 5.0f;
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)screenWidth_ / (float)screenHeight_, 0.1f, farPlane);

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(cameraPos + cameraTarget));
    
    glm::vec3 lightDir = glm::vec3(-0.2f, -1.0f, -0.3f);
    glUniform3fv(glGetUniformLocation(shaderProgram, "lightDir"), 1, glm::value_ptr(lightDir));
}

void Renderer::drawCube(glm::vec3 position, glm::vec3 scale, glm::vec4 color) const {
    glUseProgram(shaderProgram); // MANTÉM O SHADER 3D ATIVO!

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, scale);

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform4fv(glGetUniformLocation(shaderProgram, "objectColor"), 1, glm::value_ptr(color));

    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void Renderer::drawSphere(glm::vec3 position, float radius, glm::vec4 color) const {
    glUseProgram(shaderProgram); // MANTÉM O SHADER 3D ATIVO!

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, glm::vec3(radius * 2.0f));

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform4fv(glGetUniformLocation(shaderProgram, "objectColor"), 1, glm::value_ptr(color));

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);
}

void Renderer::resize(int screenWidth, int screenHeight) {
    // screenHeight_ == 0 nao deve acontecer, mas protege a divisao usada no
    // aspect ratio da projecao (applyCameraAndLight / drawTextOnFloor).
    screenWidth_ = screenWidth;
    screenHeight_ = screenHeight > 0 ? screenHeight : 1;
}

void Renderer::computeCellSize(const Maze &maze) {
    cellSize_ = 2.0f; 
    mazeWidth3D_ = maze.width() * cellSize_;
    mazeDepth3D_ = maze.height() * cellSize_;
}

void Renderer::getCellCenter(int x, int y, float &outX, float &outZ) const {
    outX = (x * cellSize_) + (cellSize_ / 2.0f);
    outZ = (y * cellSize_) + (cellSize_ / 2.0f);
}

void Renderer::drawMaze(const Maze &maze) const {
    applyCameraAndLight();

    glm::vec4 wallColor(0.2f, 0.2f, 0.2f, 1.0f); 
    glm::vec4 floorColor(0.9f, 0.9f, 0.9f, 1.0f); 

    float wallThickness = cellSize_ * 0.15f;
    float wallHeight = cellSize_ * 0.8f;

    drawCube(glm::vec3(mazeWidth3D_ / 2.0f, -0.1f, mazeDepth3D_ / 2.0f), 
             glm::vec3(mazeWidth3D_, 0.2f, mazeDepth3D_), floorColor);

    for (int y = 0; y < maze.height(); ++y) {
        for (int x = 0; x < maze.width(); ++x) {
            float cx, cz;
            getCellCenter(x, y, cx, cz);
            const Cell &cell = maze.at(x, y);

            float halfCell = cellSize_ / 2.0f;
            float wh = wallHeight / 2.0f; 

            if (cell.walls[NORTH]) drawCube(glm::vec3(cx, wh, cz - halfCell), glm::vec3(cellSize_ + wallThickness, wallHeight, wallThickness), wallColor);
            if (cell.walls[SOUTH]) drawCube(glm::vec3(cx, wh, cz + halfCell), glm::vec3(cellSize_ + wallThickness, wallHeight, wallThickness), wallColor);
            if (cell.walls[WEST])  drawCube(glm::vec3(cx - halfCell, wh, cz), glm::vec3(wallThickness, wallHeight, cellSize_ + wallThickness), wallColor);
            if (cell.walls[EAST])  drawCube(glm::vec3(cx + halfCell, wh, cz), glm::vec3(wallThickness, wallHeight, cellSize_ + wallThickness), wallColor);
        }
    }
}

void Renderer::drawVisited(const Maze &maze, const SolveResult &result, size_t stepCount) const {
    glm::vec4 visitedFloorColor(0.0f, 0.5f, 1.0f, 0.4f); 
    glm::vec4 deadEndColor(0.9f, 0.1f, 0.1f, 0.7f);      

    size_t limit = std::min(stepCount, result.visitOrder.size());
    
    // FASE 1: Apenas colore o chão
    for (size_t i = 0; i < limit; ++i) {
        int idx = result.visitOrder[i];
        int x = idx % maze.width();
        int y = idx / maze.width();
        float cx, cz;
        getCellCenter(x, y, cx, cz);
        
        glm::vec4 currentColor = visitedFloorColor;
        const Cell &cell = maze.at(x, y);
        int wallCount = cell.walls[NORTH] + cell.walls[SOUTH] + cell.walls[EAST] + cell.walls[WEST];
        
        if (wallCount == 3) currentColor = deadEndColor;

        drawCube(glm::vec3(cx, 0.01f, cz), glm::vec3(cellSize_ * 0.98f, 0.02f, cellSize_ * 0.98f), currentColor);
        
        // Renderiza o texto no chão (Escala maior 0.035f e cor BRANCA)
        if (i < result.visitCosts.size()) {
            drawTextOnFloor(std::to_string(result.visitCosts[i]), cx, cz, 0.035f, glm::vec3(1.0f, 1.0f, 1.0f));
        }
    }
}

void Renderer::drawPath(const Maze &maze, const SolveResult &result, size_t stepCount) const {
    if (stepCount <= result.visitOrder.size()) return;
    
    size_t pathSteps = stepCount - result.visitOrder.size();
    size_t limit = std::min(pathSteps, result.path.size());

    glm::vec4 pathFloorColor(0.2f, 0.8f, 0.2f, 1.0f); 
    glm::vec4 ballColor(1.0f, 0.4f, 0.0f, 1.0f);         
    float radius = cellSize_ * 0.35f;

    // Pinta o chão de verde gradativamente e adiciona os números
    for (size_t i = 0; i < limit; ++i) {
        int idx = result.path[i];
        int x = idx % maze.width();
        int y = idx / maze.width();
        float cx, cz;
        getCellCenter(x, y, cx, cz);
        
        drawCube(glm::vec3(cx, 0.02f, cz), glm::vec3(cellSize_ * 0.98f, 0.02f, cellSize_ * 0.98f), pathFloorColor);
        
        // Renderiza o texto no chão do caminho final (Escala maior 0.035f e cor BRANCA)
        if (i < result.pathCosts.size()) {
            drawTextOnFloor(std::to_string(result.pathCosts[i]), cx, cz, 0.035f, glm::vec3(1.0f, 1.0f, 1.0f));
        }
    }

    // Desenha a bola exatamente na ponta do caminho verde
    if (limit > 0) {
        int currentIdx = result.path[limit - 1];
        int x = currentIdx % maze.width();
        int y = currentIdx / maze.width();
        float cx, cz;
        getCellCenter(x, y, cx, cz);
        drawSphere(glm::vec3(cx, radius, cz), radius, ballColor);
    }
}

void Renderer::drawStartAndGoal(const Maze &maze) const {
    auto [sx, sy] = maze.start();
    auto [gx, gy] = maze.goal();

    glm::vec4 startColor(1.0f, 0.8f, 0.0f, 1.0f); 
    glm::vec4 goalColor(0.8f, 0.1f, 0.2f, 1.0f);  

    float cx, cz;
    getCellCenter(sx, sy, cx, cz);
    drawCube(glm::vec3(cx, 0.1f, cz), glm::vec3(cellSize_ * 0.8f, 0.2f, cellSize_ * 0.8f), startColor);

    getCellCenter(gx, gy, cx, cz);
    drawCube(glm::vec3(cx, 0.1f, cz), glm::vec3(cellSize_ * 0.8f, 0.2f, cellSize_ * 0.8f), goalColor);
}

void Renderer::initText() {
    unsigned int vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &textVertexShader, NULL);
    glCompileShader(vShader);

    unsigned int fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &textFragmentShader, NULL);
    glCompileShader(fShader);

    textShaderProgram = glCreateProgram();
    glAttachShader(textShaderProgram, vShader);
    glAttachShader(textShaderProgram, fShader);
    glLinkProgram(textShaderProgram);

    glDeleteShader(vShader);
    glDeleteShader(fShader);

    // Usa alocação dinâmica (Heap) para não estourar a memória do Java (JNI)
    unsigned char* ttf_buffer = new unsigned char[1<<20];
    unsigned char* temp_bitmap = new unsigned char[512*512];

    FILE* f = fopen("arial.ttf", "rb"); 
    if (!f) {
        std::cerr << "ERRO: Fonte arial.ttf nao encontrada na pasta raiz!" << std::endl;
        delete[] ttf_buffer;
        delete[] temp_bitmap;
        return;
    }
    fread(ttf_buffer, 1, 1<<20, f);
    fclose(f);

    stbtt_BakeFontBitmap(ttf_buffer, 0, 32.0, temp_bitmap, 512, 512, 32, 96, cdata); 

    glGenTextures(1, &fontTexture);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 512, 512, 0, GL_RED, GL_UNSIGNED_BYTE, temp_bitmap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    delete[] ttf_buffer;
    delete[] temp_bitmap;

    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Renderer::drawTextOnFloor(std::string text, float cx, float cz, float scale, glm::vec3 color) const {
    glUseProgram(textShaderProgram);

    float maxDim = std::max(mazeWidth3D_, mazeDepth3D_);
    float radius = maxDim * 1.2f * cameraZoom;
    float camX = radius * cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
    float camY = radius * sin(glm::radians(cameraPitch));
    float camZ = radius * sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));

    glm::vec3 cameraPos = glm::vec3(camX, camY, camZ);
    glm::vec3 cameraTarget = glm::vec3(mazeWidth3D_ / 2.0f, 0.0f, mazeDepth3D_ / 2.0f);
    glm::mat4 view = glm::lookAt(cameraPos + cameraTarget, cameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));
    float farPlane = maxDim * 5.0f < 1000.0f ? 1000.0f : maxDim * 5.0f;
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)screenWidth_ / (float)screenHeight_, 0.1f, farPlane);

    glUniformMatrix4fv(glGetUniformLocation(textShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(textShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3f(glGetUniformLocation(textShaderProgram, "textColor"), color.x, color.y, color.z);

    float textWidth = 0.0f;
    for (char c : text) {
        if (c >= 32 && c < 128) textWidth += cdata[c - 32].xadvance * scale;
    }
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glBindVertexArray(textVAO);

    float currentX = 0.0f;
    float currentZ = 0.0f;

    for (char c : text) {
        if (c >= 32 && c < 128) {
            stbtt_aligned_quad q;
            stbtt_GetBakedQuad(cdata, 512, 512, c - 32, &currentX, &currentZ, &q, 1);

            float x0 = cx - (textWidth / 2.0f) + (q.x0 * scale);
            float x1 = cx - (textWidth / 2.0f) + (q.x1 * scale);
            float z0 = cz + (q.y0 * scale);
            float z1 = cz + (q.y1 * scale);

            float vertices[6][4] = {
                { x0, z0, q.s0, q.t0 }, { x0, z1, q.s0, q.t1 }, { x1, z1, q.s1, q.t1 },
                { x0, z0, q.s0, q.t0 }, { x1, z1, q.s1, q.t1 }, { x1, z0, q.s1, q.t0 }
            };

            glBindBuffer(GL_ARRAY_BUFFER, textVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }
    glBindVertexArray(0);
}