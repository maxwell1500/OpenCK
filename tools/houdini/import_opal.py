"""Instantiate an OpenCK OPAL placement list (.opl) inside Houdini.

Batch entry point for REMAINING.md section 3.8 (Houdini integration). OPAL
files are plain CSV (see OpalList in src/model/tools/opallist.hpp): a header
row plus one placement per data row. This script creates one null locator
per row under a container subnet so level artists can previs placements::

    hython.exe import_opal.py --opal town.opl --parent /obj/openck_town

Column handling (best effort, documented):
- The first column names the locator (sanitized to a valid node name).
- Columns named x/y/z (case-insensitive) drive the locator translation
  when all three parse as numbers; otherwise the locator stays at origin.
- Every column becomes a spare string parameter on the locator, so no
  placement data is lost even when the column set is game-specific.
"""

import argparse
import csv
import os
import re
import sys

try:
    import hou
except ImportError:
    sys.stderr.write(
        "import_opal.py must run inside Houdini (hython/houdini with the "
        "'hou' module available).\n"
    )
    sys.exit(2)

_NAME_CLEAN = re.compile(r"[^A-Za-z0-9_]+")


def parse_args(argv):
    parser = argparse.ArgumentParser(
        description="Instantiate an OpenCK OPAL placement list."
    )
    parser.add_argument("--opal", required=True,
                        help="Source .opl file (CSV with header row).")
    parser.add_argument("--parent", default="/obj/openck_opl",
                        help="Container path for the locators "
                             "(created when missing).")
    parser.add_argument("--hip", default="",
                        help="Optional .hip file to load first.")
    return parser.parse_args(argv)


def sanitize(name, fallback):
    cleaned = _NAME_CLEAN.sub("_", name).strip("_")
    return cleaned or fallback


def ensure_parent(path):
    node = hou.node(path)
    if node is not None:
        return node
    parent_path, _, name = path.rpartition("/")
    if not parent_path:
        parent_path = "/obj"
    parent = hou.node(parent_path)
    if parent is None:
        raise RuntimeError("parent context not found: %s" % parent_path)
    if parent.type().category().name() != "Object":
        raise RuntimeError("container must live in an Object context: %s"
                           % path)
    return parent.createNode("subnet", sanitize(name, "openck_opl"))


def main(argv):
    args = parse_args(argv)

    if not os.path.isfile(args.opal):
        sys.stderr.write("opl file not found: %s\n" % args.opal)
        return 1
    if args.hip:
        if not os.path.isfile(args.hip):
            sys.stderr.write("hip file not found: %s\n" % args.hip)
            return 1
        hou.hipFile.load(args.hip)

    with open(args.opal, "r", newline="", encoding="utf-8-sig") as handle:
        reader = csv.DictReader(handle)
        headers = reader.fieldnames or []
        rows = [row for row in reader if any((value or "").strip()
                                             for value in row.values())]
    if not headers:
        sys.stderr.write("opl file has no header row: %s\n" % args.opal)
        return 1

    lower = {name.lower(): name for name in headers}
    pos_cols = [lower.get(axis) for axis in ("x", "y", "z")]

    container = ensure_parent(args.parent)
    created = 0
    for index, row in enumerate(rows):
        label = (row.get(headers[0]) or "").strip()
        locator = container.createNode(
            "null", sanitize(label, "placement_%03d" % index))

        if all(pos_cols) and all((row.get(col) or "").strip()
                                 for col in pos_cols):
            try:
                locator.parmTuple("t").set(
                    (float(row[pos_cols[0]]),
                     float(row[pos_cols[1]]),
                     float(row[pos_cols[2]])))
            except ValueError:
                pass  # non-numeric position columns: stay at origin

        group = locator.parmTemplateGroup()
        for column in headers:
            parm = hou.StringParmTemplate(
                "opl_%s" % sanitize(column.lower(), "col"),
                column, 1, default_value=((row.get(column) or ""),))
            group.append(parm)
        locator.setParmTemplateGroup(group)
        created += 1

    print("instantiated %d placement(s) from %s under %s"
          % (created, args.opal, container.path()))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
