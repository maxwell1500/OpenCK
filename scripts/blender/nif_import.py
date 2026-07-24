#!/usr/bin/env python3
"""
NIF Import Script for Blender
Imports a NIF file into Blender using PyNifly or NifTools addon.

Usage: blender --background --python nif_import.py -- input.nif output.blend

Licensed under GPL-3.0 (compatible with OpenCK license)
"""

import bpy
import bmesh
import sys
import os

def import_nif(nif_path, blend_path):
    """Import a NIF file into Blender."""

    try:
        from io_scene_nifly.nif_import import NifImportOperator
        use_pynifly = True
        print("Using PyNifly for import")
    except ImportError:
        use_pynifly = False
        print("PyNifly not found, trying NifTools...")

    if use_pynifly:
        try:
            bpy.ops.import_scene.nif(filepath=nif_path)
            print(f"Imported using PyNifly from: {nif_path}")

            if blend_path:
                bpy.ops.wm.save_as_mainfile(filepath=blend_path)
                print(f"Saved to: {blend_path}")

        except Exception as e:
            print(f"Error importing with PyNifly: {e}")
            return False
    else:
        try:
            bpy.ops.import_scene.nif(filepath=nif_path)
            print(f"Imported using NifTools from: {nif_path}")

            if blend_path:
                bpy.ops.wm.save_as_mainfile(filepath=blend_path)
                print(f"Saved to: {blend_path}")

        except Exception as e:
            print(f"Error importing with NifTools: {e}")
            return False

    return True

def main():
    args = sys.argv[sys.argv.index("--") + 1:]

    if len(args) < 1:
        print("Usage: blender --background --python nif_import.py -- input.nif [output.blend]")
        sys.exit(1)

    nif_path = args[0]
    blend_path = args[1] if len(args) > 1 else None

    if not os.path.exists(nif_path):
        print(f"Error: NIF file not found: {nif_path}")
        sys.exit(1)

    success = import_nif(nif_path, blend_path)

    if not success:
        sys.exit(1)

if __name__ == '__main__':
    main()
