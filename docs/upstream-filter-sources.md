# Upstream Filter Sources

Lookup table mapping common input arrays to the filter(s) that produce them. Used when writing the **Required Input Sources** section of a filter's documentation (see the `vv-filter-documentation` skill for the convention).

This file is intended to be grown over time. As more filters are documented or new filters are added, append rows. The longer-term goal is to derive a dependency graph from this data so that pipeline order constraints can be visualized.

## How To Use

When documenting a filter that requires array X, look up X in the table below and link to the producing filter from your Required Input Sources section using MyST link syntax:

```markdown
- **X** -- produced by [Producer Human Name](ProducerFilter.md).
```

For arrays produced by EBSD readers (Cell Quaternions, Cell Phases, ensemble Crystal Structures), name the typical readers explicitly rather than picking one:

```markdown
- **Cell Quaternions** -- typically read from EBSD data via [Read H5EBSD](ReadH5EbsdFilter.md), [Read CTF Data](ReadCtfDataFilter.md), or [Read ANG Data](ReadAngDataFilter.md); can also be produced from Euler angles by [Convert Orientations](ConvertOrientationsFilter.md).
```

## Conventions for Growing This Table

- **One row per array, multiple producers comma-separated** in the Producer column when the array can come from several filters.
- **Plugin column** lists where the *producer* lives, not where the consuming filter lives. This drives the relative-path direction of MyST links.
- **Cross-plugin links** must use `../OtherPlugin/FilterFile.md`. Within-plugin links use `FilterFile.md` only.
- **Filter names match `humanName()`** in the .cpp, not the file name. The link target file is always `<PascalCaseFilterName>Filter.md`.
- **Notes column** (optional) for caveats: ensemble-level vs feature-level vs cell-level, optional inputs, mode dependencies (e.g., "only when *Find Coherence* is enabled").

## Lookup Table

| Input Array | Producer (humanName) | Plugin | Notes |
|---|---|---|---|
| Cell Quaternions | Read H5EBSD / Read CTF Data / Read ANG Data / Convert Orientations | OrientationAnalysis | From EBSD readers, or computed from Euler angles |
| Cell Eulers | Same EBSD readers | OrientationAnalysis | Bunge convention Z-X-Z |
| Cell Phases | Same EBSD readers | OrientationAnalysis | Read alongside quaternions |
| Crystal Structures | Same EBSD readers / Create Ensemble Info | OrientationAnalysis | Ensemble-level array |
| Cell Feature Ids | Segment Features (Misorientation) | OrientationAnalysis | Misorientation-based grain segmentation |
| Cell Feature Ids | Segment Features (C-Axis Misalignment) | OrientationAnalysis | Hexagonal-only |
| Cell Feature Ids | Segment Features (Scalar) | SimplnxCore | Scalar-based segmentation |
| Cell Parent Ids | Merge Twins | OrientationAnalysis | Cell-level grouping into parent grains |
| Feature Parent Ids | Merge Twins | OrientationAnalysis | Feature-level mapping |
| Feature Phases | Compute Feature Phases | SimplnxCore | Feature-level (one phase per feature) |
| Feature Phases (Binary) | Compute Feature Phases (Binary) | SimplnxCore | Two-phase classification |
| Average Quaternions | Compute Average Orientations | OrientationAnalysis | Per-feature average orientation |
| Average Euler Angles | Compute Average Orientations | OrientationAnalysis | Bunge convention Z-X-Z |
| Average C-Axes | Compute Average C-Axis Orientations | OrientationAnalysis | Hexagonal-only |
| Feature Centroids | Compute Feature Centroids | SimplnxCore | Cell-of-mass per feature |
| Feature Equivalent Diameters | Compute Feature Sizes | SimplnxCore | Diameter of equal-volume sphere |
| Feature Num. Cells (NumElements) | Compute Feature Sizes | SimplnxCore | Cell count per feature |
| Feature Volumes | Compute Feature Sizes | SimplnxCore | Physical volume |
| Surface Features | Compute Surface Features | SimplnxCore | Boolean per feature |
| Surface Features | Compute Feature Neighbors | SimplnxCore | Optional output (enable *Store Surface Features Array*) |
| Biased Features | Compute Biased Features (Bounding Box) | SimplnxCore | Boolean per feature |
| Number of Neighbors | Compute Feature Neighbors | SimplnxCore | Count per feature |
| Contiguous Neighbor List | Compute Feature Neighbors | SimplnxCore | NeighborList of contiguous Feature Ids |
| Shared Surface Area List | Compute Feature Neighbors | SimplnxCore | NeighborList in cell-face-count units |
| Boundary Cells | Compute Feature Neighbors | SimplnxCore | Optional output (enable *Store Boundary Cells Array*) |
| Neighborhoods Neighbor List | Compute Feature Neighborhoods | SimplnxCore | Sphere-radius-based neighbor list |
| Mask | Multi-Threshold Objects | SimplnxCore | Boolean per cell |
| Boundary Euclidean Distances | Compute Euclidean Distance Map | SimplnxCore | Cell-level distance to nearest boundary |
| Triangle Geometry | Quick Surface Mesh | SimplnxCore | Surface mesh from Feature Ids |
| Face Labels | Quick Surface Mesh | SimplnxCore | Per-face Feature ID pair (2-component) |
| Face Normals | Compute Triangle Normals | SimplnxCore | 3-component, on triangle face data |
| Face Areas | Compute Triangle Areas | SimplnxCore | Single-component, on triangle face data |
| Node Types | Quick Surface Mesh | SimplnxCore | Per-vertex node classification |
| Cell Shifts Array (alignment) | Align Sections (Feature Centroid) | SimplnxCore | Output of any Align Sections * filter |
| Cell Shifts Array (alignment) | Align Sections (Misorientation) | OrientationAnalysis | |
| Cell Shifts Array (alignment) | Align Sections (Mutual Information) | OrientationAnalysis | |
| Confidence Index / Image Quality | Read H5EBSD / Read CTF Data / Read ANG Data | OrientationAnalysis | Per-cell scalar quality metric |

## Future: Graph Representation

The lookup table is naturally a directed dependency graph: each row is an edge from a producer filter to the input-array node. A separate downstream effort may convert this into a graph (DOT, Mermaid, or networked JSON) to support pipeline-order checking, "what would I need to run before X?" queries, and visual dependency maps. Keep that future use in mind when adding rows -- be precise about producer humanName so the data is machine-parseable.
