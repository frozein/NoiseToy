# NoiseToy

NoiseToy is a very primitive library for dynamically generating various types of noise textures in realtime on the GPU, with additional support for exporting them. It uses OpenGL compute shaders, and was written in less than 1000 LOC. I made this solely for personal use and made it public for learning purposes, so please do not expect any level of polish.

## Features
* Perlin noise
* Worley noise
* Worley-box noise (variation on worley I developed, uses a box SDF instead of a circle)
* Ability to have different noise types/parameters in each channel
* Ability to layer different noise types in a single channel
* Exporting (to an uncompressed mem dump)
* 3D noise

## Building
NoiseToy depends on GLFW, GLAD, and Dear ImGui, so make sure these libraries are visible to your package manager of choice. To build, simply `cmake` in the root directory and the build files will be generated.

## Screenshots

![alt text](https://github.com/frozein/NoiseToy/blob/master/assets/screenshots/1.png)
![alt text](https://github.com/frozein/NoiseToy/blob/master/assets/screenshots/2.png)
