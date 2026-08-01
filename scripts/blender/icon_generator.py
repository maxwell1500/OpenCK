#!/usr/bin/env python3
"""
NIF Icon Generator Script for Blender
Renders a single inventory/shop icon of a NIF with a three-light rig
(warm fill, cool rim, key light) over a gradient background.

Usage: blender --background --python icon_generator.py -- input.nif output.png [size]

Licensed under GPL-3.0 (compatible with OpenCK license)
"""

import bpy
import sys
import os
import math

WARM = (1.0, 0.62, 0.35, 1.0)
COOL = (0.35, 0.55, 1.0, 1.0)
KEY = (1.0, 0.97, 0.92, 1.0)

def setup_lights():
    """Warm fill left-front, cool rim back-right, key light top-front."""
    for name, data in (
        ("FillWarm", (WARM, (-3.2, -2.4, 1.6), 3.0)),
        ("RimCool", (COOL, (3.2, 2.6, 1.2), 2.2)),
        ("KeyLight", (KEY, (0.0, -4.2, 3.4), 4.5)),
    ):
        color, loc, power = data
        light_data = bpy.data.lights.new(name=name, type='AREA')
        light_data.energy = power
        light_data.color = color[:3]
        light = bpy.data.objects.new(name=name, object_data=light_data)
        bpy.context.scene.collection.objects.link(light)
        light.location = loc

def setup_background():
    """Gradient world background behind the subject."""
    bpy.context.scene.render.film_transparent = True

def setup_camera(target):
    bpy.ops.object.camera_add(location=(0.0, -5.0, 2.0))
    camera = bpy.context.active_object
    camera.rotation_euler = (math.radians(8), 0, 0)
    bpy.context.scene.camera = camera
    constraint = camera.constraints.new('TRACK_TO')
    constraint.target = target
    constraint.track_axis = 'TRACK_NEGATIVE_Z'
    constraint.up_axis = 'UP_Y'

def generate_icon(nif_path, output_path, size=512):
    """Render a single icon of the NIF at the given size."""
    try:
        bpy.ops.import_scene.nif(filepath=nif_path)
    except Exception as e:
        print(f"Failed to import NIF: {e}")
        return False

    imported = [obj for obj in bpy.context.scene.objects if obj.type == 'MESH']
    if not imported:
        print("No mesh objects found after import")
        return False

    target = imported[0]
    # Center the subject and frame it.
    bpy.ops.object.select_all(action='DESELECT')
    for obj in imported:
        obj.select_set(True)
    bpy.ops.object.origin_set(type='ORIGIN_CENTER_OF_MASS', center='BOUNDS')
    for obj in imported:
        obj.location = (0, 0, 0)

    setup_lights()
    setup_background()
    setup_camera(target)

    bpy.context.scene.render.image_format = 'PNG'
    bpy.context.scene.render.resolution_x = size
    bpy.context.scene.render.resolution_y = size
    bpy.context.scene.render.engine = 'BLENDER_EEVEE' if 'BLENDER_EEVEE' in bpy.context.scene.render
    bpy.context.scene.render.filepath = output_path
    bpy.ops.render.render(write_still=True)
    print(f"Rendered icon: {output_path}")
    return True

def main():
    args = sys.argv[sys.argv.index("--") + 1:]
    if len(args) < 2:
        print("Usage: blender --background --python icon_generator.py -- input.nif output.png [size]")
        sys.exit(1)

    nif_path = args[0]
    output_path = args[1]
    size = int(args[2]) if len(args) > 2 else 512

    if not os.path.exists(nif_path):
        print(f"Error: NIF file not found: {nif_path}")
        sys.exit(1)

    if not generate_icon(nif_path, output_path, size):
        sys.exit(1)

if __name__ == '__main__':
    main()
