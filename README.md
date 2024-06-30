Trigger-NG
===========
This is a fork of Trigger Rally https://trigger-rally.sourceforge.io/ targeting newer OpenGL (ES) versions.

# Notes

I rewrote rendering code to use VBOs and shaders; nothing else was changed apart from fixing a few stale bugs 
I've encountered in process of rewrite.

Rendering code supports both desktop OpenGL 2.1/3.0+ and OpenGL ES 2.0.

## Fog
Fog calculation is only implemented for a terrain and a sky because I don't want to duplicate those calculations in every shader.
This might lead to some unexpected results (e.g. distant trees being crisp).
Proper implementation would apply a fog effect at the postprocessing stage using depth buffer.

## Lighting
Original game renderer only applies lighting to the car model. I followed along, but it would be a good idea 
to improve on that and add lighting also on the terrain.

# Original README

See doc/README.txt

# Dependencies & Building

The only new dependency is a GLM. You can either use the one provided by your distro's package manager (beware of obsolete versions!) 
or download it manually and place in the current directory. However, the simplest way is to just clone this repository using 
`--recurse-submodules` flag to Git. This will pull a 100% working version of GLM into the project folder.

I've removed a GLU dependency, as it was only required for `gluScaleImage`. Now image scaling is implemented using `SDL_BlitScaled` 
from SDL.

By default the code is compiled with OpenGL 2.1 in mind. Beware that this is the only supported way on Windows/MSYS as of now. 
- To use OpenGL 3.0+ Core Profile, specify `GL30PLUS=1` when running `make`.
- To use OpenGL ES 2.0, specify `GLES2=1` when running `make`. Note that you'll need GLESv2 library and corresponding headers available.

Other than that, the general build process is the same as original; see original doc/BUILDING.txt for more information.
