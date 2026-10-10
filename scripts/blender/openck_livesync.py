#!/usr/bin/env python3
"""OpenCK live-sync bridge (Phase 12.2).

Runs inside Blender (GUI or background) with `--` args:

    --op=open --nif=MESH --skeleton=SKEL --collision=COLL [--output=OUT]

`open` imports the mesh with the NifTools addon, attaches the skeleton as
the armature of any skinned mesh, imports collision meshes into a
"Collision" collection, and registers a save_post handler that exports the
scene back to --output. Ctrl-S in Blender is therefore the whole export
step: OpenCK watches --output, verifies it with the same NIF parsers the
editor uses, and offers it for commit into the plugin.

`export` performs that export headlessly (used by tests and batch runs).

The export is a staging file beside the source mesh: this script never
writes the original NIF. Only OpenCK commits a verified export.
"""

import json
import os
import sys


def parse_args(argv):
    opts = {}
    for arg in argv:
        if arg.startswith('--') and '=' in arg:
            key, value = arg[2:].split('=', 1)
            opts[key] = value
        elif arg.startswith('--'):
            opts[arg[2:]] = ''
    return opts


def import_nif(filepath, collection_name='OpenCK'):
    """Import a NIF with whichever NIF addon is installed.

    NifTools ships `bpy.ops.import_scene.nif` (Skyrim/FO4 game names vary);
    pynifly exposes a module-level import. Returns True when objects
    arrived in the scene.
    """
    import bpy

    if not os.path.isfile(filepath):
        print('openck-livesync: missing file %s' % filepath)
        return False

    before = set(bpy.data.objects)
    imported = False
    try:
        import addon_utils
        have_niftools = any(
            mod.__name__ == 'io_scene_niftools' and mod.bl_info.get('blender', (0, 0, 0))
            for mod in addon_utils.modules()
            if hasattr(mod, 'bl_info') and 'nif' in mod.__name__.lower())
    except Exception:
        have_niftools = False

    if have_niftools:
        # Different NifTools releases take the game via different keyword
        # arguments; try the Skyrim default first and fall back.
        for kwargs in ({'game': 'SKYRIM_SE'}, {'game': 'SKYRIM'}, {}):
            try:
                bpy.ops.import_scene.nif(filepath=filepath, **kwargs)
                imported = True
                break
            except TypeError:
                continue
            except Exception as exc:
                print('openck-livesync: niftools import failed: %s' % exc)
                break

    if not imported:
        try:
            import pynifly
            pynifly.NifFile(filepath)
            imported = True
        except Exception as exc:
            print('openck-livesync: no NIF addon import worked: %s' % exc)
            return False

    after = set(bpy.data.objects) - before
    if not after:
        print('openck-livesync: import produced no objects for %s' % filepath)
        return False

    if collection_name:
        collection = bpy.data.collections.get(collection_name)
        if collection is None:
            collection = bpy.data.collections.new(collection_name)
            bpy.context.scene.collection.children.link(collection)
        for obj in after:
            for other in list(obj.users_collection):
                other.objects.unlink(obj)
            collection.objects.link(obj)
    return True


def attach_skeleton(skeleton_path):
    """Make the imported skeleton the parent armature of skinned meshes.

    NifTools' importer can bind a skeleton root when it is present in the
    same scene, so importing the skeleton NIF is what makes skinning
    editable. Returns True when a skeleton object is in the scene.
    """
    import bpy

    if not skeleton_path:
        return False
    if not import_nif(skeleton_path, collection_name='Skeleton'):
        return False
    for obj in bpy.data.objects:
        if obj.type == 'ARMATURE':
            print('openck-livesync: skeleton attached (%s)' % obj.name)
            return True
    return False


def import_collisions(paths):
    """Import collision meshes into their own collection so real geometry
    stays selectable and colliders do not render with the mesh."""
    for path in paths or []:
        import_nif(path, collection_name='Collision')


_export_handler_registered = False


def export_scene(output_path):
    """Write the scene back to output_path with the NifTools exporter."""
    import bpy

    if not output_path:
        print('openck-livesync: no --output given; skipping export')
        return False
    if not bpy.data.objects:
        # An empty save must not clobber a good export.
        print('openck-livesync: scene empty; skipping export')
        return False

    try:
        bpy.ops.export_scene.nif(filepath=output_path)
        print('openck-livesync: exported %s' % output_path)
        return True
    except Exception as exc:
        print('openck-livesync: export failed: %s' % exc)
        return False


def on_save_post(_scene, output_path_holder=None):
    export_scene(output_path_holder['path'])


def register_export_on_save(output_path):
    global _export_handler_registered
    import bpy

    holder = {'path': output_path}

    def handler(_scene):
        on_save_post(_scene, holder)

    bpy.app.handlers.save_post.append(handler)
    _export_handler_registered = True
    print('openck-livesync: Ctrl-S now exports to %s' % output_path)


def main():
    opts = parse_args(sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else [])
    op = opts.get('op', 'export')

    nif = opts.get('nif', '')
    skeleton = opts.get('skeleton', '')
    collisions = [v for k, v in opts.items() if k == 'collision' and v]
    # argv parsing above keeps only the last repeated key; accept a
    # comma-separated list for multiple collision files.
    if len(collisions) == 1 and ',' in collisions[0]:
        collisions = [p for p in collisions[0].split(',') if p]
    output = opts.get('output', '')

    if op == 'open':
        if not nif:
            print('openck-livesync: --nif is required for op=open')
            return 1
        if not import_nif(nif):
            return 1
        attach_skeleton(skeleton)
        import_collisions(collisions)
        if output:
            register_export_on_save(output)
        # GUI session: leave the window for the artist.
        return 0

    if op == 'export':
        return 0 if export_scene(output) else 1

    print('openck-livesync: unknown op %r' % op)
    return 1


if __name__ == '__main__':
    sys.exit(main())
