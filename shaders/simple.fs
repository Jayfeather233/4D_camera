#version 330 core
in vec3 ourColor;
in float vDepthW;

out vec4 FragColor;

// The scene orders fragments by the real 4D distance (see simple4D.vs), which
// has to be done per fragment: interpolating it linearly between vertices is
// only approximate and would dither the winner wherever two surfaces are almost
// equally far away in 4D.
uniform float view4D_depth_near;
uniform vec2 view4D_depth_range;
// Small bias used to keep the wireframe overlay on top of the filled surface.
uniform float depth_bias;

void main() {
  FragColor = vec4(ourColor, 1.0);

  float depth;
  if (view4D_depth_near > 0.0) {
    // vDepthW = 1 / q.w, and 1 - near/q.w is the window depth of a 4D perspective.
    depth = 1.0 - view4D_depth_near * vDepthW;
  } else if (view4D_depth_range.y > view4D_depth_range.x) {
    depth = (vDepthW - view4D_depth_range.x) / (view4D_depth_range.y - view4D_depth_range.x);
  } else {
    depth = gl_FragCoord.z;
  }
  gl_FragDepth = clamp(depth + depth_bias, 0.0, 1.0);
}
