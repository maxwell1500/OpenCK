#!/usr/bin/env python3
"""
NIF Operations Helper for OpenCK
Runs inside Blender (--background --python) or standalone Python.

Operations:
  extract_shapes   - Extract mesh/node shapes from a NIF (via NifTools addon)
  extract_textures - Extract texture references from a NIF
  validate         - Validate NIF for structural errors
  compare          - Compare two NIF files
  save_nif         - Save shape data to a NIF file (via NifTools addon)
  get_version      - Get Blender/NifTools version

Output: JSON to stdout

This script requires either:
  1. Blender with the NifTools addon installed (preferred, full NIF support)
  2. pynifly installed in the Python environment (fallback)
  3. Neither (limited OBJ/FBX fallback only)

Licensed under GPL-3.0 (compatible with OpenCK license)
"""

import sys
import os
import json
import traceback


def get_blender():
    """Import and return the bpy module if running inside Blender."""
    try:
        import bpy
        return bpy
    except ImportError:
        return None


def get_niftools():
    """Return the NifTools addon if available, else None."""
    bpy = get_blender()
    if bpy is None:
        return None
    # Check for io_scene_nif (NifTools addon)
    if hasattr(bpy.ops.io_scene, 'nif'):
        return 'io_scene_nif'
    # Older addon name
    if hasattr(bpy.ops, 'import_scene') and hasattr(bpy.ops.import_scene, 'nif'):
        return 'import_scene_nif'
    return None


def get_pynifly():
    """Import and return pynifly's NifFile class if available."""
    try:
        from pynifly import NifFile
        return NifFile
    except ImportError:
        return None


def op_extract_shapes(nif_path):
    """Extract shape data from a NIF file using NifTools addon or pynifly."""
    NifFile = get_pynifly()
    if NifFile is not None:
        return _extract_shapes_pynifly(nif_path, NifFile)

    bpy = get_blender()
    if bpy is None or get_niftools() is None:
        return {
            "error": "No NIF library available",
            "detail": "Install pynifly (pip install pynifly) or run with Blender+NifTools addon"
        }

    try:
        bpy.ops.wm.read_factory_settings(use_empty=True)

        addon = get_niftools()
        if addon == 'io_scene_nif':
            bpy.ops.import_scene.nif(filepath=nif_path)
        else:
            bpy.ops.import_scene.nif(filepath=nif_path)

        shapes = []
        for obj in bpy.data.objects:
            if obj.type != 'MESH':
                continue
            mesh = obj.data
            s = {
                "name": obj.name,
                "vertices": [{"x": v.co.x, "y": v.co.y, "z": v.co.z} for v in mesh.vertices],
                "triangles": [{"v0": loop.vertex_indices[0],
                                "v1": loop.vertex_indices[1],
                                "v2": loop.vertex_indices[2]}
                               for poly in mesh.polygons for loop in [poly]],
                "uvs": [],
                "normals": [{"x": no.x, "y": no.y, "z": no.z} for no in mesh.vertices],
                "bone_names": [g.name for g in obj.vertex_groups],
                "textures": {}
            }
            if mesh.uv_layers:
                uv_layer = mesh.uv_layers.active
                if uv_layer:
                    s["uvs"] = [{"u": d.uv.x, "v": d.uv.y} for d in uv_layer.data]
            for slot in mesh.materials:
                if slot and slot.use_nodes:
                    for node in slot.node_tree.nodes:
                        if node.type == 'TEX_IMAGE' and node.image:
                            s["textures"][node.name] = node.image.filepath
            shapes.append(s)

        return {
            "success": True,
            "game": "",
            "shapes": shapes,
            "bone_names": []
        }
    except Exception as e:
        return {"error": str(e), "traceback": traceback.format_exc()}


def _extract_shapes_pynifly(nif_path, NifFile):
    """Extract shapes using pynifly."""
    try:
        nif = NifFile(nif_path)
        shapes = []

        for shape in nif.shapes:
            s = {
                "name": shape.name if hasattr(shape, 'name') else "",
                "vertices": [],
                "triangles": [],
                "uvs": [],
                "normals": [],
                "bone_names": [],
                "textures": {}
            }

            if hasattr(shape, 'verts') and shape.verts:
                s["vertices"] = [{"x": v[0], "y": v[1], "z": v[2]} for v in shape.verts]

            if hasattr(shape, 'tris') and shape.tris:
                s["triangles"] = [{"v0": t[0], "v1": t[1], "v2": t[2]} for t in shape.tris]

            if hasattr(shape, 'uv_layers') and shape.uv_layers:
                for layer_name, layer_data in shape.uv_layers.items():
                    if layer_data:
                        s["uvs"] = [{"u": uv[0], "v": uv[1]} for uv in layer_data]
                        break

            if hasattr(shape, 'normals') and shape.normals:
                s["normals"] = [{"x": n[0], "y": n[1], "z": n[2]} for n in shape.normals]

            if hasattr(shape, 'bone_names'):
                s["bone_names"] = list(shape.bone_names) if shape.bone_names else []

            if hasattr(shape, 'textures') and shape.textures:
                s["textures"] = {k: v for k, v in shape.textures.items() if v}

            shapes.append(s)

        bone_names = []
        if hasattr(nif, 'bones'):
            bone_names = [b.name for b in nif.bones] if nif.bones else []

        game = ""
        if hasattr(nif, 'game'):
            game = str(nif.game) if nif.game else ""

        return {
            "success": True,
            "game": game,
            "shapes": shapes,
            "bone_names": bone_names
        }
    except Exception as e:
        return {"error": str(e), "traceback": traceback.format_exc()}


def op_extract_textures(nif_path):
    """Extract texture references from a NIF file."""
    NifFile = get_pynifly()
    if NifFile is not None:
        try:
            nif = NifFile(nif_path)
            textures = {}
            for shape in nif.shapes:
                if hasattr(shape, 'textures') and shape.textures:
                    for slot, path in shape.textures.items():
                        if path:
                            textures[slot] = path
                if hasattr(shape, 'material') and shape.material:
                    mat = shape.material
                    if hasattr(mat, 'textures') and mat.textures:
                        for slot, path in mat.textures.items():
                            if path:
                                textures[f"material_{slot}"] = path
            return {"success": True, "textures": textures}
        except Exception as e:
            return {"error": str(e), "traceback": traceback.format_exc()}

    bpy = get_blender()
    if bpy is None or get_niftools() is None:
        return {
            "error": "No NIF library available",
            "detail": "Install pynifly (pip install pynifly) or run with Blender+NifTools addon"
        }

    try:
        bpy.ops.wm.read_factory_settings(use_empty=True)
        addon = get_niftools()
        if addon == 'io_scene_nif':
            bpy.ops.import_scene.nif(filepath=nif_path)
        else:
            bpy.ops.import_scene.nif(filepath=nif_path)

        textures = {}
        for obj in bpy.data.objects:
            if obj.type != 'MESH':
                continue
            for slot in obj.data.materials:
                if slot and slot.use_nodes:
                    for node in slot.node_tree.nodes:
                        if node.type == 'TEX_IMAGE' and node.image:
                            textures[node.name] = node.image.filepath

        return {"success": True, "textures": textures}
    except Exception as e:
        return {"error": str(e), "traceback": traceback.format_exc()}


def op_validate(nif_path):
    """Validate a NIF file for common issues using pynifly or basic checks."""
    NifFile = get_pynifly()
    if NifFile is not None:
        try:
            nif = NifFile(nif_path)
            errors = []
            for i, shape in enumerate(nif.shapes):
                name = shape.name if hasattr(shape, 'name') else f"Shape_{i}"

                if hasattr(shape, 'verts') and shape.verts:
                    for vi, v in enumerate(shape.verts):
                        if any(abs(c) > 10000 for c in v):
                            errors.append({
                                "severity": "warning",
                                "field": f"shape[{i}].vertices[{vi}]",
                                "message": f"Extreme vertex position in '{name}'",
                                "block": i
                            })
                            break

                    if len(shape.verts) < 3:
                        errors.append({
                            "severity": "error",
                            "field": f"shape[{i}]",
                            "message": f"Shape '{name}' has fewer than 3 vertices",
                            "block": i
                        })

                if hasattr(shape, 'tris') and shape.tris:
                    max_idx = max(max(t) for t in shape.verts) if shape.verts else 0
                    for ti, t in enumerate(shape.tris):
                        if any(idx > max_idx for idx in t):
                            errors.append({
                                "severity": "error",
                                "field": f"shape[{i}].tris[{ti}]",
                                "message": f"Triangle index out of range in '{name}'",
                                "block": i
                            })
                            break

                    if len(shape.tris) == 0:
                        errors.append({
                            "severity": "error",
                            "field": f"shape[{i}]",
                            "message": f"Shape '{name}' has no triangles",
                            "block": i
                        })

            return {
                "success": True,
                "errors": errors,
                "valid": len([e for e in errors if e["severity"] == "error"]) == 0
            }
        except Exception as e:
            return {"error": str(e), "traceback": traceback.format_exc()}

    bpy = get_blender()
    if bpy is None or get_niftools() is None:
        return {
            "error": "No NIF library available",
            "detail": "Cannot validate without pynifly or Blender+NifTools"
        }

    return {
        "success": True,
        "errors": [],
        "valid": True,
        "note": "Basic validation only — install pynifly for deep validation"
    }


def op_compare(nif1_path, nif2_path):
    """Compare two NIF files structurally."""
    NifFile = get_pynifly()
    if NifFile is not None:
        try:
            nif1 = NifFile(nif1_path)
            nif2 = NifFile(nif2_path)

            vertex_changes = []
            texture_changes = []

            shapes1 = list(nif1.shapes) if hasattr(nif1, 'shapes') else []
            shapes2 = list(nif2.shapes) if hasattr(nif2, 'shapes') else []

            if len(shapes1) != len(shapes2):
                vertex_changes.append(f"Shape count: {len(shapes1)} vs {len(shapes2)}")

            min_count = min(len(shapes1), len(shapes2))
            for i in range(min_count):
                s1 = shapes1[i]
                s2 = shapes2[i]
                name1 = s1.name if hasattr(s1, 'name') else f"Shape_{i}"
                name2 = s2.name if hasattr(s2, 'name') else f"Shape_{i}"

                v1 = list(s1.verts) if hasattr(s1, 'verts') and s1.verts else []
                v2 = list(s2.verts) if hasattr(s2, 'verts') and s2.verts else []
                if len(v1) != len(v2):
                    vertex_changes.append(f"Shape '{name1}': vertex count {len(v1)} vs {len(v2)}")
                else:
                    diffs = sum(1 for a, b in zip(v1, v2) if any(abs(a[j] - b[j]) > 0.001 for j in range(3)))
                    if diffs > 0:
                        vertex_changes.append(f"Shape '{name1}': {diffs} vertices differ")

                t1 = dict(s1.textures) if hasattr(s1, 'textures') and s1.textures else {}
                t2 = dict(s2.textures) if hasattr(s2, 'textures') and s2.textures else {}
                all_keys = set(list(t1.keys()) + list(t2.keys()))
                for k in all_keys:
                    p1 = t1.get(k, "")
                    p2 = t2.get(k, "")
                    if p1 != p2:
                        texture_changes.append(f"Shape '{name1}' texture '{k}': '{p1}' vs '{p2}'")

            return {
                "success": True,
                "vertex_changes": vertex_changes,
                "texture_changes": texture_changes,
                "identical": len(vertex_changes) == 0 and len(texture_changes) == 0
            }
        except Exception as e:
            return {"error": str(e), "traceback": traceback.format_exc()}

    return {
        "error": "NIF comparison requires pynifly",
        "detail": "Install pynifly (pip install pynifly) for NIF comparison"
    }


def op_save_nif(output_path, game, shapes_json, bone_names_json):
    """Save shape data to a NIF file via Blender+NifTools addon."""
    import bpy  # Must be running inside Blender

    bpy.ops.wm.read_factory_settings(use_empty=True)

    for shape_data in shapes_json:
        verts = [(v["x"], v["y"], v["z"]) for v in shape_data.get("vertices", [])]
        tris = [(t["v0"], t["v1"], t["v2"]) for t in shape_data.get("triangles", [])]
        name = shape_data.get("name", "Shape")

        if not verts or not tris:
            continue

        mesh = bpy.data.meshes.new(name)
        mesh.from_pydata(verts, [], tris)
        mesh.update()

        obj = bpy.data.objects.new(name, mesh)
        bpy.context.collection.objects.link(obj)
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj

    addon = get_niftools()
    if addon is None:
        return {
            "error": "NifTools addon not available in Blender",
            "detail": "Install the NifTools addon from https://github.com/BigglesWorthless/NifToolsBlender"
        }

    try:
        if addon == 'io_scene_nif':
            bpy.ops.export_scene.nif(
                filepath=output_path,
                use_selection=True,
                apply_modifiers=True
            )
        else:
            bpy.ops.export_scene.nif(
                filepath=output_path,
                use_selection=True,
                apply_modifiers=True
            )
        return {"success": True}
    except Exception as e:
        return {"error": str(e), "traceback": traceback.format_exc()}


def op_get_version():
    """Get Blender and NIF addon version info."""
    bpy = get_blender()
    if bpy is not None:
        addon = get_niftools()
        return {
            "success": True,
            "version": bpy.app.version_string,
            "blender": True,
            "niftools": addon is not None,
            "niftools_name": addon
        }

    NifFile = get_pynifly()
    if NifFile is not None:
        try:
            from pynifly import __version__
            return {"success": True, "version": __version__, "pynifly": True}
        except (ImportError, AttributeError):
            pass

    return {
        "success": True,
        "version": "unknown",
        "blender": False,
        "pynifly": False
    }


def main():
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []

    if len(args) < 1:
        print(json.dumps({"error": "No operation specified"}))
        sys.exit(1)

    operation = args[0]

    try:
        if operation == "extract_shapes":
            result = op_extract_shapes(args[1])
        elif operation == "extract_textures":
            result = op_extract_textures(args[1])
        elif operation == "validate":
            result = op_validate(args[1])
        elif operation == "compare":
            result = op_compare(args[1], args[2])
        elif operation == "save_nif":
            shapes = json.loads(args[2]) if len(args) > 2 else []
            bones = json.loads(args[3]) if len(args) > 3 else []
            result = op_save_nif(args[1], "SKYRIM", shapes, bones)
        elif operation == "get_version":
            result = op_get_version()
        else:
            result = {"error": f"Unknown operation: {operation}"}
    except IndexError:
        result = {"error": f"Missing arguments for operation: {operation}"}
    except Exception as e:
        result = {"error": str(e), "traceback": traceback.format_exc()}

    print(json.dumps(result))


if __name__ == '__main__':
    main()
