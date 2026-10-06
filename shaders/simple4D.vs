#version 400 core
layout(location = 0) in vec4 aPos;
layout(location = 1) in vec3 aColor;

out vec3 ourColor;

// Both branches are 4D perspective projections of the form
//   screen = f * q.xyz / (D * q.w - q.z)
// where D is the eye distance of the 3D camera (its look-at translation is
// scaled by the homogeneous q.w). They differ in D, in the explicit near clip
// below, and in which quantity the depth comes from.
//
// The depth is not taken from gl_Position: the fragment shader turns this into
// the real 4D distance. vDepthW carries the one quantity the rasteriser
// interpolates exactly for the projection in use:
//   view4D_near <= 0 -> q.w is affine in the 4D point
//   view4D_near >  0 -> 1/q.w is affine in the projected 3D point
out float vDepthW;

// no mat5 so
uniform mat4 model_rot;
uniform vec4 model_off;

uniform mat4 view4D_rot;
uniform vec4 view4D_off;
uniform mat4 view3D;

uniform mat4 projection;

// > 0: project 4D to 3D with a perspective divide along the 4th axis and this
//      value as the near distance. That is what makes movement in the 4th
//      dimension visible (things grow and shrink); it needs GL_CLIP_DISTANCE0.
// <= 0: the orbiting scenes: q goes to the 3D camera as a homogeneous 3D
//       coordinate, which supplies the 4D depth through its perspective.
uniform float view4D_near;

void main() {
  vec4 q = view4D_rot * (model_rot * aPos + model_off - view4D_off);

  if (view4D_near > 0.0) {
    float w = max(q.w, view4D_near);
    gl_Position = projection * view3D * vec4(q.xyz / w, 1.0);
    gl_ClipDistance[0] = q.w - view4D_near;
    vDepthW = 1.0 / w;
  } else {
    gl_Position = projection * view3D * q;
    gl_ClipDistance[0] = 1.0;
    vDepthW = q.w;
  }

  ourColor = aColor;
}
