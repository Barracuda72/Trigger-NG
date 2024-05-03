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

See doc/BUILDING.txt
