# NX Info (nxinfo)

`nxinfo` is a command line tool that reports information about files used by
DREAM3D-NX and simplnx. It is built and installed alongside `nxrunner`.

Its output is designed to be read by people and by automated tools, including
LLM coding agents that need to learn what a `.dream3d` file contains before
deciding what to do with it.

## Commands

### Dump DataStructure

```bash
nxinfo --dump-data-structure <file.dream3d> [--output-file <path>] [--format JSON|TEXT|DOT]
nxinfo -d <file.dream3d> [-o <path>] [-f JSON|TEXT|DOT]
```

Prints the DataStructure hierarchy of the given `.dream3d` file. Both NX and
legacy DREAM.3D 6.x files are supported.

The file is opened in preflight mode. NX-format numeric array payloads and
ordinary legacy attribute-array payloads are not loaded. Some legacy DREAM.3D
geometry storage, NeighborList, and Statistics payloads are fully imported.
Legacy StringArrays do not load their strings, but they allocate one empty
string placeholder per tuple. Time and memory use can therefore scale with
those imported legacy payloads and with the number of legacy string tuples.

Output goes to stdout unless `--output-file` is given. Errors go to stderr.
Nothing else is written to stdout, so the output can be piped to `jq` or
redirected to a file.

When `--output-file` is used, its parent directory must already exist. nxinfo
writes a temporary sibling and replaces the destination only after the complete
dump is flushed successfully. If writing or replacement fails, an existing
destination file is preserved.

The output path must not identify the input `.dream3d` file. This check also
rejects alternate paths, symbolic links and hard links that resolve to the
same file.

`--format` is case-insensitive and defaults to `JSON`.
Each of `--dump-data-structure`, `--output-file`, and `--format` may be
specified only once; short and long spellings count as the same option. An
option always consumes its required next argument literally, so file paths
beginning with `-` do not require a separate `--` marker.

`--help` and `--version` must each be used by themselves.

| Format | Description |
|--------|-------------|
| `JSON` | Structured hierarchy with per-object metadata. See schema below. |
| `TEXT` | Indented names, one per line and prefixed with `\|--`; backslashes and control characters are escaped. |
| `DOT`  | GraphViz digraph with unique object-ID nodes and escaped name labels. Render with `dot -Tpng`. |

Example:

```bash
nxinfo -d /data/SmallIN100.dream3d -o /tmp/SmallIN100.json
nxinfo -d /data/SmallIN100.dream3d -f DOT | dot -Tpng -o /tmp/SmallIN100.png
```

### Help

```bash
nxinfo --help | -h
```

### Version

```bash
nxinfo --version | -v
```

## Exit codes

| Code | Meaning |
|------|---------|
| 0    | Success |
| 1    | Unexpected exception caught at the process boundary |
| -100 | No arguments provided |
| -101 | Failed parsing arguments (unknown or repeated option, missing value, unknown format, no command) |
| -102 | Input file missing or not a regular file |
| -103 | Failed reading the DataStructure |
| -104 | Failed opening or writing the output destination |

The table shows the signed values returned by nxinfo's command logic. POSIX
shells expose only the low eight bits of a process exit status, so they report
`-100` through `-104` as `156` through `152`, respectively. Other process APIs
can represent negative exit values differently.

## JSON schema (schema_version 1)

Top level:

```json
{ "objects": [ <node>, ... ], "schema_version": 1 }
```

Objects and children are ordered alphabetically by name for deterministic
machine-readable output.

Every node:

| Field      | Type    | Description |
|------------|---------|-------------|
| `name`     | string  | Object name |
| `path`     | string  | Full path from the root, `/` separated |
| `id`       | integer | DataObject id in this session (not stable across reads) |
| `type`     | string  | Class name, e.g. `ImageGeom`, `AttributeMatrix`, `DataArray<int32>` |
| `children` | array   | Child nodes; empty for leaves |

AttributeMatrix adds `tuple_shape`.

DataArray adds `data_type`, `tuple_shape`, `component_shape`, `num_tuples`,
`num_components`, and `store_type` (`InMemory`, `OutOfCore`, `Empty`,
`EmptyOutOfCore`; a preflight read reports `Empty`).

StringArray adds `data_type` (`"string"`) and `num_tuples`.

NeighborList adds `data_type` and `num_tuples`.

Geometries add a `geometry` object:

| Field                 | Present on | Description |
|-----------------------|------------|-------------|
| `geometry_type`       | all        | `Image`, `RectGrid`, `Vertex`, `Edge`, `Triangle`, `Quad`, `Tetrahedral`, `Hexahedral` |
| `unit_dimensionality` | all        | 0, 1, 2 or 3 |
| `length_units`        | all        | e.g. `Micrometer` |
| `num_cells`           | all        | Number of cells / elements |
| `dimensions`          | Image, RectGrid | `[x, y, z]` voxel counts |
| `cell_data_path`      | Image, RectGrid | Path of the cell AttributeMatrix, if assigned |
| `origin`              | Image, readable RectGrid | `[x, y, z]`. A preflight RectGrid omits this field because its origin is derived from bounds values that are not read. |
| `spacing`             | Image      | `[x, y, z]` |
| `num_vertices`, `vertex_data_path` | node geometries | Vertex count and assigned vertex AttributeMatrix path |
| `num_edges`, `edge_data_path`     | Edge and higher | Edge count and assigned edge AttributeMatrix path |
| `num_faces`, `face_data_path`     | Triangle, Quad and higher | Face count and assigned face AttributeMatrix path |
| `num_polyhedra`, `polyhedron_data_path` | Tetrahedral, Hexahedral | Polyhedron count and assigned polyhedron AttributeMatrix path |

Any `*_data_path` key is omitted when the AttributeMatrix has not been assigned.

Example from the round-trip test file (abridged):

```json
{
  "objects": [
    {
      "children": [
        {
          "children": [
            {
              "children": [],
              "component_shape": [
                1
              ],
              "data_type": "boolean",
              "id": 7,
              "name": "Conditional [bool]",
              "num_components": 1,
              "num_tuples": 192000,
              "path": "Image Geometry/CellData/Conditional [bool]",
              "store_type": "Empty",
              "tuple_shape": [
                80,
                60,
                40
              ],
              "type": "DataArray<bool>"
            }
          ],
          "id": 6,
          "name": "CellData",
          "path": "Image Geometry/CellData",
          "tuple_shape": [
            80,
            60,
            40
          ],
          "type": "AttributeMatrix"
        }
      ],
      "geometry": {
        "cell_data_path": "Image Geometry/CellData",
        "dimensions": [
          40,
          60,
          80
        ],
        "geometry_type": "Image",
        "length_units": "Meter",
        "num_cells": 192000,
        "origin": [
          0.0,
          20.0,
          66.0
        ],
        "spacing": [
          0.25,
          0.550000011920929,
          1.8600000143051147
        ],
        "unit_dimensionality": 3
      },
      "id": 1,
      "name": "Image Geometry",
      "path": "Image Geometry",
      "type": "ImageGeom"
    }
  ],
  "schema_version": 1
}
```
