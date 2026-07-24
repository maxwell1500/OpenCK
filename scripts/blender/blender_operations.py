#!/usr/bin/env python3
"""
Blender Launcher Operations for OpenCK
Runs inside Blender (--background --python) to detect addons and perform operations.

Operations:
  detect_addons      - Detect PyNifly/NifTools addon installation and enable them
  export_nif         - Export NIF from Blender scene via NifTools addon
  import_nif         - Import NIF into Blender via NifTools addon
  generate_preview   - Generate preview images for NIF
  batch_export       - Batch export models from a directory
  validate_nif       - Validate NIF file via Blender import

Output: JSON to stdout

Licensed under GPL-3.0 (compatible with OpenCK license)
"""

import bpy
import sys
import os
import json
import traceback


NIFTOOLS_ADDON_MODULE = "io_scene_nif"
PYNIFLY_ADDON_MODULE = "pynifly"


def enable_addon(module_name):
    """Try to enable a Blender addon by module name. Returns True if enabled."""
    try:
        bpy.ops.preferences.addon_enable(module=module_name)
        return True
    except (RuntimeError, AttributeError):
        return False


def is_addon_enabled(module_name):
    """Check if a Blender addon is currently enabled."""
    try:
        return module_name in bpy.context.preferences.addons
    except (RuntimeError, AttributeError):
        return False


def get_addon_path(module_name):
    """Get the filesystem path of an installed addon, or empty string."""
    try:
        addon = bpy.context.preferences.addons.get(module_name)
        if addon is not None:
            return getattr(addon, 'module', '') and ''
        return ''
    except (RuntimeError, AttributeError):
        return ''


def op_detect_addons():
    """Detect installed NIF addons in Blender and try to enable them."""
    result = {
        "pynifly": {"installed": False, "enabled": False, "path": ""},
        "niftools": {"installed": False, "enabled": False, "path": ""},
        "blender_version": ".".join(str(v) for v in bpy.app.version)
    }

    for mod_name in (NIFTOOLS_ADDON_MODULE, PYNIFLY_ADDON_MODULE):
        try:
            if mod_name in bpy.context.preferences.addons:
                addon = bpy.context.preferences.addons[mod_name]
                if mod_name == NIFTOOLS_ADDON_MODULE:
                    result["niftools"]["installed"] = True
                    result["niftools"]["enabled"] = True
                else:
                    result["pynifly"]["installed"] = True
                    result["pynifly"]["enabled"] = True
            else:
                if enable_addon(mod_name):
                    if mod_name == NIFTOOLS_ADDON_MODULE:
                        result["niftools"]["installed"] = True
                        result["niftools"]["enabled"] = True
                    else:
                        result["pynifly"]["installed"] = True
                        result["pynifly"]["enabled"] = True
        except (RuntimeError, AttributeError):
            pass

    try:
        scripts_dir = bpy.utils.script_paths()
        for sdir in scripts_dir:
            addons_dir = os.path.join(sdir, "addons")
            if os.path.isdir(addons_dir):
                for mod_name in (NIFTOOLS_ADDON_MODULE, PYNIFLY_ADDON_MODULE):
                    if os.path.isdir(os.path.join(addons_dir, mod_name)):
                        if mod_name == NIFTOOLS_ADDON_MODULE:
                            result["niftools"]["installed"] = True
                            if not result["niftools"]["path"]:
                                result["niftools"]["path"] = os.path.join(addons_dir, mod_name)
                        else:
                            result["pynifly"]["installed"] = True
                            if not result["pynifly"]["path"]:
                                result["pynifly"]["path"] = os.path.join(addons_dir, mod_name)
    except (RuntimeError, AttributeError):
        pass

    return result


def op_export_nif(blend_path, nif_path, game="SKYRIM"):
    """Export a .blend file to a .nif file via NifTools addon."""
    try:
        if blend_path and os.path.exists(blend_path):
            bpy.ops.wm.open_mainfile(filepath=blend_path)

        bpy.ops.object.select_all(action='SELECT')
        if bpy.context.view_layer.objects.active is None:
            return {"success": False, "error": "No active object to export"}

        bpy.context.view_layer.objects.active = bpy.context.view_layer.objects[0]

        if NIFTOOLS_ADDON_MODULE not in bpy.context.preferences.addons:
            if not enable_addon(NIFTOOLS_ADDON_MODULE):
                return {
                    "success": False,
                    "error": "NifTools addon not available. Install it from https://github.com/BigglesWorthless/NifToolsBlender"
                }

        if hasattr(bpy.ops.export_scene, 'nif'):
            bpy.ops.export_scene.nif(
                filepath=nif_path,
                use_selection=True,
                apply_modifiers=True
            )
        else:
            return {
                "success": False,
                "error": "export_scene.nif operator not found after enabling NifTools addon"
            }

        if not os.path.exists(nif_path):
            return {
                "success": False,
                "error": "Export completed but output file not found",
                "output": nif_path
            }

        return {"success": True, "output": nif_path}
    except Exception as e:
        return {"success": False, "error": str(e), "traceback": traceback.format_exc()}


def op_import_nif(nif_path, blend_path=""):
    """Import a .nif file into Blender via NifTools addon, optionally save as .blend."""
    try:
        if not os.path.exists(nif_path):
            return {"success": False, "error": f"NIF file not found: {nif_path}"}

        bpy.ops.wm.read_factory_settings(use_empty=True)

        if NIFTOOLS_ADDON_MODULE not in bpy.context.preferences.addons:
            if not enable_addon(NIFTOOLS_ADDON_MODULE):
                return {
                    "success": False,
                    "error": "NifTools addon not available. Install it from https://github.com/BigglesWorthless/NifToolsBlender"
                }

        if hasattr(bpy.ops.import_scene, 'nif'):
            bpy.ops.import_scene.nif(filepath=nif_path)
        else:
            return {
                "success": False,
                "error": "import_scene.nif operator not found after enabling NifTools addon"
            }

        if blend_path:
            bpy.ops.wm.save_as_mainfile(filepath=blend_path)

        imported = [obj.name for obj in bpy.data.objects if obj.type == 'MESH']
        return {
            "success": True,
            "output": blend_path or nif_path,
            "imported_objects": imported
        }
    except Exception as e:
        return {"success": False, "error": str(e), "traceback": traceback.format_exc()}


def op_generate_preview(nif_path, width, height, angles):
    """Render preview images of a NIF file from multiple angles."""
    try:
        if not os.path.exists(nif_path):
            return {"success": False, "error": f"NIF file not found: {nif_path}"}

        bpy.ops.wm.read_factory_settings(use_empty=True)

        if NIFTOOLS_ADDON_MODULE not in bpy.context.preferences.addons:
            if not enable_addon(NIFTOOLS_ADDON_MODULE):
                return {
                    "success": False,
                    "error": "NifTools addon not available for NIF import"
                }

        bpy.ops.import_scene.nif(filepath=nif_path)

        mesh_objects = [obj for obj in bpy.data.objects if obj.type == 'MESH']
        if not mesh_objects:
            return {"success": False, "error": "No mesh objects found in NIF"}

        scene = bpy.context.scene
        scene.render.resolution_x = int(width)
        scene.render.resolution_y = int(height)
        scene.render.engine = 'BLENDER_EEVEE'

        previews = []
        for angle in range(int(angles)):
            rotation = (360.0 / int(angles)) * angle
            for obj in mesh_objects:
                obj.rotation_euler = (0, 0, 0)
                obj.rotation_euler.z = rotation * 0.0174533
            bpy.context.view_layer.update()
            output_path = nif_path + f".preview_{angle}.png"
            bpy.ops.render.render(write_still=True)
            if hasattr(bpy.data, 'images') and bpy.data.images:
                img = list(bpy.data.images)[-1]
                img.filepath_raw = output_path
                img.file_format = 'PNG'
                img.save_render(output_path)
            previews.append(output_path)

        return {"success": True, "previews": previews}
    except Exception as e:
        return {"success": False, "error": str(e), "traceback": traceback.format_exc()}


def op_batch_export(plugin_path, output_dir):
    """Batch export all NIF files referenced by a plugin."""
    try:
        if not os.path.isdir(plugin_path):
            return {"success": False, "error": f"Plugin path not found: {plugin_path}"}

        os.makedirs(output_dir, exist_ok=True)

        nif_files = []
        for root, dirs, files in os.walk(plugin_path):
            for f in files:
                if f.lower().endswith('.nif'):
                    nif_files.append(os.path.join(root, f))

        if not nif_files:
            return {"success": True, "count": 0, "note": "No NIF files found in plugin directory"}

        if NIFTOOLS_ADDON_MODULE not in bpy.context.preferences.addons:
            if not enable_addon(NIFTOOLS_ADDON_MODULE):
                return {
                    "success": False,
                    "error": "NifTools addon not available for batch export"
                }

        exported = []
        for nif_path in nif_files:
            try:
                bpy.ops.wm.read_factory_settings(use_empty=True)
                bpy.ops.import_scene.nif(filepath=nif_path)
                relative = os.path.relpath(nif_path, plugin_path)
                out_path = os.path.join(output_dir, relative).replace('.nif', '.blend')
                os.makedirs(os.path.dirname(out_path), exist_ok=True)
                bpy.ops.wm.save_as_mainfile(filepath=out_path)
                exported.append(out_path)
            except Exception as e:
                sys.stderr.write(f"Failed to export {nif_path}: {e}\n")

        return {"success": True, "count": len(exported), "exported": exported}
    except Exception as e:
        return {"success": False, "error": str(e), "traceback": traceback.format_exc()}


def op_validate_nif(nif_path):
    """Validate a NIF file by attempting to import it via NifTools."""
    try:
        if not os.path.exists(nif_path):
            return {"success": False, "error": f"NIF file not found: {nif_path}"}

        bpy.ops.wm.read_factory_settings(use_empty=True)

        if NIFTOOLS_ADDON_MODULE not in bpy.context.preferences.addons:
            if not enable_addon(NIFTOOLS_ADDON_MODULE):
                return {
                    "success": False,
                    "error": "NifTools addon not available for validation"
                }

        try:
            bpy.ops.import_scene.nif(filepath=nif_path)
            return {
                "success": True,
                "valid": True,
                "errors": [],
                "object_count": len([o for o in bpy.data.objects if o.type == 'MESH'])
            }
        except Exception as import_err:
            return {
                "success": True,
                "valid": False,
                "errors": [str(import_err)],
                "object_count": 0
            }
    except Exception as e:
        return {"success": False, "error": str(e), "traceback": traceback.format_exc()}


def main():
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []

    if len(args) < 1:
        print(json.dumps({"error": "No operation specified"}))
        sys.exit(1)

    operation = args[0]

    try:
        if operation == "detect_addons":
            result = op_detect_addons()
        elif operation == "export_nif":
            blend = args[1] if len(args) > 1 else ""
            nif = args[2] if len(args) > 2 else ""
            game = args[3] if len(args) > 3 else "SKYRIM"
            result = op_export_nif(blend, nif, game)
        elif operation == "import_nif":
            nif = args[1] if len(args) > 1 else ""
            blend = args[2] if len(args) > 2 else ""
            result = op_import_nif(nif, blend)
        elif operation == "generate_preview":
            nif = args[1] if len(args) > 1 else ""
            w = args[2] if len(args) > 2 else "512"
            h = args[3] if len(args) > 3 else "512"
            a = args[4] if len(args) > 4 else "8"
            result = op_generate_preview(nif, w, h, a)
        elif operation == "batch_export":
            plugin = args[1] if len(args) > 1 else ""
            outdir = args[2] if len(args) > 2 else ""
            result = op_batch_export(plugin, outdir)
        elif operation == "validate_nif":
            nif = args[1] if len(args) > 1 else ""
            result = op_validate_nif(nif)
        else:
            result = {"error": f"Unknown operation: {operation}"}
    except IndexError:
        result = {"error": f"Missing arguments for operation: {operation}"}
    except Exception as e:
        result = {"error": str(e), "traceback": traceback.format_exc()}

    print(json.dumps(result))


if __name__ == '__main__':
    main()
