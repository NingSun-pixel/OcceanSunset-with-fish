#version 330 core

// ��������
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoords;

// ʵ��������
layout(location = 4) in vec3 instancePosition;    // ʵ��λ��
layout(location = 5) in vec3 instanceVelocity;    // ʵ���ٶ�
layout(location = 6) in vec3 instanceScale;       // ʵ������
layout(location = 7) in float instanceRotation;   // ʵ����ת�Ƕ�
uniform float deltaTime;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 VertColor;
out vec2 TexCoords;
out vec3 FragPos;
out vec3 Normal;

void main() {
    // �������λ��
    vec3 updatedPosition = instancePosition + instanceVelocity * deltaTime;

    // ��ת�任
    float angle = radians(instanceRotation);
    mat4 rotationMatrix = mat4(
        vec4(cos(angle), 0.0, sin(angle), 0.0),
        vec4(0.0, 1.0, 0.0, 0.0),
        vec4(-sin(angle), 0.0, cos(angle), 0.0),
        vec4(0.0, 0.0, 0.0, 1.0)
    );

    // ���ű任
    mat4 scaleMatrix = mat4(
        vec4(instanceScale.x, 0.0, 0.0, 0.0),
        vec4(0.0, instanceScale.y, 0.0, 0.0),
        vec4(0.0, 0.0, instanceScale.z, 0.0),
        vec4(0.0, 0.0, 0.0, 1.0)
    );

    // ƽ�Ʊ任
    mat4 translationMatrix = mat4(
        vec4(1.0, 0.0, 0.0, 0.0),
        vec4(0.0, 1.0, 0.0, 0.0),
        vec4(0.0, 0.0, 1.0, 0.0),
        vec4(updatedPosition, 1.0)
    );


    mat4 modelMatrix = translationMatrix * rotationMatrix * scaleMatrix;

    gl_Position = projection * view * translationMatrix * vec4(inPosition.x, inPosition.z, inPosition.y, 1.0);
    Normal = mat3(transpose(inverse(model))) * inNormal;
    VertColor = updatedPosition; // ������ɫ
    TexCoords = inTexCoords;
}
