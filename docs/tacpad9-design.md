# Tacpad 9 / Model Nine product design

The parametric Blender 4.x source is [tacpad9_blender.py](../tacpad9_blender.py).
The product studio is built by [tacpad9_render.py](../tacpad9_render.py).

Run the model script in Blender's Scripting tab. Edit P to change dimensions.
Run the studio script to build materials, printed legends, an OLED pixel UI,
softbox lighting and three product cameras. It saves the scene and photographs.

```sh
blender --background --python tacpad9_render.py
```

The enclosure is 85 x 120 x 22 mm, with nine keys on a 19 mm pitch.
The knob is 16 mm in diameter and 12 mm high. The OLED window is 38 x 16 mm.
Control parts extend above the enclosure. STL coordinates are millimetres.
This is a design concept; switch fits and PCB mounts are not engineered.
