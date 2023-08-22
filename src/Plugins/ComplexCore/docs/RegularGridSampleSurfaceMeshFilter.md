# Sample Triangle Geometry on Regular Grid

## Group (Subgroup)

Sampling (Resolution)

## Description

This **Filter** "samples" a triangulated surface mesh on a rectilinear grid. The user can specify the number of **Cells** along the X, Y, and Z directions in addition to the resolution in each direction and origin to define a rectilinear grid.  The sampling is then performed by the following steps:

1. Determine the bounding box and **Triangle** list of each **Feature** by scanning all **Triangles** and noting the **Features** on either side of the **Triangle**
2. For each **Cell** in the rectilinear grid, determine which bounding box(es) they fall in (*Note:* the bounding box of multiple **Features** can overlap)
3. For each bounding box a **Cell** falls in, check against that **Feature's** **Triangle** list to determine if the **Cell** falls within that n-sided polyhedra (*Note:* if the surface mesh is conformal, then each **Cell** will only belong to one **Feature**, but if not, the last **Feature** the **Cell** is found to fall inside of will *own* the **Cell**)
4. Assign the **Feature** number that the **Cell** falls within to the *Feature Ids* array in the new rectilinear grid geometry

## Parameters

| Name | Type | Description |
|------|------|-------------|
| Dimensions | uint64 | Number of **Cells** along each axis |
| Resolution | float32 (3x) | The resolution values (dx, dy, dz) |
| Origin | float32 (3x) | The origin of the sampling volume |

## Required Geometry

Triangle

## Required Objects

| Type | Default Name | Type | Component Dimensions | Description |
|------|--------------|-------------|---------|-----|
| **Data Array** | Face Labels | int32 | (2) | Specifies which **Features** are on either side of each **Face**. |

## Created

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Image Geometry** | Image Geometry | N/A | N/A | Created **Image Geometry** name and *DataPath* |
| **Attribute Matrix** | Cell Data | Cell | N/A | Created **Cell Attribute Matrix** name |
| **Data Array** | Feature Ids | int32 | (1) | Specifies to which **Feature** each **Cell** belongs |

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM.3D Mailing Lists

If you need more help with a **Filter**, please consider asking your question on the [DREAM.3D Users Google group!](https://groups.google.com/forum/?hl=en#!forum/dream3d-users)


## Python Filter Arguments

+ module: complex
+ Class Name: RegularGridSampleSurfaceMeshFilter
+ Displayed Name: Sample Triangle Geometry on Regular Grid

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| cell_attribute_matrix_name | Cell Data Name | The name for the cell data Attribute Matrix within the Image geometry | complex.DataObjectNameParameter |
| dimensions | Dimensions (Voxels) | The dimensions of the created Image geometry | complex.VectorUInt64Parameter |
| feature_ids_array_name | Feature Ids Name | The name for the feature ids array in cell data Attribute Matrix | complex.DataObjectNameParameter |
| image_geom_path | Image Geometry | The name and path for the image geometry to be created | complex.DataGroupCreationParameter |
| length_unit | Length Units (For Description Only) | The units to be displayed below | complex.ChoicesParameter |
| origin | Origin | The origin of the created Image geometry | complex.VectorFloat32Parameter |
| spacing | Spacing | The spacing of the created Image geometry | complex.VectorFloat32Parameter |
| surface_mesh_face_labels_array_path | Face Labels | Array specifying which Features are on either side of each Face | complex.ArraySelectionParameter |
| triangle_geometry_path | Triangle Geometry | The geometry to be sampled onto grid | complex.GeometrySelectionParameter |

