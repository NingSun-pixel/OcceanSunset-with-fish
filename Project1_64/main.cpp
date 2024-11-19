#include<assimp/Importer.hpp>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <glm/common.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <fstream>
#include <sstream>
#include <filesystem> 
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <cmath>
#include <AntTweakBar.h>

class TextureManager {
public:
    // ��ȡ����ID����������Ѽ��أ��򷵻����е�ID���������������
    static GLuint getTexture(const std::string& texturePath) {
        auto it = textureCache.find(texturePath);
        if (it != textureCache.end()) {
            // �����Ѵ��ڣ��������е�ID
            return it->second;
        }
        else {
            unsigned int textureID;
            glGenTextures(1, &textureID);

            int width, height, nrComponents;
            unsigned char* data = stbi_load(texturePath.c_str(), &width, &height, &nrComponents, 0);
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
                std::cout << "Texture failed to load at path: " << texturePath << std::endl;
                stbi_image_free(data);
            }

            textureCache[texturePath] = textureID;
            return textureID;

        }
    }

    // �������м��ص�����
    static void cleanup() {
        for (auto& texture : textureCache) {
            glDeleteTextures(1, &texture.second);
        }
        textureCache.clear();
    }

private:
    static std::unordered_map<std::string, GLuint> textureCache;
};

// ���徲̬��Ա
std::unordered_map<std::string, GLuint> TextureManager::textureCache;
// Ĭ�Ϲ��շ������ɫ
glm::vec3 lightDirection = glm::normalize(glm::vec3(-1.0f, 1.0f, 1.0f)); // б���µĹ��շ���
glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);  // ��ɫ��
float smoothness = 0.0f;  // Ĭ�ϵ� smooth ֵ


class Camera {
public:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    float yaw;
    float pitch;
    float fov;

    Camera(glm::vec3 startPosition, glm::vec3 startUp, float startYaw, float startPitch, float startFov)
        : position(startPosition), up(startUp), yaw(startYaw), pitch(startPitch), fov(startFov) {
        updateCameraVectors();
    }

    glm::mat4 getViewMatrix() const {
        return glm::lookAt(position, position + front, up);
    }

    glm::mat4 getProjectionMatrix(float aspectRatio) const {
        return glm::perspective(glm::radians(fov), aspectRatio, 0.1f, 100.0f);
    }

    void processKeyboard(char key, float deltaTime) {
        float velocity = 2.5f * deltaTime;
        if (key == 'w')
            position += front * velocity;
        if (key == 's')
            position -= front * velocity;
        if (key == 'a')
            position -= glm::normalize(glm::cross(front, up)) * velocity;
        if (key == 'd')
            position += glm::normalize(glm::cross(front, up)) * velocity;
    }

    void processMouseMovement(float xOffset, float yOffset) {
        float sensitivity = 0.1f;
        xOffset *= sensitivity;
        yOffset *= sensitivity;

        yaw += xOffset;
        pitch += yOffset;

        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;

        updateCameraVectors();
    }

private:
    void updateCameraVectors() {
        glm::vec3 newFront;
        newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        newFront.y = sin(glm::radians(pitch));
        newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(newFront);
    }
};


Camera camera(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f, 45.0f);
float lastFrame = 0.0f;

// ��������ṹ
struct Joint {
    glm::vec3 position;      // ����λ��
    float rotationAngle;     // ��ǰ��ת�Ƕ�
    glm::mat4 transform;     // ���ձ任����
};

// ��β�����й���
std::vector<Joint> fishTailJoints;

void UpdateFishTail(float time) {
    float frequency = 2.0f;  // �ڶ�Ƶ��
    float maxAmplitude = 15.0f; // ���ڶ��Ƕ�

    // ����ÿ����������ת�Ƕ�
    for (int i = 0; i < fishTailJoints.size(); i++) {
        // ����ÿ�ι�������λƫ��
        float phaseOffset = static_cast<float>(i) * 0.5f;

        // ���㵽ԭ��ľ��룬�����ݾ������ŷ���
        glm::vec3 position = fishTailJoints[i].position;
        float distance = glm::length(position);  // �������
        float scaleFactor = glm::clamp(distance / 10.0f, 1.0f, 1.0f); // �������ӣ�0.1 �� 1 ֮��

        // ʹ�����Ҳ�������ת�Ƕȣ���Ӧ����������
        fishTailJoints[i].rotationAngle = maxAmplitude * scaleFactor * std::sin(frequency * time + phaseOffset);

        // ������ת�任������ y ����ת��
        fishTailJoints[i].transform = glm::rotate(glm::mat4(1.0f), glm::radians(fishTailJoints[i].rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    }
}


void ApplyFishTailTransform() {
    glm::mat4 parentTransform = glm::mat4(1.0f);

    for (int i = 0; i < fishTailJoints.size(); i++) {
        // �ۻ����任
        fishTailJoints[i].transform = parentTransform * fishTailJoints[i].transform;

        // ���¸��任��������һ������
        parentTransform = fishTailJoints[i].transform;
    }
}


void InitializeFishTail() {
    int numJoints = 5;
    fishTailJoints.resize(numJoints);

    // ��ʼ������λ�ã�������β�� z �����죩
    for (int i = 0; i < numJoints; i++) {
        fishTailJoints[i].position = glm::vec3(0.0f, 0.0f, -i * 0.5f); // ÿ�ι������һ������
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

GLuint loadCubemap(std::vector<std::string> faces) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (GLuint i = 0; i < faces.size(); i++) {
        //get 6 Texture
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}



// Macro for indexing vertex buffer
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

using namespace std;

// Vertex Shader (for convenience, it is defined in the main here, but we will be using text files for shaders in future)
// Note: Input to this shader is the vertex positions that we specified for the triangle. 
// Note: gl_Position is a special built-in variable that is supposed to contain the vertex position (in X, Y, Z, W)
// Since our triangle vertices were specified as vec3, we just set W to 1.0.

// ��ȡ�ļ�����Ϊ�ַ���
std::string readShaderFile(const char* filePath) {
    std::ifstream shaderFile;
    std::stringstream shaderStream;

    // ���ļ�
    shaderFile.open(filePath);
    if (!shaderFile.is_open()) {
        std::cerr << "Failed to open shader file: " << filePath << std::endl;
        return "";
    }

    // ��ȡ�ļ����������ݵ�����
    shaderStream << shaderFile.rdbuf();
    shaderFile.close();

    // �����ļ�������Ϊ�ַ���
    return shaderStream.str();
}

unsigned int createShader(const char* vertexPath, const char* fragmentPath) {
    // ��ȡ�����Ƭ����ɫ���ļ�
    std::string vertexCode = readShaderFile(vertexPath);
    std::string fragmentCode = readShaderFile(fragmentPath);

    const char* vertexShaderSource = vertexCode.c_str();
    const char* fragmentShaderSource = fragmentCode.c_str();

    // �����ͱ��붥����ɫ��
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);     
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    //cout << vertexShaderSource << endl;
    //cout << fragmentShaderSource<< endl;

    // ���������
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // �����ͱ���Ƭ����ɫ��
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // ���������
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // ������ɫ������
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // ������Ӵ���
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    // ɾ����ɫ���������Ѿ������ӵ������У�������Ҫ
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);


    return shaderProgram;
}


// ȫ�ֱ�����ƽ�ƾ���
glm::vec3 translation(0.0f, 0.0f, 0.0f);

// һ���򵥵Ľṹ�����洢������Ϣ
struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 normal;
    glm::vec2 texCoords;
};

//std::vector<Vertex> vertices;
//std::vector<unsigned int> indices;
unsigned int shaderProgram_use;
unsigned int textureID;
unsigned int skyboxShaderProgram_use;
GLuint skyboxVAO;


struct Model {
    unsigned int VAO, VBO, EBO;
    unsigned int textureID;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

struct Bone {
    std::string name;
    glm::mat4 offsetMatrix; // ��ģ�Ϳռ䵽�����ռ��ƫ�ƾ���
    glm::mat4 finalTransformation; // ���ձ任����
    int parentIndex; // ������������
};

struct VertexWeight {
    int boneID;
    float weight;
};


// ȫ�ֱ��������ڴ洢���ģ��
std::vector<Model> models;
std::vector<Model> fishmodels;
vector<std::string> faces;
GLuint cubemapTexture;
//std::vector<string> ConstNameTexOfFBX = { "CoralRock","Coral_1","Coral5", "Coral9", "Coral1"};

//fbx->Tex
//void loadTextureForModel(const std::string& modelPath, Model& model) {
//    // ��ȡ�ļ���
//    std::filesystem::path modelFilePath(modelPath);
//    std::string modelName = modelFilePath.stem().string(); // ��ȡ�ļ�����������չ����
//    cout << modelName;
//    if (modelName.find("CoralRock"))
//    {
//        cout << modelName;
//        modelName = "CoralRock";
//    }
//    else {
//        // ���ҵ㲢��ȡ������
//        size_t dotPos = modelName.find('.');
//        if (dotPos != std::string::npos) {
//            modelName = modelName.substr(0, dotPos); // ��ȡ��ǰ��������
//        }
//    }
//    // ��������·������������λ��ģ���ļ��ϼ�Ŀ¼�е� Texture �ļ���
//    std::filesystem::path modelDirectory = modelFilePath.parent_path().parent_path();
//    std::filesystem::path texturePath = modelDirectory / "Texture_2" / (modelName + ".png");
//
//    // ��������
//    if (std::filesystem::exists(texturePath)) {
//        model.textureID = loadTexture(texturePath.string().c_str());
//    }
//    else {
//        std::cerr << "Texture file not found at path: " << texturePath << std::endl;
//    }
//}

// load single model and create for their VAO��VBO��EBO
void loadSingleModel(const std::string& path, Model& model) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "Error: " << importer.GetErrorString() << std::endl;
        return;
    }

    aiMesh* mesh = scene->mMeshes[0];

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        vertex.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        vertex.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        vertex.texCoords = mesh->mTextureCoords[0] ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y) : glm::vec2(0.0f, 0.0f);
        // vertex color
        if (mesh->mColors[0]) { // check if have vertex color
            vertex.color = glm::vec3(mesh->mColors[0][i].r, mesh->mColors[0][i].g, mesh->mColors[0][i].b);
        }
        else {
            vertex.color = glm::vec3(1.0f, 1.0f, 1.0f); // no color just white
        }

        model.vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            model.indices.push_back(face.mIndices[j]);
    }

    std::filesystem::path modelFilePath(path);
    std::string modelDirectory = modelFilePath.parent_path().parent_path().string();
    std::string modelName = modelFilePath.stem().string();

    cout << modelName << endl;
    bool isConstTex = false;

    size_t pos = modelName.find('_');
    if (pos != std::string::npos) {
        string result = modelName.substr(0, pos);
        modelName = result;
    }

    std::filesystem::path texturePath = modelDirectory + "/Texture_1/" + modelName + ".png";
    cout << texturePath << endl;
    if (std::filesystem::exists(texturePath)) {
        model.textureID = TextureManager::getTexture(texturePath.string().c_str());
    }

    glGenVertexArrays(1, &model.VAO);
    glGenBuffers(1, &model.VBO);
    glGenBuffers(1, &model.EBO);

    glBindVertexArray(model.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, model.VBO);
    glBufferData(GL_ARRAY_BUFFER, model.vertices.size() * sizeof(Vertex), &model.vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, model.indices.size() * sizeof(unsigned int), &model.indices[0], GL_STATIC_DRAW);

    // vertex position -> location 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    // vertex color -> location 1
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);

    // vertex normal -> location 2
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);

    // UV -> location 3
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);
}


void loadSingleFishModel(const std::string& path, Model& model) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "Error: " << importer.GetErrorString() << std::endl;
        return;
    }

    aiMesh* mesh = scene->mMeshes[0];

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        vertex.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        vertex.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        vertex.texCoords = mesh->mTextureCoords[0] ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y) : glm::vec2(0.0f, 0.0f);
        // vertex color
        if (mesh->mColors[0]) { // check if have vertex color
            vertex.color = glm::vec3(mesh->mColors[0][i].r, mesh->mColors[0][i].g, mesh->mColors[0][i].b);
        }
        else {
            vertex.color = glm::vec3(1.0f, 1.0f, 1.0f); // no color just white
        }

        model.vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            model.indices.push_back(face.mIndices[j]);
    }

    std::filesystem::path modelFilePath(path);
    std::string modelDirectory = modelFilePath.parent_path().parent_path().string();
    std::string modelName = modelFilePath.stem().string();

    cout << modelName << endl;
    bool isConstTex = false;

    size_t pos = modelName.find('_');
    if (pos != std::string::npos) {
        string result = modelName.substr(0, pos);
        modelName = result;
    }

    std::filesystem::path texturePath = modelDirectory + "/Texture_3/" + modelName + ".png";
    cout << texturePath << endl;
    if (std::filesystem::exists(texturePath)) {
        model.textureID = TextureManager::getTexture(texturePath.string().c_str());
    }

    glGenVertexArrays(1, &model.VAO);
    glGenBuffers(1, &model.VBO);
    glGenBuffers(1, &model.EBO);

    glBindVertexArray(model.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, model.VBO);
    glBufferData(GL_ARRAY_BUFFER, model.vertices.size() * sizeof(Vertex), &model.vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, model.indices.size() * sizeof(unsigned int), &model.indices[0], GL_STATIC_DRAW);

    // vertex position -> location 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    // vertex color -> location 1
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);

    // vertex normal -> location 2
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);

    // UV -> location 3
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);
}

// ��������ģ��
void loadModels(const std::vector<std::string>& fbxFiles) {
    //�ȴ�������TexID
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

    // ������պе�VAO��VBO
    GLuint skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
}

void loadFishModels(const std::vector<std::string>& fbxFiles) {
    //�ȴ�������TexID
    for (const auto& file : fbxFiles) {
        Model model;
        loadSingleFishModel(file, model);
        fishmodels.push_back(model);
    }

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

void renderSkybox(GLuint skyboxShader, GLuint skyboxVAO, GLuint cubemapTexture, glm::mat4 view, glm::mat4 projection) {
    // ������Ȳ��ԣ�ʹ��պ�ʼ����ʾ�ڱ���
    glDepthFunc(GL_LEQUAL);  // ������Ⱥ������������С�ڻ���ڵĲ���ͨ������
    glDepthMask(GL_FALSE);   // ��ֹд����Ȼ�����

    // ʹ����պ���ɫ��
    glUseProgram(skyboxShader);

    // set view and projection Matrix
    glm::mat4 skyboxView = glm::mat4(glm::mat3(view));
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader, "view"), 1, GL_FALSE, glm::value_ptr(skyboxView));
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // ����պе���������ͼ
    glBindVertexArray(skyboxVAO);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

    // ������պ�
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    // �ָ���Ȼ�����������
    glDepthMask(GL_TRUE);    // �ָ����д��
    glDepthFunc(GL_LESS);    // �ָ���Ȳ���
}

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
    // ��ȡ�������ͼ��ͶӰ����
    // ��ȡ��ǰʱ�䣨�룩
    float currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    // ����ʱ�������������ɫ��
    glUniform1f(glGetUniformLocation(shaderProgram_use, "uTime"), currentTime);
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 projection = camera.getProjectionMatrix(1920.0f / 1080.0f);  // ���贰����800x800
    renderSkybox(skyboxShaderProgram_use, skyboxVAO, cubemapTexture, view, projection);
    glDepthFunc(GL_LESS);      // �ָ�Ĭ����Ȳ���ģʽ
    glDepthMask(GL_TRUE);       // ����д����Ȼ�����
    //drawing
    // ʹ����ɫ������
    glUseProgram(shaderProgram_use);

    GLint lightDirLoc = glGetUniformLocation(shaderProgram_use, "lightDirection");
    GLint lightColorLoc = glGetUniformLocation(shaderProgram_use, "lightColor");
    GLint smoothnessLoc = glGetUniformLocation(shaderProgram_use, "smoothness");

    // ����Ĭ�ϵĹ��շ��򡢹�����ɫ�� smooth ֵ
    glUniform3fv(lightDirLoc, 1, &lightDirection[0]);
    glUniform3fv(lightColorLoc, 1, &lightColor[0]);
    glUniform1f(smoothnessLoc, smoothness);

    // control camera speed
    float currentFrame = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;



    // transfer view and projection Matrix to shader
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram_use, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram_use, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    //sub animation
    UpdateFishTail(currentTime); // update the tail
    ApplyFishTailTransform();    // apply the tail animation

    float speed = 0.5f;  // speed
    // hierarchy (big animation)
    float translationDistance = speed * currentTime *0.1f;  // ����ƽ�ƾ���
    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(translationDistance, 0.0f, 0.0f));

    for (int i = 0; i < fishmodels.size(); i++) {
        glBindVertexArray(fishmodels[i].VAO);

        glm::mat4 modelMatrix = glm::mat4(1.0f);

        //just the tail sub animate
        if (i == 0)
        {
            modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, -i * 0.5f)); // 
            modelMatrix *= fishTailJoints[i % fishTailJoints.size()].transform; 
        }
        else {
            modelMatrix = glm::translate(modelMatrix, translation);
        }
        modelMatrix = translationMatrix * modelMatrix;


        glUniformMatrix4fv(glGetUniformLocation(shaderProgram_use, "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));

        glDrawElements(GL_TRIANGLES, fishmodels[i].indices.size(), GL_UNSIGNED_INT, 0);
    }



    for (const auto& model : models) {
        glBindVertexArray(model.VAO);

        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, translation);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram_use, "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, model.textureID);
        glUniform1i(glGetUniformLocation(shaderProgram_use, "ourTexture"), 0);

        glDrawElements(GL_TRIANGLES, model.indices.size(), GL_UNSIGNED_INT, 0);
    }

    glBindVertexArray(0);  // �����
    TwDraw();

    glutSwapBuffers();
    glutPostRedisplay();
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL error: " << err << std::endl;
    }
}

// ��ʼ�����ں� OpenGL ����
void initGL() {
    // ����������ɫΪ��ɫ
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    shaderProgram_use = createShader("../Project1_64/shader/vertex_shader.glsl", "../Project1_64/shader/fragment_shader.glsl");
    skyboxShaderProgram_use = createShader("../Project1_64/shader/vertex_skybox.glsl", "../Project1_64/shader/fragment_skybox.glsl");
    faces = getAllTexFiles("C:/Users/555/Desktop/assignment/CG_Project_1/SkyBoxTexture");
    cubemapTexture = loadCubemap(faces);
    // ������Ȳ���
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

}

//get all FBX
std::vector<std::string> getAllFBXFiles(const std::string& folderPath) {
    std::vector<std::string> fbxFiles;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(folderPath)) {
        if (entry.path().extension() == ".fbx") {
            fbxFiles.push_back(entry.path().string());
        }
    }

    return fbxFiles;
}



bool mousePressed = false;     // if press or not
float lastX, lastY;            // last month position

void processNormalKeys(unsigned char key, int x, int y) {
    float deltaTime = 0.1f;
    camera.processKeyboard(key, deltaTime);
    glutPostRedisplay();
}

void mouseButtonCallback(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            mousePressed = true;
            lastX = x;
            lastY = y;
        }
        else if (state == GLUT_UP) {
            mousePressed = false;
        }
    }
}


void mouseMotionCallback(int x, int y) {
    if (mousePressed) {
        float xOffset = x - lastX;
        float yOffset = lastY - y; 

        lastX = x;
        lastY = y;

        // adapt the perpective by the movement
        camera.processMouseMovement(xOffset, yOffset);
        glutPostRedisplay();
    }
}

void reshape(int width, int height) {
    glViewport(0, 0, width, height);
    TwWindowSize(width, height);
}

void keyboard(unsigned char key, int x, int y) {
    if (key == 27) { // Escape key
        TwTerminate();
        exit(0);
    }
    TwEventKeyboardGLUT(key, x, y);
}

void mouse(int button, int state, int x, int y) {
    TwEventMouseButtonGLUT(button, state, x, y);
}

void motion(int x, int y) {
    TwEventMouseMotionGLUT(x, y);
}
// ���ڴ�С
int windowWidth = 1920;
int windowHeight = 1080;

// ��ʼ�� OpenGL �� AntTweakBar
void initOpenGLAndAntTweakBar() {
    // ��ʼ�� AntTweakBar
    TwInit(TW_OPENGL, NULL);

    // ����һ���µ� Tweak Bar
    TwBar* bar = TwNewBar("Settings");

    // ���� Tweak Bar ��λ�úʹ�С
    // �� UI ���ڴ��ڵ����½�
    int barWidth = 200;  // Tweak Bar �Ŀ��
    int barHeight = 150; // Tweak Bar �ĸ߶�
    TwWindowSize(windowWidth, windowHeight);

    // ʹ����ʱ�������ò���
    int position[] = { windowWidth - barWidth, windowHeight - barHeight };
    TwSetParam(bar, NULL, "position", TW_PARAM_INT32, 2, position);

    int size[] = { barWidth, barHeight };
    TwSetParam(bar, NULL, "size", TW_PARAM_INT32, 2, size);

    // ���һ���򵥵İ�ť��һ����������� Tweak Bar
    static float value = 0.0f;
    TwAddVarRW(bar, "Value", TW_TYPE_FLOAT, &value, " label='Value' min=0 max=100 step=1 ");
}

void renderSceneUI() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // ��Ⱦ��� OpenGL ����...

    // ��Ⱦ AntTweakBar UI
    TwDraw();

    glutSwapBuffers();
    glutPostRedisplay();
}


int main(int argc, char** argv) {
    std::cout << "Current working directory: " << std::filesystem::current_path() << std::endl;
    // Set up the window


    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("Scene Rendering");
    // Tell glut where the display function is

    // ��ʼ�� GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }
    initGL();
    initOpenGLAndAntTweakBar();

    glutDisplayFunc(renderScene);

    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);

    InitializeFishTail();


    std::vector<std::string> fbxfishFiles = getAllFBXFiles("C:/Users/555/Desktop/assignment/CG_Project_1/Anim/FBX_3");

    loadFishModels(fbxfishFiles);
    //// ����ģ�Ͳ����� OpenGL ������
    //std::vector<std::string> fbxFiles = getAllFBXFiles("C:/Users/555/Desktop/assignment/CG_Project_1/FBX_3");
    ////loadModels(fbxFiles);


    glutKeyboardFunc(processNormalKeys);
    glutMouseFunc(mouseButtonCallback);       
    glutMotionFunc(mouseMotionCallback);    
    glutMainLoop(); 
    return 0;
}











