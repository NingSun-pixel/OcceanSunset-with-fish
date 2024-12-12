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

    // ����ʵ���������
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, instances.size() * sizeof(FishInstance), &instances[0], GL_DYNAMIC_DRAW);
}

// ������ģ��
void FishSimulation::loadFishModels(const std::vector<std::string>& fbxFiles) {
    loadSeparateModels(fbxFiles, fishModels); // ʹ�� ModelLoader �ļ��ع���
    if (fishModels.empty()) {
        std::cerr << "Failed to load any fish models!" << std::endl;
    }
}

// ������Ⱥλ��
void FishSimulation::updateFish(float deltaTime) {
    for (auto& fish : instances) {
        fish.position += fish.velocity * deltaTime;

        // �򵥵ı߽練���߼�
        if (fish.position.x > 10.0f || fish.position.x < -10.0f)
            fish.velocity.x *= -1.0f;
        if (fish.position.y > 5.0f || fish.position.y < -5.0f)
            fish.velocity.y *= -1.0f;
        if (fish.position.z > 10.0f || fish.position.z < -10.0f)
            fish.velocity.z *= -1.0f;
    }

    // ���»���������
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, instances.size() * sizeof(FishInstance), &instances[0]);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// ��Ⱦ��Ⱥ
void FishSimulation::renderFish(GLuint shaderProgram) {
    glUseProgram(shaderProgram);

    // �������
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1920.0f / 1080.0f, 0.1f, 100.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    for (const auto& model : fishModels) {
        glBindVertexArray(model.VAO);
        glDrawElementsInstanced(GL_TRIANGLES, model.indices.size(), GL_UNSIGNED_INT, 0, instances.size());
        glBindVertexArray(0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, model.textureID);
        glUniform1i(glGetUniformLocation(shaderProgram, "albedoMap"), 0);
    }
}
