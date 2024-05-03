Trigger-NG
===========
This is a fork of Trigger Rally https://trigger-rally.sourceforge.io/ targeting newer OpenGL (ES) versions.

# Notes

## Fog
Fog calculation is only implemented for a terrain and a sky because I don't want to duplicate those calculations in every shader.
This might lead to some unexpected results (e.g. distant trees being crisp).
Proper implementation would apply a fog effect at the postprocessing stage using depth buffer.

## Lighting
Original game only applies lighting to the car model. I decided to improve on that and plan to add lighting also on terrain.

# Original README

See doc/README.txt

# Dependencies & Building

See doc/BUILDING.txt
