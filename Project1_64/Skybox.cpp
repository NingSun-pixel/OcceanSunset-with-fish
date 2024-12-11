#include "Skybox.h"

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
