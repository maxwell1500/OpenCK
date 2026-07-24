#!/usr/bin/env python3
"""
NIF Preview Generator Script for Blender
Generates preview images of NIF files from multiple angles.

Usage: blender --background --python preview_generator.py -- input.nif output_dir [width] [height] [num_angles]

Licensed under GPL-3.0 (compatible with OpenCK license)
"""

import bpy
import sys
import os
import math

def generate_preview(nif_path, output_dir, width=256, height=256, num_angles=4):
    """Generate preview images of a NIF file from multiple angles."""

    try:
        bpy.ops.import_scene.nif(filepath=nif_path)
    except Exception as e:
        print(f"Failed to import NIF: {e}")
        return False

    imported_meshes = [obj for obj in bpy.context.scene.objects if obj.type == 'MESH']
    if not imported_meshes:
        print("No mesh objects found after import")
        return False

    for obj in imported_meshes:
        bpy.data.objects.remove(bpy.data.objects.get('Cube'), do_unlink=True) if 'Cube' in bpy.data.objects else None
        break

    for obj in bpy.context.scene.objects:
        if obj.type == 'MESH':
            imported_mesh = obj
            break
    else:
        imported_mesh = None

    bpy.ops.object.camera_add(location=(5, 5, 5))
    camera = bpy.context.active_object
    camera.rotation_euler = (math.pi / 4, 0, math.pi / 4)
    bpy.context.scene.camera = camera

    if imported_mesh:
        constraint = camera.constraints.new('TRACK_TO')
        constraint.target = imported_mesh
        constraint.track_axis = 'TRACK_NEGATIVE_Z'
        constraint.up_axis = 'UP_Y'

    bpy.context.scene.render.image_format = 'PNG'
    bpy.context.scene.render.resolution_x = width
    bpy.context.scene.render.resolution_y = height
    bpy.context.scene.render.film_transparent = True

    os.makedirs(output_dir, exist_ok=True)

    angle_step = 360 / num_angles

    for i in range(num_angles):
        angle = math.radians(angle_step * i)

        distance = 5
        camera.location = (
            distance * math.cos(angle),
            distance * math.sin(angle),
            2
        )

        output_path = os.path.join(output_dir, f"preview_{i:02d}.png")
        bpy.context.scene.render.filepath = output_path
        bpy.ops.render.render(write_still=True)
        print(f"Rendered preview {i+1}/{num_angles}: {output_path}")

    print(f"Generated {num_angles} previews in: {output_dir}")
    return True

def main():
    args = sys.argv[sys.argv.index("--") + 1:]

    if len(args) < 2:
        print("Usage: blender --background --python preview_generator.py -- input.nif output_dir [width] [height] [num_angles]")
        print("Defaults: width=256, height=256, num_angles=4")
        sys.exit(1)

    nif_path = args[0]
    output_dir = args[1]
    width = int(args[2]) if len(args) > 2 else 256
    height = int(args[3]) if len(args) > 3 else 256
    num_angles = int(args[4]) if len(args) > 4 else 4

    if not os.path.exists(nif_path):
        print(f"Error: NIF file not found: {nif_path}")
        sys.exit(1)

    success = generate_preview(nif_path, output_dir, width, height, num_angles)

    if not success:
        sys.exit(1)

if __name__ == '__main__':
    main()
