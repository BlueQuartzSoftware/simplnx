# Remove Flagged Vertices

## Group (Subgroup)

Data Cleanup (Geometry)

## Description

This **Filter** removes **Vertices** from the supplied **Vertex Geometry** that are flagged as **TRUE** by a boolean mask array. A new reduced **Vertex Geometry** is created that contains all the remaining **Vertices**. It is unknown until run time how many **Vertices** will be removed from the **Geometry**. Therefore, this **Filter** creates a new **Vertex Geometry** (at the user-supplied path) to hold the reduced result rather than modifying the input in place. The selected **Feature Attribute Matrix** is cleared by default. Other **Attribute Matrices**, including **Ensemble** data, are copied unchanged. Additionally, *all **Vertex** data will be copied*, with tuples *removed* for any **Vertices** removed by the **Filter**. The user must supply a name for the reduced **Vertex Geometry**, but all other copied objects (**Attribute Matrices** and **Attribute Arrays**) will retain the same names as the original source, even when those names contain the source geometry's name. With clearing off, copying `Geom/Geom Feature Data/Geom Values` to `Geom Out` creates `Geom Out/Geom Feature Data/Geom Values`.

*Note:* Preflight uses the source vertex count for the new geometry and its **Vertex** data. Execution resizes them to the number of retained vertices.

## Feature Attribute Matrix

A **Feature** is a group of vertices with the same Feature Id. A **Feature Attribute Matrix** stores one row for each Feature Id. Removing vertices changes feature membership. Feature sizes, shapes, centroids, neighbors, and average values can change. Treat every feature value computed before this filter as invalid for the reduced geometry.

Renumbering does not recompute these values. *Clear Feature Attribute Matrix* is on by default. It recreates the selected matrix with no arrays. Numeric arrays, string arrays, and **NeighborLists** are removed from the output matrix. Recompute any arrays needed later. A later filter that selects a cleared array fails preflight until that array is created again. Pipelines converted from DREAM.3D 6.x start with Clear off, because the old filter copied the Feature data unchanged.

### Clearing and Renumbering Modes

| Clear Feature Attribute Matrix | Renumber Features | Result |
|---|---|---|
| On | On | Remaining Feature Ids become 1 through N. The empty matrix has N+1 rows, including row 0. Background Id 0 stays 0. |
| On | Off (default) | The empty matrix keeps the source tuple shape. Feature Ids stay unchanged. Rows for removed features are unused. |
| Off | Hidden and ignored | All Attribute Matrices other than the Vertex Attribute Matrix are copied unchanged. Feature Ids stay unchanged. Use this mode for raw data without features or for debugging. Copied feature values still describe the original geometry. |

*Feature Attribute Matrix*, *Renumber Features*, and *Vertex Feature Ids* are shown only when *Clear Feature Attribute Matrix* is on. Both selection paths are required when Clear is on, even when Renumber is off. If the Vertex Geometry has no Feature data, turn Clear off. When Clear is off, the filter ignores both selections and any saved Renumber setting.

Select an `int32` array with one component for *Vertex Feature Ids*. When Clear and Renumber are both on, *Vertex Feature Ids* must also be in the geometry's Vertex Attribute Matrix and have one tuple per vertex. Then every source Feature Id must be nonnegative and smaller than the source Feature Attribute Matrix tuple count.

### Preflight Information and Errors

Preflight lists removed arrays under **Cleared Feature Data**. With clearing off, **Feature Data Not Cleared** explains that copied feature values are invalid for the reduced geometry. **Ensemble** data is always copied unchanged. The source geometry and its Feature Attribute Matrix stay unchanged.

When Clear is on, the selected Feature Attribute Matrix must be a direct child of the input Vertex Geometry. It must not be the geometry's Vertex Attribute Matrix. Preflight can report these errors:

- **-67421**: The Feature Attribute Matrix is not a direct child of the selected Vertex Geometry. Select a matrix directly under that geometry.
- **-67422**: The selected matrix is the geometry's Vertex Attribute Matrix. Select the matrix that holds Feature data, or turn Clear off.
- **-67423**: Renumber is on, and *Vertex Feature Ids* is not in the geometry's Vertex Attribute Matrix. Select the array from that matrix.
- **-67424**: Renumber is on, and *Vertex Feature Ids* does not have one tuple per vertex. Select an array with one tuple per vertex.

### Required Input Sources

- **Mask** -- a single-component boolean (or `uint8`) array, one value per vertex, where **TRUE** marks the vertices to remove. Typically produced by [Multi-Threshold Objects](MultiThresholdObjectsFilter.md).

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
