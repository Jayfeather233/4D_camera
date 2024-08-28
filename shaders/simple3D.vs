#version 400 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor; // 顶点颜色

out vec3 ourColor; // 向片段着色器输出颜色

uniform mat3 model_rot;
uniform vec3 model_off;
uniform mat4 view3D;
uniform mat4 projection;

void main() {
  gl_Position = projection * view3D * vec4(model_rot * aPos + model_off, 1.0);
  ourColor = aColor;
}
