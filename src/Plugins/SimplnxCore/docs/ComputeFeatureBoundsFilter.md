# Compute Feature Bounding Boxes

## Group (Subgroup)

Geometry

## Description

**Warning**: *Potential Runtime Error* - It is expected that the max feature id plus one (`max_feature_id_value` + 1) is equal to or less than the number of tuples in the supplied feature Attribute Matrix. This cannot be checked in preflight and will terminate the pipeline if encountered.

This filter calculates the bounding boxes for each **Feature** given Feature Ids and Geometry (refer to table below for supported geometry types and their corresponding feature id sizing). A **Feature** is a contiguous region of cells (voxels, vertices, edges, or faces) that share the same Feature Id; a **Cell** is the smallest addressable element of the geometry. _**This filter does output `NaN`s for empty features**_, cases where a point can not be associated to a feature. The bounding boxes are defined and stored as two points in space, a lower and upper point. The optimal storage solution is use case defined, and as such there are two options provided `split` and `unified`.

The bounding box values are stored in **physical coordinates** (the same length units as the geometry's origin and spacing), not cell/voxel indices. For an **Image Geometry** each coordinate is computed as `origin + index * spacing`; for node-based geometries (Vertex, Edge, Triangle, Quad) the vertex coordinates are used directly.

### Output Array(s) Type

The *Output Array(s) Type* parameter controls how the bounding box data is stored in the output:

- **Split [0]**: Produces two separate 3-component `float32` arrays — one for the minimum (lower) bound and one for the maximum (upper) bound of each feature's bounding box. Best for visualization and cases where min and max bounds need to be handled independently.
- **Unified [1]**: Produces a single 6-component `float32` array containing all bounds data in the format min-x, min-y, min-z, max-x, max-y, max-z. Best for passing bounding box data to other simplnx filters and internal calculations.

| Geometry Type | Expected Feature ID Length|
|---------------|---------------------------|
| Image | Equal to the Image Dimensions; typically equivalent to the `Cell Data` Attribute Matrix |
| Vertex | Equal to the Number of Vertices/Points; typically equivalent to the `Vertex Data` Attribute Matrix |
| Edge | Equal to the Number of Edges; typically equivalent to the `Edge Data` Attribute Matrix |
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

### Edge Geometry Nuances

Producing an edge geometry for the bounding boxes has a couple nuances that aren't very intuitive, these will be covered here. Firstly, the output edge geometry may **NOT** contain all features that are in the input geometry. For a feature to be included it must meet two conditions:

- The bounding box must not contain any NANs
- The feature id must be greater than or equal to 0

This is most relevant if you have empty features in the input geometry or you have invalid feature ids (-1). This is remedied by the feature ids created at the edge (cell) data, these map the edges making up the bounding boxes to the feature they originate from. With empty features, this will cause gaps in the sequence (eg with 3 being an empty feature the edge feature ids would follow a 1,2,4,5 sequence). This is important to note because the user may wish to create a Feature Attribute Matrix by creating an Attribute Matrix equivalent to `number of edges / 12`, but this would only be true for the case in which the values in array are consecutive in order (ie there are no empty features).

Lastly, we will peel back the covers on how the geometry is constructed in the case that the user needs to parse or manipulate the data within it. Each bound box in the geometry is extrapolated from a maximum and minimum point. The points are constructed from every combination in the following order:

```console
| Index |     Vertex Point      |
|-------|-----------------------|
|   0   | {min-X, min-Y, min-Z} |
|   1   | {max-X, min-Y, min-Z} |
|   2   | {max-X, max-Y, min-Z} |
|   3   | {min-X, max-Y, min-Z} |
|   4   | {min-X, min-Y, max-Z} |
|   5   | {max-X, min-Y, max-Z} |
|   6   | {max-X, max-Y, max-Z} |
|   7   | {min-X, max-Y, max-Z} |
```

The edges for each bounding box are 12 in number and constructed in following order:

```console
| Index | Vertex Indices |
|-------|----------------|
|   0   |     {0, 1}     |
|   1   |     {1, 2}     |
|   2   |     {2, 3}     |
|   3   |     {3, 4}     |
|   4   |     {4, 5}     |
|   5   |     {5, 6}     |
|   6   |     {6, 7}     |
|   7   |     {7, 4}     |
|   8   |     {0, 4}     |
|   9   |     {1, 5}     |
|  10   |     {2, 6}     |
|  11   |     {3, 7}     |
```

Since edges are the cell level data in edge geometries, the feature ids map to the edges. This means that the feature ids array will always contain 11 more consecutive instances of the same feature from when it first appears (12 total). To know the number of features in the edge geom, just divide the number of edges by 12.

### Required Input Sources

This filter requires a **Cell Feature Ids** array, typically produced by a segmentation filter such as [Segment Features (Scalar)](ScalarSegmentFeaturesFilter.md) or one of the misorientation-based segmentation filters in the OrientationAnalysis plugin. The selected **Feature Data Attribute Matrix** is where the output bounds arrays are created.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
