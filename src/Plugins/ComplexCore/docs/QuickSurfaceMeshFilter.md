# Quick Surface Mesh


## Group (Subgroup) ##

Surface Meshing (Generation)

## Description ##

This **Filter** generates a **Triangle Geometry** from a grid **Geometry** (either an **Image Geometry** or a **RectGrid Geometry**) that represents a surface mesh of the present **Features**. The algorithm proceeds by creating a pair of **Triangles** for each face of the **Cell** where the neighboring **Cells** have a different **Feature** Id value. The meshing operation is extremely quick but can result in a surface mesh that is very "stair stepped". The user is encouraged to use a smoothing operation to reduce this "blockiness".

The user may choose any number of **Cell Attribute Arrays** to transfer to the created **Triangle Geometry**. The **Faces** will gain the values of the **Cells** from which they were created.  Currently, the **Filter** disallows the transferring of data that has a *multi-dimensional* component dimensions vector.  For example, scalar values and vector values are allowed to be transferred, but N x M matrices cannot currently be transferred. 

For more information on surface meshing, visit the tutorial.

---------------

![Example Quick Mesh Output](Images/QuickSurfaceMeshOutput.png)

---------------

## Parameters ##

None

## Required Geometry ##

Image/RectGrid

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Cell Attribute Array** | FeatureIds | int32_t | (1) | Specifies to which **Feature** each **Cell** belongs |
| Any **Cell Attribute Array** |  None | Any | Any | Specifies which **Cell Attribute Arrays** to transfer to the created **Triangle Geometry** |

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Data Container** | TriangleDataContainer | N/A | N/A | Created **Data Container** name with a **Triangle Geometry** |
| **Attribute Matrix** | VertexData | Vertex | N/A | Created **Vertex Attribute Matrix** name  |
| **Vertex Attribute Array** | NodeTypes | int8_t | (1) | Specifies the type of node in the **Geometry** |
| **Attribute Matrix** | FaceData | Face | N/A | Created **Face Attribute Matrix** name  |
| **Face Attribute Array** | FaceLabels | int32_t | (2) | Specifies which **Features** are on either side of each **Face** |
| **Attribute Matrix** | FaceFeatureData | Face Feature | N/A | **Feature Attribute Matrix** of the created *Face Labels* |
| Any **Face Attribute Array** | None | Any | Any | The set of transferred **Cell Attribute Arrays** |

## Example Pipelines ##

+ (01) SmallIN100 Quick Mesh

## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: complex
+ Class Name: QuickSurfaceMeshFilter
+ Displayed Name: Quick Surface Mesh

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| face_data_group_name | Face Data [AttributeMatrix] | The complete path to the DataGroup where the Face Data of the Triangle Geometry will be created | complex.DataObjectNameParameter |
| face_feature_attribute_matrix_name | Face Feature Data [AttributeMatrix] | The complete path to the DataGroup where the Feature Data will be stored. | complex.DataObjectNameParameter |
| face_labels_array_name | Face Labels | The complete path to the Array specifying which Features are on either side of each Face in the Triangle Geometry | complex.DataObjectNameParameter |
| feature_ids_path | Feature Ids | The complete path to the Array specifying which Feature each Cell belongs to | complex.ArraySelectionParameter |
| fix_problem_voxels | Attempt to Fix Problem Voxels | See help page. | complex.BoolParameter |
| generate_triple_lines | Generate Triple Lines | Experimental feature. May not work. | complex.BoolParameter |
| grid_geometry_data_path | Grid Geometry | The complete path to the Grid Geometry from which to create a Triangle Geometry | complex.GeometrySelectionParameter |
| node_types_array_name | NodeType | The complete path to the Array specifying the type of node in the Triangle Geometry | complex.DataObjectNameParameter |
| selected_data_array_paths | Attribute Arrays to Transfer | The paths to the Arrays specifying which Cell Attribute Arrays to transfer to the created Triangle Geometry | complex.MultiArraySelectionParameter |
| triangle_geometry_name | Created Triangle Geometry | The name of the created Triangle Geometry | complex.DataGroupCreationParameter |
| vertex_data_group_name | Vertex Data [AttributeMatrix] | The complete path to the DataGroup where the Vertex Data of the Triangle Geometry will be created | complex.DataObjectNameParameter |

