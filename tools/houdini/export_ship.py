"""Export OpenCK ship geometry from Houdini to FBX.

Batch entry point for REMAINING.md section 3.8 (Houdini integration). Run
headless through hython, e.g. via the OpenCK Houdini bridge::

    hython.exe export_ship.py --out ship.fbx
    hython.exe export_ship.py --out ship.fbx --node /obj/ship_geo

Without --node, every SOP with its display flag set under /obj is exported.
The scene is NOT saved; pass --hip to load a .hip file first.
"""

import argparse
import os
import sys

try:
    import hou
except ImportError:
    sys.stderr.write(
        "export_ship.py must run inside Houdini (hython/houdini with the "
        "'hou' module available).\n"
    )
    sys.exit(2)


def parse_args(argv):
    parser = argparse.ArgumentParser(
        description="Export ship geometry to FBX."
    )
    parser.add_argument("--out", required=True,
                        help="Destination .fbx file.")
    parser.add_argument("--node", default="",
                        help="SOP/Object path to export (default: all "
                             "display-flagged SOPs under /obj).")
    parser.add_argument("--hip", default="",
                        help="Optional .hip file to load before exporting.")
    return parser.parse_args(argv)


def export_targets(explicit_node):
    """Resolve the SOP paths to hand to the FBX ROP."""
    if explicit_node:
        node = hou.node(explicit_node)
        if node is None:
            raise RuntimeError("node not found: %s" % explicit_node)
        return [node.path()]

    targets = []
    obj = hou.node("/obj")
    if obj is None:
        return targets
    for child in obj.children():
        if child.type().category().name() != "Sop":
            continue
        if child.isDisplayFlagSet():
            targets.append(child.path())
    return targets


def export_fbx(sop_paths, out_path):
    """Export the given SOPs through a throwaway filmboxfbx ROP."""
    out = hou.node("/out")
    if out is None:
        raise RuntimeError("no /out context in this scene")
    rop = out.createNode("filmboxfbx", "openck_ship_export")
    try:
        rop.parm("sopoutput").set(out_path)
        rop.parm("startnode").set(" ".join(sop_paths))
        rop.parm("execute").pressButton()
    finally:
        rop.destroy()


def main(argv):
    args = parse_args(argv)

    if args.hip:
        if not os.path.isfile(args.hip):
            sys.stderr.write("hip file not found: %s\n" % args.hip)
            return 1
        hou.hipFile.load(args.hip)

    targets = export_targets(args.node)
    if not targets:
        sys.stderr.write("nothing to export (no --node and no "
                         "display-flagged SOPs under /obj)\n")
        return 1

    out_dir = os.path.dirname(os.path.abspath(args.out))
    if out_dir and not os.path.isdir(out_dir):
        os.makedirs(out_dir)

    export_fbx(targets, os.path.abspath(args.out))
    print("exported %d node(s) to %s" % (len(targets), args.out))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
