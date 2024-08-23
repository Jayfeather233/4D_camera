#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor; // 顶点颜色

out vec3 ourColor; // 向片段着色器输出颜色

uniform mat4 view3D;
uniform mat4 projection;

void main() {
  gl_Position = projection * view3D * vec4(aPos, 1.0);
  ourColor = aColor;
}
