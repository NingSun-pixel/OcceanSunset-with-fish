#include "FishSimulation.h"
#include <random>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/compatibility.hpp> // For glm::lerp

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

void FishSimulation::initFishInstances() {
    instances.resize(numInstances);

    // ��ʼ�������ĵ�
    centers = {
        glm::vec3(-15.0f, 10.0f, -15.0f),
        glm::vec3(15.0f, 15.0f, 15.0f),
        glm::vec3(-20.0f, 20.0f, 20.0f)
    };

    int numCenters = centers.size();

    for (int i = 0; i < numInstances; ++i) {
        // ������䵽ĳ�����ĵ�
        glm::vec3 center = centers[i % numCenters];

        // ���Ӹ�������ƫ�����Լ��ٹ����ܼ���Ч��
        instances[i].position = glm::vec3(
            randomFloat(center.x - 10.0f, center.x + 10.0f), // ����Χ�� [-10, 10]
            randomFloat(center.y - 5.0f, center.y + 5.0f),   // �߶ȷ�Χ [-5, 5]
            randomFloat(center.z - 10.0f, center.z + 10.0f)  // ����Χ�� [-10, 10]
        );

        // ��������ٶ�
        instances[i].velocity = glm::vec3(
            randomFloat(-0.3f, 0.3f),  // ���ͳ�ʼ�ٶ�
            randomFloat(-0.3f, 0.3f),
            randomFloat(-0.3f, 0.3f)
        );

        // �����ʵ������ŷ�Χ
        instances[i].scale = glm::vec3(randomFloat(1.5f, 2.0f));
        instances[i].rotation = randomFloat(-30.0f, 30.0f);


        // ���������ɫ (RGBA)
        instances[i].color = glm::vec4(
            randomFloat(0.2f, 1.0f), // ��ɫ����
            randomFloat(0.2f, 1.0f), // ��ɫ����
            randomFloat(0.2f, 1.0f), // ��ɫ����
            1.0f                     // Alpha ��͸��
        );
    }

    // ����ʵ����������
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, instances.size() * sizeof(FishInstance), instances.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}


// ������ģ��
void FishSimulation::loadFishModel(const std::string& modelPath) {
    loadSingleModel(modelPath, fishModel); // ʹ��ģ�ͼ�����
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

    glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, sizeof(FishInstance), (void*)offsetof(FishInstance, color));
    glEnableVertexAttribArray(8);
    glVertexAttribDivisor(8, 1); // ÿ��ʵ��ʹ��һ����ɫ

    glBindVertexArray(0);
}

void FishSimulation::updateFish(float deltaTime) {
    static float time = 0.0f;
    static int currentChunk = 0;       // ��ǰ����ķ���
    const int chunkSize = 200;         // ÿ�θ��µ�������
    const float minSpeed = 0.5f;       // ��С�ٶ�
    const float floatAmplitude = 2.0f; // ���¸����ķ���
    const float floatFrequency = 1.0f; // ���¸�����Ƶ��
    const float expansionFactor = 2.0f; // �ۼ�����뾶������
    static bool wasLightTriggered = false; // ���ڼ�Ⲽ��ֵ�ı仯
    time += deltaTime;

    // ��ȡ LightingManager ʵ���Ͳ���ֵ
    LightingManager& lightingManager = LightingManager::getInstance();
    bool lightTriggered = lightingManager.gettoggleLightingPreset(); // �����д˷���

    int start = currentChunk * chunkSize;
    int end = std::min(start + chunkSize, numInstances);

    for (int i = start; i < end; ++i) {
        auto& fish = instances[i];

        if (lightTriggered) {

            // ת��ΪΧ�� x=0, z=0 ������ת��������ۼ�
            glm::vec3 targetAxis = glm::vec3(0.0f, fish.position.y, 0.0f); // x=0, z=0 ��
            glm::vec3 directionToAxis = glm::normalize(targetAxis - fish.position);

            // �ۼ����������ڿ�����
            glm::vec3 attractionForce = directionToAxis * 0.5f; // �ۼ���Ȩ��

            // Բ���˶�����Χ������ת
            glm::vec3 tangent = glm::cross(directionToAxis, glm::vec3(0.0f, 1.0f, 0.0f));
            glm::vec3 rotationalForce = tangent * 2.0f; // ��ת�ٶ�Ȩ��

            // �ۺ��˶�
            fish.velocity = glm::lerp(fish.velocity, attractionForce + rotationalForce, 0.5f);

            // ����λ��
            fish.position += fish.velocity * deltaTime * 10.0f;
        }
        else {
            // �����ۼ��߼�
            glm::vec3 randomAcceleration = glm::vec3(
                randomFloat(-0.3f, 0.3f),
                randomFloat(-0.3f, 0.3f),
                randomFloat(-0.3f, 0.3f)
            );

            glm::vec3 closestCenter = centers[0];
            float minDistance = glm::length(fish.position - centers[0]);
            for (const auto& center : centers) {
                float distance = glm::length(fish.position - center);
                if (distance < minDistance) {
                    minDistance = distance;
                    closestCenter = center;
                }
            }

            // ����ۼ�����İ뾶
            float expandedRadius = minDistance * expansionFactor;

            // ����ۼ���
            glm::vec3 directionToCenter = glm::normalize(closestCenter - fish.position);
            glm::vec3 attractionForce = directionToCenter * glm::clamp(expandedRadius / 15.0f, 0.0f, 1.0f); // �ۼ�������չ�������

            // �����ٶ�
            fish.velocity = glm::lerp(fish.velocity, attractionForce, 0.5f);
            fish.velocity += randomAcceleration * deltaTime * 0.8f; // �����ٶ��Ŷ�

            // �������¸���Ч��
            float individualOffset = static_cast<float>(i) * 0.2f; // Ϊÿ�������һ��ƫ��ֵ
            fish.velocity.y += floatAmplitude * sin(floatFrequency * time + individualOffset);

            // ǿ����С�ٶ�
            if (glm::length(fish.velocity) < minSpeed) {
                fish.velocity += glm::normalize(fish.velocity) * (minSpeed - glm::length(fish.velocity));
            }

            // ����λ��
            fish.position += fish.velocity * deltaTime;
        }

        // �߽練���߼�
        if (fish.position.x > 30.0f * expansionFactor || fish.position.x < -30.0f * expansionFactor) fish.velocity.x *= -1.0f;
        if (fish.position.y > 25.0f * expansionFactor || fish.position.y < 0.0f) fish.velocity.y *= -1.0f;
        if (fish.position.z > 30.0f * expansionFactor || fish.position.z < -30.0f * expansionFactor) fish.velocity.z *= -1.0f;
    }

    // ������������
    currentChunk = (currentChunk + 1) % ((numInstances + chunkSize - 1) / chunkSize);

    // ����ʵ��������
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, start * sizeof(FishInstance), (end - start) * sizeof(FishInstance), &instances[start]);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // ���²���ֵ״̬
    wasLightTriggered = lightTriggered;
}

// ��Ⱦ��Ⱥ
void FishSimulation::renderFish(GLuint shaderProgram) {
    glUseProgram(shaderProgram);
    LightingManager& lighting = LightingManager::getInstance();

    GLint GPUlightDirLoc = glGetUniformLocation(shaderProgram, "lightDirection");
    GLint GPUlightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
    GLint GPUsmoothnessLoc = glGetUniformLocation(shaderProgram, "smoothness");

    // ��ȡ���Դ������ Uniform λ��
    GLint numPointLightsLoc = glGetUniformLocation(shaderProgram, "numPointLights");
    GLint pointLightPositionsLoc = glGetUniformLocation(shaderProgram, "pointLightPositions");
    GLint pointLightColorsLoc = glGetUniformLocation(shaderProgram, "pointLightColors");
    GLint pointLightIntensitiesLoc = glGetUniformLocation(shaderProgram, "pointLightIntensities");
    GLint pointLightRadiiLoc = glGetUniformLocation(shaderProgram, "pointLightRadii");

    // �� LightingManager ��ȡ���Դ����
    const std::vector<PointLight>& pointLights = LightingManager::getInstance().getPointLights();
    int numPointLights = pointLights.size();

    // ���õ��Դ����
    glUniform1i(numPointLightsLoc, numPointLights);

    if (numPointLights > 0) {
        // ׼�����Դ����
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

        // ���ݵ��Դ��������ɫ��
        glUniform3fv(pointLightPositionsLoc, numPointLights, glm::value_ptr(pointLightPositions[0]));
        glUniform3fv(pointLightColorsLoc, numPointLights, glm::value_ptr(pointLightColors[0]));
        glUniform1fv(pointLightIntensitiesLoc, numPointLights, pointLightIntensities.data());
        glUniform1fv(pointLightRadiiLoc, numPointLights, pointLightRadii.data());
    }


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
