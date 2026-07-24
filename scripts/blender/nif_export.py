#!/usr/bin/env python3
"""
NIF Export Script for Blender
Exports selected Blender objects to NIF format using PyNifly or NifTools addon.

Usage: blender --background --python nif_export.py -- input.blend output.nif [game]

Licensed under GPL-3.0 (compatible with OpenCK license)
"""

import bpy
import sys
import os

def export_nif(blend_path, nif_path, game="SKYRIM"):
    """Export Blender objects to NIF format."""

    if blend_path and os.path.exists(blend_path):
        bpy.ops.wm.open_mainfile(filepath=blend_path)

    try:
        from io_scene_nifly.nif_import import NifImportOperator
        use_pynifly = True
        print("Using PyNifly for export")
    except ImportError:
        use_pynifly = False
        print("PyNifly not found, trying NifTools...")

    if use_pynifly:
        try:
            bpy.ops.export_scene.nif(
                filepath=nif_path,
                game=game,
                filter_glob="*.nif",
                use_selection=True,
                apply_modifiers=True
            )
            print(f"Exported using PyNifly to: {nif_path}")
        except Exception as e:
            print(f"Error exporting with PyNifly: {e}")
            return False
    else:
        try:
            bpy.ops.export_scene.nif(
                filepath=nif_path,
                filter_glob="*.nif",
                use_selection=True,
                apply_modifiers=True
            )
            print(f"Exported using NifTools to: {nif_path}")
        except Exception as e:
            print(f"Error exporting with NifTools: {e}")
            return False

    return True

def main():
    args = sys.argv[sys.argv.index("--") + 1:]

    if len(args) < 2:
        print("Usage: blender --background --python nif_export.py -- input.blend output.nif [game]")
        print("Games: SKYRIM, SKYRIMSE, FO4, FO76, FNV, FO3")
        sys.exit(1)

    blend_path = args[0]
    nif_path = args[1]
    game = args[2] if len(args) > 2 else "SKYRIM"

    success = export_nif(blend_path, nif_path, game)

    if not success:
        sys.exit(1)

if __name__ == '__main__':
    main()
