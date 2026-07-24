#!/usr/bin/env python3
"""
NIF Batch Operations Script for Blender
Processes multiple NIF files at once (export, convert, validate, etc.)

Usage: blender --background --python nif_batch.py -- operation input_dir [output_dir] [options]

Operations:
  export - Export all NIFs in directory to Blender format
  validate - Validate all NIFs for errors
  extract_textures - Extract texture references from NIFs

Licensed under GPL-3.0 (compatible with OpenCK license)
"""

import bpy
import sys
import os
import json

def batch_validate(input_dir):
    """Validate all NIFs in directory."""
    nif_files = [f for f in os.listdir(input_dir) if f.endswith('.nif')]
    print(f"Validating {len(nif_files)} NIF files")

    results = []

    for nif_file in nif_files:
        nif_path = os.path.join(input_dir, nif_file)

        try:
            bpy.ops.import_scene.nif(filepath=nif_path)

            errors = []
            for obj in bpy.context.scene.objects:
                if obj.type == 'MESH':
                    mesh = obj.data
                    if len(mesh.vertices) == 0:
                        errors.append(f"Mesh '{obj.name}' has no vertices")
                    if len(mesh.polygons) == 0:
                        errors.append(f"Mesh '{obj.name}' has no faces")

                    for mat_slot in obj.material_slots:
                        mat = mat_slot.material
                        if mat and mat.use_nodes:
                            for node in mat.node_tree.nodes:
                                if node.type == 'TEX_IMAGE' and node.image is None:
                                    errors.append(f"Material '{mat.name}' has unlinked texture node")

            result = {
                'file': nif_file,
                'valid': len(errors) == 0,
                'errors': errors
            }
            results.append(result)

            if errors:
                print(f"  {nif_file}: {len(errors)} issues")
                for err in errors[:5]:
                    print(f"    - {err}")
            else:
                print(f"  {nif_file}: OK")

            bpy.ops.object.select_all(action='SELECT')
            bpy.ops.object.delete()

        except Exception as e:
            results.append({
                'file': nif_file,
                'valid': False,
                'errors': [str(e)]
            })
            print(f"  {nif_file}: ERROR - {e}")

    report_path = os.path.join(input_dir, "validation_report.json")
    with open(report_path, 'w') as f:
        json.dump(results, f, indent=2)

    print(f"\nValidation complete. Report saved to: {report_path}")
    return True

def batch_extract_textures(input_dir, output_file=None):
    """Extract texture references from all NIFs."""
    if output_file is None:
        output_file = os.path.join(input_dir, "texture_list.json")

    nif_files = [f for f in os.listdir(input_dir) if f.endswith('.nif')]
    print(f"Extracting textures from {len(nif_files)} NIF files")

    all_textures = {}

    for nif_file in nif_files:
        nif_path = os.path.join(input_dir, nif_file)

        try:
            bpy.ops.import_scene.nif(filepath=nif_path)

            textures = set()
            for obj in bpy.context.scene.objects:
                if obj.type == 'MESH':
                    for mat_slot in obj.material_slots:
                        mat = mat_slot.material
                        if mat and mat.use_nodes:
                            for node in mat.node_tree.nodes:
                                if node.type == 'TEX_IMAGE' and node.image:
                                    textures.add(node.image.filepath)

            all_textures[nif_file] = list(textures)

            bpy.ops.object.select_all(action='SELECT')
            bpy.ops.object.delete()

        except Exception as e:
            print(f"  Failed to process {nif_file}: {e}")

    with open(output_file, 'w') as f:
        json.dump(all_textures, f, indent=2)

    print(f"\nTexture extraction complete. Saved to: {output_file}")
    return True

def main():
    args = sys.argv[sys.argv.index("--") + 1:]

    if len(args) < 2:
        print("Usage: blender --background --python nif_batch.py -- operation input_dir [output_dir]")
        print("\nOperations:")
        print("  validate - Validate all NIFs for errors")
        print("  extract_textures - Extract texture references")
        sys.exit(1)

    operation = args[0]
    input_dir = args[1]
    output_dir = args[2] if len(args) > 2 else None

    if operation == "validate":
        batch_validate(input_dir)
    elif operation == "extract_textures":
        output_file = os.path.join(output_dir, "texture_list.json") if output_dir else None
        batch_extract_textures(input_dir, output_file)
    else:
        print(f"Unknown operation: {operation}")
        sys.exit(1)

if __name__ == '__main__':
    main()
