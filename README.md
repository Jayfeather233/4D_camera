# 4D_camera
Render a 4D object to 3D just like using a 4D camera

This project is inspired by: [yugu233](http://yugu233.com/) and his project [4D](https://github.com/Crispher/4D). I think it's interesting and want to build a cpp/openGL version of it instead of typejs/three.js.

Some of the codes taken from [tendo518](https://github.com/tendo518) and [learnopengl](https://learnopengl.com/)

### Compile

dependencies:

```
cmake
glfw3
glew
glm
fmt
```

then run `bash ./build.sh` to build & run.

### Movement:

For moving 4D Cam: `qweasd` for rotation, `zx` for zoom in/out;

For moving 3D Cam: `ijkl` for rotation, `nm` for zoom in/out

### Demo

Here is a simple demo of a 4D Cube floating on a 2D plane, while I rotating it.

![](./imgs/demo_cube_floatingon_plane.gif)


TODO:

more shapes, GUI for displaying different object, both solid/mesh render and line render, camera4D optimization.
