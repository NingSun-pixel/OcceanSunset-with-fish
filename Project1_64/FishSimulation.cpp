#include "FishSimulation.h"
#include <random>

// �������������������
float randomFloat(float min, float max) {
    static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
}

// ���캯��
FishSimulation::FishSimulation(int numInstances, Camera& camera)
    : numInstances(numInstances), camera(camera) {}

// ��������
FishSimulation::~FishSimulation() {
    glDeleteBuffers(1, &instanceVBO);
}

// ��ʼ����Ⱥʵ��
void FishSimulation::initFishInstances() {
    instances.resize(numInstances);
    for (int i = 0; i < numInstances; ++i) {
        instances[i].position = glm::vec3(
            randomFloat(-10.0f, 10.0f),
            randomFloat(-5.0f, 5.0f),
            randomFloat(-10.0f, 10.0f)
        );
        instances[i].velocity = glm::vec3(
            randomFloat(-0.1f, 0.1f),
            randomFloat(-0.1f, 0.1f),
            randomFloat(-0.1f, 0.1f)
        );
        instances[i].scale = glm::vec3(randomFloat(0.5f, 1.5f));
        instances[i].rotation = randomFloat(0.0f, 360.0f);
    }

    // ����ʵ����������
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, instances.size() * sizeof(FishInstance), instances.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// ������ģ��
void FishSimulation::loadFishModel(const std::string& modelPath) {
    loadSingleModel(modelPath,fishModel); // ʹ��ģ�ͼ�����
    if (fishModel.vertices.empty()) {
        std::cerr << "Failed to load fish model!" << std::endl;
        return;
    }

    glGenVertexArrays(1, &fishModel.VAO);
    glGenBuffers(1, &fishModel.VBO);
    glGenBuffers(1, &fishModel.EBO);

    glBindVertexArray(fishModel.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, fishModel.VBO);
    glBufferData(GL_ARRAY_BUFFER, fishModel.vertices.size() * sizeof(Vertex), fishModel.vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, fishModel.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, fishModel.indices.size() * sizeof(unsigned int), fishModel.indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
    glEnableVertexAttribArray(3);

    // ����ʵ��������
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(FishInstance), (void*)offsetof(FishInstance, position));
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);

    glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(FishInstance), (void*)offsetof(FishInstance, velocity));
    glEnableVertexAttribArray(5);
    glVertexAttribDivisor(5, 1);

    glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, sizeof(FishInstance), (void*)offsetof(FishInstance, scale));
    glEnableVertexAttribArray(6);
    glVertexAttribDivisor(6, 1);

    glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, sizeof(FishInstance), (void*)offsetof(FishInstance, rotation));
    glEnableVertexAttribArray(7);
    glVertexAttribDivisor(7, 1);

    glBindVertexArray(0);
}



// ������Ⱥʵ��
void FishSimulation::updateFish(float deltaTime) {
    for (auto& fish : instances) {
        fish.position += fish.velocity * deltaTime;
        if (fish.position.x > 10.0f || fish.position.x < -10.0f) fish.velocity.x *= -1.0f;
        if (fish.position.y > 5.0f || fish.position.y < -5.0f) fish.velocity.y *= -1.0f;
        if (fish.position.z > 10.0f || fish.position.z < -10.0f) fish.velocity.z *= -1.0f;
    }

    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, instances.size() * sizeof(FishInstance), instances.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// ��Ⱦ��Ⱥ
void FishSimulation::renderFish(GLuint shaderProgram) {
    glUseProgram(shaderProgram);
    LightingManager& lighting = LightingManager::getInstance();

    GLint GPUlightDirLoc = glGetUniformLocation(shaderProgram, "lightDirection");
    GLint GPUlightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
    GLint GPUsmoothnessLoc = glGetUniformLocation(shaderProgram, "smoothness");

    // ��ȡ����ز����� Uniform λ��
    GLint fogColorLoc = glGetUniformLocation(shaderProgram, "fogColor");
    GLint fogDensityLoc = glGetUniformLocation(shaderProgram, "fogDensity");
    GLint fogHeightStartLoc = glGetUniformLocation(shaderProgram, "fogHeightStart");
    GLint fogHeightEndLoc = glGetUniformLocation(shaderProgram, "fogHeightEnd");
    GLint fogDistanceStartLoc = glGetUniformLocation(shaderProgram, "fogDistanceStart");
    GLint fogDistanceEndLoc = glGetUniformLocation(shaderProgram, "fogDistanceEnd");

    // ��ȡ LightingManager ��ʵ��
    const LightingManager& lightingManager = LightingManager::getInstance();
    const FogSettings& fogSettings = lightingManager.getFogSettings();

    // ������Ч����
    glUniform3fv(fogColorLoc, 1, &fogSettings.fogColor[0]);
    glUniform1f(fogDensityLoc, fogSettings.fogDensity);
    glUniform1f(fogHeightStartLoc, fogSettings.fogHeightStart);
    glUniform1f(fogHeightEndLoc, fogSettings.fogHeightEnd);
    glUniform1f(fogDistanceStartLoc, fogSettings.fogDistanceStart);
    glUniform1f(fogDistanceEndLoc, fogSettings.fogDistanceEnd);

    // ����Ĭ�ϵĹ��շ��򡢹�����ɫ�� smooth ֵ
    glUniform3fv(GPUlightDirLoc, 1, &lighting.getLightDirection()[0]);
    glUniform3fv(GPUlightColorLoc, 1, &lighting.getLightColor()[0]);
    glUniform1f(GPUsmoothnessLoc, static_cast<float>(lighting.getSmoothness()));
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    // ȫ�ֱ�����ƽ�ƾ���
    glm::vec3 translation(0.0f, 0.0f, 0.0f);
    modelMatrix = glm::translate(modelMatrix, translation);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));

    glm::vec3 viewPos = camera.position;

    // ����λ��
    GLuint viewPosLocation = glGetUniformLocation(shaderProgram, "viewPos");
    glUniform3fv(viewPosLocation, 1, glm::value_ptr(viewPos));
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 projection = camera.getProjectionMatrix(1920.0f / 1080.0f);  // ���贰����1920.0f / 1080.0f
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // �� Diffuse (Albedo) ��ͼ������Ԫ 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fishModel.textureID_D);
    glUniform1i(glGetUniformLocation(shaderProgram, "albedoMap"), 0);

    //// �� Normal ��ͼ������Ԫ 1
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, fishModel.textureID_N);
    glUniform1i(glGetUniformLocation(shaderProgram, "normalMap"), 1);

    // �� Roughness ��ͼ������Ԫ 2
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, fishModel.textureID_R);
    glUniform1i(glGetUniformLocation(shaderProgram, "RoughnessMap"), 2);

    glBindVertexArray(fishModel.VAO);
    glDrawElementsInstanced(GL_TRIANGLES, fishModel.indices.size(), GL_UNSIGNED_INT, 0, numInstances);
    glBindVertexArray(0);
}
