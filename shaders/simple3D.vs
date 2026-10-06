#version 400 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor; // 每个顶点的颜色

out vec3 ourColor; // 把颜色数据输出到片段着色器

// The 3D layer lives in the w = 0 hyperplane, so it is ordered by the same 4D
// distance as the 4D objects (see simple.fs).
out float vDepthW;

uniform mat3 model_rot;
uniform vec3 model_off;
uniform mat4 view3D;
uniform mat4 projection;

uniform mat4 view4D_rot;
uniform vec4 view4D_off;
uniform float view4D_depth_near;
uniform vec2 view4D_depth_range;

void main() {
  vec3 position = model_rot * aPos + model_off;
  gl_Position = projection * view3D * vec4(position, 1.0);

  vDepthW = 0.0;
  if (view4D_depth_near > 0.0 || view4D_depth_range.y > view4D_depth_range.x) {
    vec4 q = view4D_rot * (vec4(position, 0.0) - view4D_off);
    vDepthW = (view4D_depth_near > 0.0) ? 1.0 / max(q.w, view4D_depth_near) : q.w;
  }

  ourColor = aColor;
}
