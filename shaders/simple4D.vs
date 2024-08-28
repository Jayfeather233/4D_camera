#version 400 core
layout(location = 0) in vec4 aPos;
layout(location = 1) in vec3 aColor; // 顶点颜色

out vec3 ourColor; // 向片段着色器输出颜色

// no mat5 so
uniform mat4 model_rot;
uniform vec4 model_off;

uniform mat4 view4D_rot;
uniform vec4 view4D_off;
uniform mat4 view3D;

uniform mat4 projection;

void main() {
  gl_Position = projection * view3D * view4D_rot * (model_rot * aPos + model_off - view4D_off);

  ourColor = aColor * (1.0 - gl_Position.w / 50.0) +
             vec3(0.0, 0.0, 0.0) * gl_Position.w / 50.0;
}
