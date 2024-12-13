#version 330 core
out vec4 FragColor;
in vec3 TexCoords;

uniform samplerCube skybox;
uniform vec3 lightColor;      // ƽ�й����ɫ
uniform float smoothness;//�����ƹ�ǿ��

void main() {
    FragColor = texture(skybox, TexCoords);
    float gray = (FragColor.r + FragColor.g + FragColor.b)/3;
    FragColor = vec4(gray, gray, gray, 1.0) * vec4(lightColor, 1.0) * smoothness/5.0f;
    float gray2 = (FragColor.r + FragColor.g + FragColor.b) / 3;
    vec3 grayV = vec3(gray2, gray2, gray2);
    FragColor = vec4(mix(grayV, FragColor.rgb, 0.5), 1.0);
}
