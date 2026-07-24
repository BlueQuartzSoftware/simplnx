# Write Vtk Rectilinear Grid

## Group (Subgroup)

I/O Filters

## Description

This filter writes the selected data to a **VTK legacy file**. A VTK legacy file is the older, human-readable file format used by the Visualization Toolkit (VTK); it can be opened directly by common scientific visualization tools such as **ParaView** and **VisIt**.

The file is written with a dataset type of `RECTILINEAR_GRID`. A rectilinear grid stores three explicit per-axis coordinate arrays (one list of X positions, one of Y positions, and one of Z positions). The grid lines are axis-aligned but the spacing along each axis may vary.

Although the input to this filter is an **Image Geometry** (which has a single uniform spacing along each axis), the output is written in the rectilinear-grid form: the filter expands the image's uniform spacing into the explicit per-axis coordinate arrays that the `RECTILINEAR_GRID` format requires. If the output only needs uniform spacing, the smaller [Write Vtk Structured Points](WriteVtkStructuredPointsFilter.md) format may be preferable.

The user selects which **Cell** **Data Array**s from the **Image Geometry** are written into the file. Each selected array becomes a cell-data field in the VTK file.

The *Write Binary File* parameter controls the on-disk encoding. When *false* (the default), the data is written as plain ASCII text, which is portable and easy to inspect but larger and slower to read. When *true*, the data is written in VTK's binary form, producing smaller files that load faster.

### Required Input Sources

- **Image Geometry** -- the geometry whose dimensions and spacing define the grid.
- **Cell Data Array(s) to write** -- any per-cell arrays on the **Image Geometry**. A common choice is a *FeatureIds* array produced by [Segment Features (Scalar)](ScalarSegmentFeaturesFilter.md).

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
