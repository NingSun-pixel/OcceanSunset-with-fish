#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/common.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <filesystem> 
#include <cmath>
#include "main.h"

FishSimulation* fishSimulation;

Camera camera(glm::vec3(67.4356f, 30.831f, 14.8224f), glm::vec3(0.0f, 1.0f, 0.0f), -167.5f, -17.5f, 45.0f);
float lastFrame = 0.0f;

// 定义骨骼结构
struct Joint {
    glm::vec3 position;      // 骨骼位置
    float rotationAngle;     // 当前旋转角度
    glm::mat4 transform;     // 最终变换矩阵
};

// 鱼尾的所有骨骼
std::vector<Joint> fishTailJoints;

void UpdateFishTail(float time) {
    float frequency = 2.0f;  // 摆动频率
    float maxAmplitude = 15.0f; // 最大摆动角度

    // 更新每个骨骼的旋转角度
    for (int i = 0; i < fishTailJoints.size(); i++) {
        // 计算每段骨骼的相位偏移
        float phaseOffset = static_cast<float>(i) * 0.5f;

        // 计算到原点的距离，并根据距离缩放幅度
        glm::vec3 position = fishTailJoints[i].position;
        float distance = glm::length(position);  // 计算距离
        float scaleFactor = glm::clamp(distance / 10.0f, 1.0f, 1.0f); // 缩放因子，0.1 到 1 之间

        // 使用正弦波计算旋转角度，并应用缩放因子
        fishTailJoints[i].rotationAngle = maxAmplitude * scaleFactor * std::sin(frequency * time + phaseOffset);

        // 生成旋转变换矩阵（绕 y 轴旋转）
        fishTailJoints[i].transform = glm::rotate(glm::mat4(1.0f), glm::radians(fishTailJoints[i].rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    }
}


void ApplyFishTailTransform() {
    glm::mat4 parentTransform = glm::mat4(1.0f);

    for (int i = 0; i < fishTailJoints.size(); i++) {
        // 累积父变换
        fishTailJoints[i].transform = parentTransform * fishTailJoints[i].transform;

        // 更新父变换，用于下一个骨骼
        parentTransform = fishTailJoints[i].transform;
    }
}


void InitializeFishTail() {
    int numJoints = 5;
    fishTailJoints.resize(numJoints);

    // 初始化骨骼位置（假设鱼尾沿 z 轴延伸）
    for (int i = 0; i < numJoints; i++) {
        fishTailJoints[i].position = glm::vec3(0.0f, 0.0f, -i * 0.5f); // 每段骨骼相隔一定距离
        fishTailJoints[i].rotationAngle = 0.0f;
        fishTailJoints[i].transform = glm::mat4(1.0f);
    }
}


//used to be not now:ALL TEX ARE SAVE IN THE SAME MODEL NAME FOLDER,go there and find it
unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

// Macro for indexing vertex buffer
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

using namespace std;

// Vertex Shader (for convenience, it is defined in the main here, but we will be using text files for shaders in future)
// Note: Input to this shader is the vertex positions that we specified for the triangle. 
// Note: gl_Position is a special built-in variable that is supposed to contain the vertex position (in X, Y, Z, W)
// Since our triangle vertices were specified as vec3, we just set W to 1.0.

// 读取文件内容为字符串
std::string readShaderFile(const char* filePath) {
    std::ifstream shaderFile;
    std::stringstream shaderStream;

    // 打开文件
    shaderFile.open(filePath);
    if (!shaderFile.is_open()) {
        std::cerr << "Failed to open shader file: " << filePath << std::endl;
        return "";
    }

    // 读取文件缓冲区内容到流中
    shaderStream << shaderFile.rdbuf();
    shaderFile.close();

    // 返回文件内容作为字符串
    return shaderStream.str();
}

unsigned int createShader(const char* vertexPath, const char* fragmentPath) {
    // 读取顶点和片段着色器文件
    std::string vertexCode = readShaderFile(vertexPath);
    std::string fragmentCode = readShaderFile(fragmentPath);

    const char* vertexShaderSource = vertexCode.c_str();
    const char* fragmentShaderSource = fragmentCode.c_str();

    // 创建和编译顶点着色器
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);     
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    //cout << vertexShaderSource << endl;
    //cout << fragmentShaderSource<< endl;

    // 检查编译错误
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // 创建和编译片段着色器
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // 检查编译错误
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // 链接着色器程序
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // 检查链接错误
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    // 删除着色器，它们已经被链接到程序中，不再需要
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);


    return shaderProgram;
}


// 全局变量：平移矩阵
glm::vec3 translation(0.0f, 0.0f, 0.0f);

unsigned int textureID;

unsigned int shaderProgram_use;
unsigned int skyboxShaderProgram_use;
unsigned int GPUInstancingShaderProgram_use;

GLuint skyboxVAO;



struct Bone {
    std::string name;
    glm::mat4 offsetMatrix; // 从模型空间到骨骼空间的偏移矩阵
    glm::mat4 finalTransformation; // 最终变换矩阵
    int parentIndex; // 父骨骼的索引
};

struct VertexWeight {
    int boneID;
    float weight;
};


// 全局变量，用于存储多个模型
std::vector<Model> models;
std::vector<Model> fishmodels;
vector<std::string> faces;
GLuint cubemapTexture;


// 加载所有模型
void loadModels(const std::vector<std::string>& fbxFiles) {
    //先创建共享TexID
    for (const auto& file : fbxFiles) {
        Model model;
        loadSingleModel(file, model);
        models.push_back(model);
    }

    //load Skybox
    float skyboxVertices[] = {
        // back
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        // front
        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        // left
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,

        // right
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,

         // up
         -1.0f,  1.0f,  1.0f,
         -1.0f,  1.0f, -1.0f,
          1.0f,  1.0f, -1.0f,
          1.0f,  1.0f, -1.0f,
          1.0f,  1.0f,  1.0f,
         -1.0f,  1.0f,  1.0f,

         // down
         -1.0f, -1.0f, -1.0f,
         -1.0f, -1.0f,  1.0f,
          1.0f, -1.0f,  1.0f,
          1.0f, -1.0f,  1.0f,
          1.0f, -1.0f, -1.0f,
         -1.0f, -1.0f, -1.0f
    };

    // 生成天空盒的VAO和VBO
    GLuint skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
}

// Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS
static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
    // create a shader object
    GLuint ShaderObj = glCreateShader(ShaderType);

    if (ShaderObj == 0) {
        fprintf(stderr, "Error creating shader type %d\n", ShaderType);
        exit(0);
    }
    // Bind the source code to the shader, this happens before compilation
    glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderText, NULL);
    // compile the shader and check for errors
    glCompileShader(ShaderObj);
    GLint success;
    // check for shader related errors using glGetShaderiv
    glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar InfoLog[1024];
        glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
        fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
        exit(1);
    }
    // Attach the compiled shader object to the program object
    glAttachShader(ShaderProgram, ShaderObj);
}

#pragma endregion SHADER_FUNCTIONS



unsigned int VAO, VBO, EBO;


LightingManager& lighting = LightingManager::getInstance();
//get all Tex
std::vector<std::string> getAllTexFiles(const std::string& folderPath) {
    std::vector<std::string> fbxFiles;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(folderPath)) {
        if (entry.path().extension() == ".png") {
            fbxFiles.push_back(entry.path().string());
        }
    }

    return fbxFiles;
}

void renderScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //std::cout <<"camera.position:" << camera.position.x << " " << camera.position.y << " " << camera.position.z << endl;
    //std::cout << "camera.front:"<< camera.front.x << " " << camera.front.y << " " << camera.front.z << endl;
    //std::cout << " camera.up:" << camera.up.x << " " << camera.up.y << " " << camera.up.z << endl;

    // 更新光照参数
    // 获取相机的视图和投影矩阵
    // 获取当前时间（秒）
    float currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float deltaTime = currentTime - lastFrame;
    lastFrame = currentTime;
    LightingManager::getInstance().updateLighting(deltaTime);

    // 传递时间变量给顶点着色器
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 projection = camera.getProjectionMatrix(1920.0f / 1080.0f);  // 假设窗口是800x800
    renderSkybox(skyboxShaderProgram_use, skyboxVAO, cubemapTexture, view, projection);
    glDepthFunc(GL_LESS);      // 恢复默认深度测试模式
    glDepthMask(GL_TRUE);       // 允许写入深度缓冲区
    //drawing
    // 使用着色器程序
    glUseProgram(shaderProgram_use);
    glUniform1f(glGetUniformLocation(shaderProgram_use, "uTime"), currentTime);


    // 光照参数切换逻辑
    static bool lastToggleState = false; // 记录上一次的布尔值状态
    bool currentToggleState = LightingManager::getInstance().gettoggleLightingPreset();

    GLint lightDirLoc = glGetUniformLocation(shaderProgram_use, "lightDirection");
    GLint lightColorLoc = glGetUniformLocation(shaderProgram_use, "lightColor");
    GLint smoothnessLoc = glGetUniformLocation(shaderProgram_use, "smoothness");

    // 获取点光源参数的 Uniform 位置
    GLint numPointLightsLoc = glGetUniformLocation(shaderProgram_use, "numPointLights");
    GLint pointLightPositionsLoc = glGetUniformLocation(shaderProgram_use, "pointLightPositions");
    GLint pointLightColorsLoc = glGetUniformLocation(shaderProgram_use, "pointLightColors");
    GLint pointLightIntensitiesLoc = glGetUniformLocation(shaderProgram_use, "pointLightIntensities");
    GLint pointLightRadiiLoc = glGetUniformLocation(shaderProgram_use, "pointLightRadii");

    // 从 LightingManager 获取点光源数据
    const std::vector<PointLight>& pointLights = LightingManager::getInstance().getPointLights();
    int numPointLights = pointLights.size();

    // 设置点光源数量
    glUniform1i(numPointLightsLoc, numPointLights);

    if (numPointLights > 0) {
        // 准备点光源数据
        std::vector<glm::vec3> pointLightPositions(numPointLights);
        std::vector<glm::vec3> pointLightColors(numPointLights);
        std::vector<float> pointLightIntensities(numPointLights);
        std::vector<float> pointLightRadii(numPointLights);

        for (int i = 0; i < numPointLights; ++i) {
            pointLightPositions[i] = pointLights[i].position;
            pointLightColors[i] = pointLights[i].color;
            pointLightIntensities[i] = pointLights[i].intensity;
            pointLightRadii[i] = pointLights[i].radius;
        } 

        // 传递点光源参数给着色器
        glUniform3fv(pointLightPositionsLoc, numPointLights, glm::value_ptr(pointLightPositions[0]));
        glUniform3fv(pointLightColorsLoc, numPointLights, glm::value_ptr(pointLightColors[0]));
        glUniform1fv(pointLightIntensitiesLoc, numPointLights, pointLightIntensities.data());
        glUniform1fv(pointLightRadiiLoc, numPointLights, pointLightRadii.data());
    }


    //fog
    // 获取雾相关参数的 Uniform 位置
    GLint fogColorLoc = glGetUniformLocation(shaderProgram_use, "fogColor");
    GLint fogDensityLoc = glGetUniformLocation(shaderProgram_use, "fogDensity");
    GLint fogHeightStartLoc = glGetUniformLocation(shaderProgram_use, "fogHeightStart");
    GLint fogHeightEndLoc = glGetUniformLocation(shaderProgram_use, "fogHeightEnd");
    GLint fogDistanceStartLoc = glGetUniformLocation(shaderProgram_use, "fogDistanceStart");
    GLint fogDistanceEndLoc = glGetUniformLocation(shaderProgram_use, "fogDistanceEnd");

    // 获取 LightingManager 的实例
    const LightingManager& lightingManager = LightingManager::getInstance();
    const FogSettings& fogSettings = lightingManager.getFogSettings();

    // 设置雾效参数
    glUniform3fv(fogColorLoc, 1, &fogSettings.fogColor[0]);
    glUniform1f(fogDensityLoc, fogSettings.fogDensity);
    glUniform1f(fogHeightStartLoc, fogSettings.fogHeightStart);
    glUniform1f(fogHeightEndLoc, fogSettings.fogHeightEnd);
    glUniform1f(fogDistanceStartLoc, fogSettings.fogDistanceStart);
    glUniform1f(fogDistanceEndLoc, fogSettings.fogDistanceEnd);

    // 设置光照方向、光照颜色和 smooth 值
    glUniform3fv(lightDirLoc, 1, &LightingManager::getInstance().getLightDirection()[0]);
    glUniform3fv(lightColorLoc, 1, &LightingManager::getInstance().getLightColor()[0]);
    glUniform1f(smoothnessLoc, static_cast<float>(LightingManager::getInstance().getSmoothness()));

    // control camera speed
    float currentFrame = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    //float deltaTime = currentFrame - lastFrame;
    //lastFrame = currentFrame;

    glm::vec3 viewPos = camera.position;


    // 传递位置
    GLuint viewPosLocation = glGetUniformLocation(shaderProgram_use, "viewPos");
    glUniform3fv(viewPosLocation, 1, glm::value_ptr(viewPos));

    // transfer view and projection Matrix to shader
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram_use, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram_use, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    //sub animation
    //UpdateFishTail(currentTime); // update the tail
    //ApplyFishTailTransform();    // apply the tail animation

    float speed = 0.5f;  // speed
    // hierarchy (big animation)
    float translationDistance = speed * currentTime * 0.1f;  // 计算平移距离
    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(translationDistance, 0.0f, 0.0f));

    for (const auto& model : models) {
        glBindVertexArray(model.VAO);

        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, translation);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram_use, "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));

        // 绑定 Diffuse 贴图到纹理单元 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, model.textureID_D);
        glUniform1i(glGetUniformLocation(shaderProgram_use, "albedoMap"), 0); // 将纹理单元 0 绑定到 albedoMap

        // 绑定 Normal 贴图到纹理单元 1
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, model.textureID_N);
        glUniform1i(glGetUniformLocation(shaderProgram_use, "normalMap"), 1); // 将纹理单元 1 绑定到 normalMap

        // 绑定 Roughness 贴图到纹理单元 2
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, model.textureID_R);
        glUniform1i(glGetUniformLocation(shaderProgram_use, "RoughnessMap"), 2); // 将纹理单元 2 绑定到 RoughnessMap


        glDrawElements(GL_TRIANGLES, model.indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);  // 解除绑定
    }


    TwDraw();
    fishSimulation->updateFish(deltaTime);
    fishSimulation->renderFish(GPUInstancingShaderProgram_use);

    glutSwapBuffers();
    glutPostRedisplay();
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL error: " << err << std::endl;
    }
}

// 初始化窗口和 OpenGL 环境
void initGL() {
    // 设置清屏颜色为黑色
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    shaderProgram_use = createShader("../Project1_64/shader/vertex_shader.glsl", "../Project1_64/shader/fragment_shader.glsl");
    skyboxShaderProgram_use = createShader("../Project1_64/shader/vertex_skybox.glsl", "../Project1_64/shader/fragment_skybox.glsl");
    GPUInstancingShaderProgram_use = createShader("../Project1_64/shader/vertex_GPUInstancing.glsl", "../Project1_64/shader/fragment_GPUInstancing.glsl");

    faces = getAllTexFiles("C:/Users/555/Desktop/assignment/CG_Project_1/SkyBoxTexture");
    cubemapTexture = loadCubemap(faces);
    // 启用深度测试
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

}

int windowWidth = 1920;
int windowHeight = 1080;
bool mousePressed = false;     // if press or not
float lastX, lastY;            // last month position

MouseHandler* mouseHandler = nullptr;

//相机点击逻辑
void processNormalKeys(unsigned char key, int x, int y) {
    if (mouseHandler) {
        mouseHandler->processKeys(key, x, y);
    }
}



void mouseButtonCallback(int button, int state, int x, int y) {
    if (mouseHandler) {
        mouseHandler->handleMouseButton(button, state, x, y);
    }
}

void mouseMotionCallback(int x, int y) {
    if (mouseHandler) {
        mouseHandler->handleMouseMotion(x, y);
    }
}

void initOpenGLAndAntTweakBar() {
    // 初始化 AntTweakBar
    TwInit(TW_OPENGL, NULL);

    // 创建一个新的 Tweak Bar
    TwBar* bar = TwNewBar("Settings");

    // 设置 Tweak Bar 的位置和大小
    int barWidth = 400;  // Tweak Bar 的宽度
    int barHeight = 200; // Tweak Bar 的高度
    TwWindowSize(windowWidth, windowHeight);

    // 添加光照方向变量
    TwAddVarCB(bar, "Light Direction", TW_TYPE_DIR3F,
        [](const void* value, void* clientData) {
            LightingManager::getInstance().setLightDirection(*static_cast<const glm::vec3*>(value));
        },
        [](void* value, void* clientData) {
            *static_cast<glm::vec3*>(value) = LightingManager::getInstance().getLightDirection();
        },
        nullptr, "label='Light Direction' help='Adjust the light direction'");

    // 添加光照颜色变量
    TwAddVarCB(bar, "Light Color", TW_TYPE_COLOR3F,
        [](const void* value, void* clientData) {
            LightingManager::getInstance().setLightColor(*static_cast<const glm::vec3*>(value));
        },
        [](void* value, void* clientData) {
            *static_cast<glm::vec3*>(value) = LightingManager::getInstance().getLightColor();
        },
        nullptr, "label='Light Color' help='Adjust the light color'");

    // 添加 smoothness 变量
    TwAddVarCB(bar, "LightStrength", TW_TYPE_DOUBLE,
        [](const void* value, void* clientData) {
            LightingManager::getInstance().setSmoothness(*static_cast<const double*>(value));
        },
        [](void* value, void* clientData) {
            *static_cast<double*>(value) = LightingManager::getInstance().getSmoothness();
        },
        nullptr, "label='Value' min=0 max=100 step=0.1");

    // 添加布尔变量用于切换光照参数
    TwAddVarCB(bar, "Toggle Lighting Preset", TW_TYPE_BOOLCPP,
        [](const void* value, void* clientData) {
            LightingManager::getInstance().settoggleLightingPreset(*static_cast<const bool*>(value));
        },
        [](void* value, void* clientData) {
            *static_cast<bool*>(value) = LightingManager::getInstance().gettoggleLightingPreset();
        },
        nullptr, "label='Toggle Lighting Preset' help='Switch between two lighting presets'");
    glm::vec3 pointLightColor = glm::vec3(0.6f, 1.0f, 1.0f);
    LightingManager::getInstance().addPointLight(glm::vec3(-15.0f, 10.0f, -15.0f), pointLightColor, 30, 15);
    LightingManager::getInstance().addPointLight(glm::vec3(15.0f, 15.0f, 15.0f), pointLightColor, 30, 15);
    LightingManager::getInstance().addPointLight(glm::vec3(-20.0f, 20.0f, 20.0f), pointLightColor, 30, 15);;


    // 动态控制点光源的第一个光源 (示例，扩展可遍历所有点光源)
    if (!LightingManager::getInstance().getPointLights().empty()) {
        TwAddVarCB(bar, "Point Light Position", TW_TYPE_DIR3F,
            [](const void* value, void* clientData) {
                auto& lights = LightingManager::getInstance().getPointLights();
                if (!lights.empty()) lights[0].position = *static_cast<const glm::vec3*>(value);
            },
            [](void* value, void* clientData) {
                auto& lights = LightingManager::getInstance().getPointLights();
                if (!lights.empty()) *static_cast<glm::vec3*>(value) = lights[0].position;
            },
            nullptr, "label='Position' help='Modify position of the first point light'");

        TwAddVarCB(bar, "Point Light Color", TW_TYPE_COLOR3F,
            [](const void* value, void* clientData) {
                auto& lights = LightingManager::getInstance().getPointLights();
                if (!lights.empty()) lights[0].color = *static_cast<const glm::vec3*>(value);
            },
            [](void* value, void* clientData) {
                auto& lights = LightingManager::getInstance().getPointLights();
                if (!lights.empty()) *static_cast<glm::vec3*>(value) = lights[0].color;
            },
            nullptr, "label='Color' help='Modify color of the first point light'");

        TwAddVarCB(bar, "Point Light Intensity", TW_TYPE_FLOAT,
            [](const void* value, void* clientData) {
                auto& lights = LightingManager::getInstance().getPointLights();
                if (!lights.empty()) 
                    lights[0].intensity = *static_cast<const float*>(value);
            },
            [](void* value, void* clientData) {
                auto& lights = LightingManager::getInstance().getPointLights();
                if (!lights.empty()) *static_cast<float*>(value) = lights[0].intensity;
            },
            nullptr, "label='Intensity' min=0 max=100 step=0.1 help='Adjust intensity of the first point light'");

        TwAddVarCB(bar, "Point Light Radius", TW_TYPE_FLOAT,
            [](const void* value, void* clientData) {
                auto& lights = LightingManager::getInstance().getPointLights();
                if (!lights.empty()) 
                    lights[0].radius = *static_cast<const float*>(value);
            },
            [](void* value, void* clientData) {
                auto& lights = LightingManager::getInstance().getPointLights();
                if (!lights.empty()) 
                    *static_cast<float*>(value) = lights[0].radius;
            },
            nullptr, "label='Radius' min=0 max=100 step=1 help='Adjust radius of the first point light'");
    }

    // 动态控制点光源的第2个光源 
    if (LightingManager::getInstance().getPointLights().size() > 1) {
        TwAddVarCB(bar, "Point Light Position 1", TW_TYPE_DIR3F,
            [](const void* value, void* clientData) {
                auto& lights = LightingManager::getInstance().getPointLights();
                if (lights.size() > 1) lights[1].position = *static_cast<const glm::vec3*>(value);
            },
            [](void* value, void* clientData) {
                auto& lights = LightingManager::getInstance().getPointLights();
                if (lights.size() > 1) *static_cast<glm::vec3*>(value) = lights[1].position;
            },
            nullptr, "label='Position' help='Modify position of the first point light'");

        TwAddVarCB(bar, "Point Light Color 1", TW_TYPE_COLOR3F,
            [](const void* value, void* clientData) {
                auto& lights = LightingManager::getInstance().getPointLights();
                if (lights.size() > 1) lights[1].color = *static_cast<const glm::vec3*>(value);
            },
            [](void* value, void* clientData) {
                auto& lights = LightingManager::getInstance().getPointLights();
                if (lights.size() > 1) *static_cast<glm::vec3*>(value) = lights[1].color;
            },
            nullptr, "label='Color' help='Modify color of the first point light'");

        TwAddVarCB(bar, "Point Light Intensity 1", TW_TYPE_FLOAT,
            [](const void* value, void* clientData) {
                auto& lights = LightingManager::getInstance().getPointLights();
                if (lights.size() > 1)
                    lights[1].intensity = *static_cast<const float*>(value);
            },
            [](void* value, void* clientData) {
                auto& lights = LightingManager::getInstance().getPointLights();
                if (lights.size() > 1) *static_cast<float*>(value) = lights[1].intensity;
            },
            nullptr, "label='Intensity' min=0 max=100 step=0.1 help='Adjust intensity of the first point light'");

        TwAddVarCB(bar, "Point Light Radius 1", TW_TYPE_FLOAT,
            [](const void* value, void* clientData) {
                auto& lights = LightingManager::getInstance().getPointLights();
                if (lights.size() > 1)
                    lights[1].radius = *static_cast<const float*>(value);
            },
            [](void* value, void* clientData) {
                auto& lights = LightingManager::getInstance().getPointLights();
                if (lights.size() > 1)
                    *static_cast<float*>(value) = lights[1].radius;
            },
            nullptr, "label='Radius' min=0 max=100 step=1 help='Adjust radius of the first point light'");
    }

    // 动态控制点光源的第3个光源 
    if (LightingManager::getInstance().getPointLights().size() > 2) {
        TwAddVarCB(bar, "Point Light Position 2", TW_TYPE_DIR3F,
            [](const void* value, void* clientData) {
                auto& lights = LightingManager::getInstance().getPointLights();
                if (lights.size() > 2) lights[2].position = *static_cast<const glm::vec3*>(value);
            },
            [](void* value, void* clientData) {
                auto& lights = LightingManager::getInstance().getPointLights();
                if (lights.size() > 2) *static_cast<glm::vec3*>(value) = lights[2].position;
            },
            nullptr, "label='Position' help='Modify position of the first point light'");

        TwAddVarCB(bar, "Point Light Color 2", TW_TYPE_COLOR3F,
            [](const void* value, void* clientData) {
                auto& lights = LightingManager::getInstance().getPointLights();
                if (lights.size() > 2) lights[2].color = *static_cast<const glm::vec3*>(value);
            },
            [](void* value, void* clientData) {
                auto& lights = LightingManager::getInstance().getPointLights();
                if (lights.size() > 2) *static_cast<glm::vec3*>(value) = lights[2].color;
            },
            nullptr, "label='Color' help='Modify color of the first point light'");

        TwAddVarCB(bar, "Point Light Intensity 2", TW_TYPE_FLOAT,
            [](const void* value, void* clientData) {
                auto& lights = LightingManager::getInstance().getPointLights();
                if (lights.size() > 2)
                    lights[2].intensity = *static_cast<const float*>(value);
            },
            [](void* value, void* clientData) {
                auto& lights = LightingManager::getInstance().getPointLights();
                if (lights.size() > 2) *static_cast<float*>(value) = lights[2].intensity;
            },
            nullptr, "label='Intensity' min=0 max=100 step=0.1 help='Adjust intensity of the first point light'");

        TwAddVarCB(bar, "Point Light Radius 2", TW_TYPE_FLOAT,
            [](const void* value, void* clientData) {
                auto& lights = LightingManager::getInstance().getPointLights();
                if (lights.size() > 2)
                    lights[2].radius = *static_cast<const float*>(value);
            },
            [](void* value, void* clientData) {
                auto& lights = LightingManager::getInstance().getPointLights();
                if (lights.size() > 2)
                    *static_cast<float*>(value) = lights[2].radius;
            },
            nullptr, "label='Radius' min=0 max=100 step=1 help='Adjust radius of the first point light'");
    }

    // 设置 Tweak Bar 的位置和大小
    int position[] = { windowWidth - barWidth, windowHeight - barHeight };
    TwSetParam(bar, NULL, "position", TW_PARAM_INT32, 2, position);

    int size[] = { barWidth, barHeight };
    TwSetParam(bar, NULL, "size", TW_PARAM_INT32, 2, size);


}


int main(int argc, char** argv) {
    std::cout << "Current working directory: " << std::filesystem::current_path() << std::endl;
    // Set up the window


    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("Scene Rendering");
    // Tell glut where the display function is

    // 初始化 GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }
    initGL();
    initOpenGLAndAntTweakBar();
    std::vector<std::string> fbxfishFiles = getAllFBXFiles("C:/Users/555/Desktop/assignment/CG_Project_1/Anim/FBX_4");
    FishSimulation fishSim(1000, camera);
    fishSimulation = &fishSim;

    fishSimulation->initFishInstances();
    cout << "loadFishModels"<< endl;
    fishSimulation->loadFishModel(fbxfishFiles[0]);

    std::vector<std::string> fbxFiles = getAllFBXFiles("C:/Users/555/Desktop/assignment/CG_Project_1/FBX_4");
    loadModels(fbxFiles);

    glutDisplayFunc(renderScene);
    // 创建 MouseHandler 对象
    MouseHandler mouseHandlerInstance(camera);
    mouseHandler = &mouseHandlerInstance;
    //相机点击逻辑
    glutKeyboardFunc(processNormalKeys);
    glutMouseFunc(mouseButtonCallback);
    glutMotionFunc(mouseMotionCallback);
    InitializeFishTail();

    glutMainLoop(); 
    return 0;
}











  