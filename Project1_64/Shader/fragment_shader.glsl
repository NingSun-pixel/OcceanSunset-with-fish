#version 330 core

in vec2 TexCoords;
in vec3 vertColor;
out vec4 FragColor;
uniform sampler2D ourTexture;

//���������TexCoords��UVû������(stlû��UV��Ϣ)
void main() {
    //FragColor = vec4(vertColor, 1.0); // ����Ϊ��ɫ
    FragColor = texture(ourTexture, TexCoords);
    if (FragColor.a < 0.5)
    {
        discard; // ����Ƭ��
    }
}
