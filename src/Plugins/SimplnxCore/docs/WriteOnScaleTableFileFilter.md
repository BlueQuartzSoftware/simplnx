# Write OnScale Table File

## Group (Subgroup)

IO (Output)

## Description

This filter writes an **Image Geometry** or **Rectilinear Grid Geometry** to an OnScale/PZFLEX table (`.flxtbl`) file. The finite-element divisions equal the grid **Cell** dimensions. The selected *Cell Feature Ids* array supplies the `matr` values that describe the spatial distribution of materials.

The output file contains these sections:

- `hedr` and `info` identify the table format.
- `xcrd`, `ycrd`, and `zcrd` contain node coordinates in the geometry length units. An **Image Geometry** uses its origin and spacing. A **Rectilinear Grid Geometry** uses its bounds arrays without changes.
- `keypoints` contains the requested keypoint counts for the X, Y, and Z axes. The filter passes these values through verbatim for PZFLEX. The default is `2 2 2`.
- `divisions` contains the grid **Cell** dimensions.
- `name` contains one phase name for each positive feature ID.
- `matr` contains the material index for each **Cell** in linear array order. Each positive value is an index into the `name` list. A value of `0` means that the cell has no material.

## Example Output

The following file contains the 24 cells from an **Image Geometry** with dimensions `{4, 3, 2}`:

```text
hedr 0
info 1
xcrd 5
0.00000000E+00 1.00000005E-03 2.00000009E-03 3.00000003E-03 4.00000019E-03
ycrd 4
0.00000000E+00 2.00000009E-03 4.00000019E-03 6.00000005E-03
zcrd 3
0.00000000E+00 3.00000003E-03 6.00000005E-03
keypoints
2 3 4
divisions
4 3 2
name 3
pzt4t11 pzt4t12 Phase_3
matr 24
1 2 3 1 2 3 1 2 3 1 2 3 1 2 3 1 2 3 1 2 3 1 2 3
```

The `name` data line in the generated file ends with one ASCII space after `Phase_3`. The code block omits this invisible trailing byte so that repository whitespace checks remain clean.

### Dimension Order

The legacy DREAM.3D exporter required dimensions in non-increasing order (X >= Y >= Z), and this filter keeps that rule. If an **Image Geometry** does not have this order, the filter determines the required 90-degree axis rotations and runs the Rotate Sample Reference Frame **Filter** internally. The exported dimensions, spacing, origin, and feature IDs come from the reordered copy. The input **Data Structure** does not change.

The filter validates two-axis reorders after rotation and reports an error if the rotated dimensions are not the sorted input dimensions. This problem occurs with anisotropic spacing because of a limitation in the current Rotate Sample Reference Frame implementation; resample to isotropic spacing or reorder the axes before this filter.

A **Rectilinear Grid Geometry** with dimensions that require reordering is not supported. The filter rejects this geometry during preflight because a 90-degree image rotation cannot preserve its non-uniform bounds arrays.

The DREAM.3D 6.6 reorder path always failed because it passed a 3x3 table to a filter that required a 4x4 table. This filter implements the intended reorder with Rotate Sample Reference Frame.

### Memory

During a reorder, peak memory for feature IDs is three times the input feature-ID bytes while Rotate Sample Reference Frame runs. This total contains the input array, the scratch copy, and the rotated output. Memory is two times the input feature-ID bytes afterwards and stays at that level until the algorithm returns. An export that does not require reordering makes no extra feature-ID copy.

### Phase Names

The phase-name array uses feature IDs as tuple indices. Tuple *0* is not written. If the maximum positive feature ID is greater than the available phase-name tuples, the filter writes `Phase_<id>` for each missing name. An empty phase-name array causes all positive IDs to use this generated form and produces a preflight warning.

Negative feature IDs are not valid material indices. The filter reports an error before it writes the output file if it finds any negative IDs. If the array has no positive feature IDs, the filter writes the legacy empty `name` section and reports a warning.

This behavior and file layout mirror the DREAM.3D 6.6 `ExportOnScaleTableFile` filter.

### Required Input Sources

- **Image Geometry** or **Rectilinear Grid Geometry**, the grid to export.
- **Cell Feature Ids**, a scalar signed or unsigned integer **Data Array** with one tuple for each grid **Cell**.
- **Phase Names**, a `StringArray` that contains the name for each positive feature ID. [Create Ensemble Info](../../OrientationAnalysis/docs/CreateEnsembleInfoFilter.md) can produce this array.

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
