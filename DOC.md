# Setup
1. Download the NDK & SDK(.jar) for termux. If not available, patch it yourself.
2. Copy the .env template and call source pn it (or use dotenv loader)
3. Create /build/ directory and generate the build files there with cmake .. --preset=
4. Build it.


# TODO
1. Figure out a way to hand over loop control & event handling over from cunoandr.c.
2. Set up graphic.c: Abstract OpenGL & Vulkan stuff. I imagine we have functions such as InitGraphics(width, height, otheroptions), Draw(), and Swap(). Don't be afraid to act as just a wrapper
3. Draw a triangle in main.c: make it work!




