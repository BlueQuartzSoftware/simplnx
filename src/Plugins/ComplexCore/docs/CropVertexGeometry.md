# Crop Geometry (Vertex)


## Group (Subgroup) ##

DREAM3D Review (Cropping/Cutting)

## Description ##

This **Filter** crops a **Vertex Geometry** by the given bounding box.  Unlike the cropping of an Image, it is unknown until run time how the **Geometry** will be changed by the cropping operation.  Therefore, this **Filter** requires that a new **Data Container** be created to contain the cropped **Vertex Geometry**.  This new **Data Container** will contain copies of any **Feature** or **Ensemble** **Attribute Matrices** from the original **Data Container**.  Additionally, all **Vertex** data will be copied, with tuples *removed* for any **Vertices** outside the bounding box.  The user must supply a name for the cropped **Data Container**, but all other copied objects (**Attribute Matrices** and **Data Arrays**) will retain the same names as the original source.

_Note:_ Since it cannot be known before run time how many **Vertices** will be removed during cropping, the new **Vertex Geometry** and all associated **Vertex** data to be copied will be initialized to have size 0.  Any **Feature** or **Ensemble** information will retain the same dimensions and size.     

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| Cropped Vertex Geometry | DataPath | Created VertexGeom Path |
| Vertex Data Name | String | Name of Vertex DataObject |
| Minimum Vertex Position | vec3 | the minimum x,y,z position |
| Maximum Vertex Position | vec3 | the maximum x,y,z position |
| Vertex Data Arrays to Crop | DataPath(s) | The path to all vertex data arrays to crop |

## Required Geometry ###

Vertex

## Required Objects ##

None

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| Cropped **Data Group** | CroppedDataContainer | N/A | N/A | **Data Container** holding the cropped **Vertex Geometry** and any copied **Attribute Matrices** and **Attribute Arrays** |

## Example Pipelines ##



## License & Copyright ##

Please see the description file distributed with this plugin.



## Python Filter Arguments

+ module: complex
+ Class Name: CropVertexGeometry
+ Displayed Name: Crop Geometry (Vertex)

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| cropped_geom | Cropped Vertex Geometry | Created VertexGeom path | complex.DataGroupCreationParameter |
| max_pos | Max Pos | Maximum vertex position | complex.VectorFloat32Parameter |
| min_pos | Min Pos | Minimum vertex position | complex.VectorFloat32Parameter |
| target_arrays | Vertex Data Arrays to crop | The complete path to all the vertex data arrays to crop | complex.MultiArraySelectionParameter |
| vertex_data_name | Vertex Data Name | Name of the vertex data AttributeMatrix | complex.DataObjectNameParameter |
| vertex_geom | Vertex Geometry to Crop | DataPath to target VertexGeom | complex.GeometrySelectionParameter |

