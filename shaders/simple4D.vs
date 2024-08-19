#version 330 core
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
  vec4 cam_pos_4D = view4D_rot * (model_rot * (aPos + model_off) + view4D_off);
  // vec4 cam_pos_4D = view4D_rot * (aPos + view4D_off);
  float ww = cam_pos_4D.w;
  cam_pos_4D.w = 1.0;
  gl_Position = projection * view3D * cam_pos_4D;
  gl_Position.w *= ww;
  ourColor = aColor * (1.0 - gl_Position.w / 100.0) +
             vec3(0.0, 0.0, 0.0) * gl_Position.w / 100.0;
}
