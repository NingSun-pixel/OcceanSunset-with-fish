#ifndef LIGHTINGMANAGER_H
#define LIGHTINGMANAGER_H

#include <iostream>
#include <glm/glm.hpp>

class LightingManager {
private:
    // ˽�й��캯����ȷ���ⲿ�޷�ʵ����
    LightingManager()
        : lightDirection(glm::normalize(glm::vec3(-0.55f, -0.55f, -0.66f))),
        lightColor(glm::vec3(1.0f, 1.0f, 1.0f)),
        smoothness(0.6f) {}

    // ��ֹ�����͸�ֵ
    LightingManager(const LightingManager&) = delete;
    LightingManager& operator=(const LightingManager&) = delete;

    // �ڲ��洢��ȫ�ֱ���
    glm::vec3 lightDirection;
    glm::vec3 lightColor;
    glm::float64 smoothness;

public:
    // ��ȡ����ʵ��
    static LightingManager& getInstance() {
        static LightingManager instance; // ��̬�ֲ�������ȷ��Ψһʵ��
        return instance;
    }

    // Getter �� Setter ����
    const glm::vec3& getLightDirection() const { return lightDirection; }
    void setLightDirection(const glm::vec3& direction) { lightDirection = glm::normalize(direction); }

    const glm::vec3& getLightColor() const { return lightColor; }
    void setLightColor(const glm::vec3& color) { lightColor = color; }

    glm::float64 getSmoothness() const { return smoothness; }
    void setSmoothness(glm::float64 value) { smoothness = value; }
};

#endif // LIGHTINGMANAGER_H
