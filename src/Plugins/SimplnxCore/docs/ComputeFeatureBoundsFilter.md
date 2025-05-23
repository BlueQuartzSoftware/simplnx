# Compute Feature Bounding Boxes

## Group (Subgroup)

Geometry

## Description

**Warning**: _Potential Runtime Error_ - It is expected that the max feature id plus one (`max_feature_id_value` + 1) is equal to or less than the number of tuples in the supplied feature Attribute Matrix. This cannot be checked in preflight and will terminate the pipeline if encountered.

This filter calculates the bounding boxes for each feature given Feature Ids and Geometry (refer to table below for supported geometry types and their corresponding feature id sizing). The bounding boxes are defined and stored as two points in space, a lower and upper point. The optimal storage solution is use case defined, and as such there are two options provided `split` and `unified`.

| Geometry Type | Expected Feature ID Length|
|---------------|---------------------------|
| Image | Equal to Image Dimesions; typically equivalent to the `Cell Data` Attribute Matrix |
| Vertex | Equal to the Number of Vertices/Points; typically equivalent to the `Vertex Data` Attribute Matrix |
| Edge | Equal to the Number of Edges; ; typically equivalent to the `Edge Data` Attribute Matrix |
| Triangle | Equal to the Number of Triangles/Faces; typically equivalent to the `Face Data` Attribute Matrix |
| Quad | Equal to the Number of Quads/Faces; typically equivalent to the `Face Data` Attribute Matrix |

### Split Output

Two 3-component `float32` **DataArray**s. Min/Lower Bound and Max/Upper Bound.

The intended use case for `split` is primarily for output compatibility. By logically segmenting them users could:

- color min and max differently in visualization
- print the points in separate columns for distinction
- simplify parsing complexity for users who may want to adapt the bounding box representation to another format (primarily for Python-bindings) (e.g. min-x, max-x, min-y, max-y, min-z, max-z || adapting to non-standard bounding shapes)

### Unified Output

One 6-component `float32` **DataArray**. Bounds array in the format of min-x, min-y, min-z, max-x, max-y, max-z.

The intended use case for `unified` is primarily for simplicity of internal calculations. Essentially, this format is the result of appending the max array onto the min array. It is easier to pass around and parse one array within `simplnx` API's. For mainline `simplnx` filters this will be the expected/preferred input format.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
