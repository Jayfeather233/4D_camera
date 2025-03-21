#version 400 core
layout(location = 0) in vec4 aPos;
layout(location = 1) in vec3 aColor;

out vec3 ourColor;

// no mat5 so
uniform mat4 model_rot;
uniform vec4 model_off;

uniform mat4 view4D_rot;
uniform vec4 view4D_off;
uniform mat4 view3D;

uniform mat4 projection;

void main() {
  gl_Position = projection * view3D * view4D_rot * (model_rot * aPos + model_off - view4D_off);

  ourColor = aColor;
  // ourColor = aColor * (1.0 - gl_Position.w * 0.02) +
  //            vec3(0.0, 0.0, 0.0) * gl_Position.w * 0.02;
}
