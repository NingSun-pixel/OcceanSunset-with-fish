#ifndef FISH_SIMULATION_H
#define FISH_SIMULATION_H

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>
#include "Camera.h"
#include "ModelLoader.h"

struct FishInstance {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 scale;
    float rotation;
};

class FishSimulation {
public:
    // ���캯������������
    FishSimulation(int numInstances, Camera& camera);
    ~FishSimulation();

    // ��ʼ����Ⱥʵ��
    void initFishInstances();
    // ������ģ��
    void loadFishModels(const std::vector<std::string>& fbxFiles);
    // ������Ⱥ
    void updateFish(float deltaTime);
    // ��Ⱦ��Ⱥ
    void renderFish(GLuint shaderProgram);
    void loadSingleGPUModel(const std::string& path, Model& model, FishInstance instance);
    void loadSeparateGPUModels(const std::vector<std::string>& fbxFiles, std::vector<Model>& models, std::vector<FishInstance> instances);

private:
    int numInstances;                             // ��Ⱥ����
    std::vector<FishInstance> instances;          // ��ʵ������
    GLuint instanceVBO;                           // ʵ�����������
    std::vector<Model> fishModels;                // ��ģ���б�
    Camera& camera;                               // �������
};

#endif // FISH_SIMULATION_H
