# Write OnScale Table File

## Group (Subgroup)

IO (Output)

## Description

This filter writes an **Image Geometry** or **Rectilinear Grid Geometry** to an OnScale/PZFLEX table (`.flxtbl`) file. The finite-element divisions equal the grid **Cell** dimensions. The selected *Cell Feature Ids* array supplies the `matr` values that describe the spatial distribution of features.

The output file contains these sections:

- `hedr` and `info` identify the table format.
- `xcrd`, `ycrd`, and `zcrd` contain node coordinates. An **Image Geometry** uses its origin and spacing. A **Rectilinear Grid Geometry** uses its bounds arrays without changes.
- `keypoints` contains the requested keypoint counts for the X, Y, and Z axes.
- `divisions` contains the grid **Cell** dimensions.
- `name` contains one phase name for each positive feature ID.
- `matr` contains the feature ID for each **Cell** in linear array order.

### Dimension Order

OnScale requires dimensions in non-increasing order: X must be greater than or equal to Y, and Y must be greater than or equal to Z. If an **Image Geometry** does not have this order, the filter determines the required 90-degree axis rotations and runs the Rotate Sample Reference Frame **Filter** internally. The exported dimensions, spacing, origin, and feature IDs come from the reordered copy. The input **Data Structure** does not change.

A **Rectilinear Grid Geometry** with dimensions that require reordering is not supported. The filter rejects this geometry during preflight because a 90-degree image rotation cannot preserve its non-uniform bounds arrays.

### Phase Names

The phase-name array uses feature IDs as tuple indices. Tuple *0* is not written. If the maximum positive feature ID is greater than the available phase-name tuples, the filter writes `Phase_<id>` for each missing name. An empty phase-name array causes all positive IDs to use this generated form.

This behavior and file layout mirror the DREAM.3D 6.6 `ExportOnScaleTableFile` filter.

### Required Input Sources

- **Image Geometry** or **Rectilinear Grid Geometry** — the grid to export.
- **Cell Feature Ids** — a scalar signed or unsigned integer **Data Array** with one tuple for each grid **Cell**.
- **Phase Names** — a `StringArray` that contains the name for each positive feature ID.

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
