#ifndef MODELLOADER_H
#define MODELLOADER_H

#include <vector>
#include <string>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include <filesystem>
#include "TextureManager.h"
using namespace std;

struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 normal;
    glm::vec2 texCoords;
};

struct Model {
    unsigned int VAO, VBO, EBO;
    unsigned int textureID_D;
    unsigned int textureID_N;
    unsigned int textureID_R;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

//
//// Ĭ�Ϲ��շ������ɫ
//static glm::vec3 lightDirection = glm::normalize(glm::vec3(-0.55f, -0.55f, -0.66f)); // б���µĹ��շ���
//static glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);  // ��ɫ��
//// ������ĳ��Դ�ļ�
//static glm::float64 smoothness = 0.6f;



void loadSingleModel(const std::string& path, Model& model);
void loadSeparateModels(const std::vector<std::string>& fbxFiles, std::vector<Model>& models);
std::vector<std::string> getAllFBXFiles(const std::string& folderPath);

#endif // MODELLOADER_H
