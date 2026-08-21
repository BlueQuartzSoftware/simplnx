# Extract Vertex Geometry

## Group (Subgroup)

Core Filters (Geometry)

## Description

This filter will extract all the voxel centers of an **Image Geometry** or a **Rectilinear Grid Geometry**
into a new **Vertex Geometry**. The user is given the option to copy or move cell arrays over to the
newly created **Vertex Geometry**. The user can also supply a mask array which has the effect of only
creating a vertex if the mask value = TRUE.

![Example showing the use of a Mask array to only generate specific points.](Images/ExtractVertexGeometry_1.png)

### Required Input Sources

- **Image Geometry** or **Rectilinear Grid Geometry** -- the grid whose voxel/cell centers will be extracted as vertices.

### Array Handling

The *Array Handling* parameter controls how the selected cell arrays are transferred to the new **Vertex Geometry**:

- **Copy Attribute Arrays [0]**: Creates copies of the selected arrays in the new vertex geometry, leaving the originals intact in the source geometry.
- **Move Attribute Arrays [1]**: Moves the selected arrays to the new vertex geometry, removing them from their original location.

% Auto generated parameter table will be inserted here

## Example Pipelines

PrebuiltPipelines/Examples/Extract Vertex Geometry.json

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
