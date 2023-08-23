# Generate GBCD Pole Figure 


## Group (Subgroup) ##

IO (Output)

## Description ##

This **Filter** creates a pole figure from the Grain Boundary Character Distribution (GBCD) data. The user must select the relevant phase for which to generate the pole figure by entering the *phase index*.

-----

![Regular Grid Visualization of the Small IN100 GBCD results](Images/Small_IN00_GBCD_RegularGrid.png)

![Using ParaView's Threshold filter + Cells to Points + Delaunay2D Filters](Images/Small_IN100_GBCD_Delaunay2D.png)

-----

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| Phase of Interest | int32 | Index of the **Ensemble** for which to plot the pole figure |
| Misorientation Axis-Angle | float (4x) | Axis-Angle pair values for drawing GBCD  in the order of Axis (Degrees) then Axis (Normalized)|
| Output Image Dimension | int32 | The value to use for the x and y dimensions of the created Image Geometry |

## Required Geometry ##

Triangle

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Ensemble Attribute Array** | GBDC | double | (n,m,l,o,q,r) | 5 parameter GBCD data. The 6<sup>th</sup> component is used internally to track the northern vs. southern hemisphere of the Lambert sphere |
| Crystal Structure | CrystalStructures | Enumeration | 1 | Crystal structure for GBCD. Currently supports from Hexagonal-High 6/mmm or Cubic-High m-3m symmetries |

## Created Objects ##

| Kind | Default Name | Description |
|------|--------------|-------------|
| Image Geometry | [ImageGeometry] | The Image Geometry to be created |
| Cell Attribute Matrix | Cell Data | The name of the cell attribute matrix created for the Image Geometry |
| Cell Intensity Array | Intensity | The data array to be created from the pole figures and stored in the Image Geometry cell attribute matrix |

## Example Pipelines ##

+ (04) SmallIN100 GBCD

## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: OrientationAnalysis
+ Class Name: GenerateGBCDPoleFigureFilter
+ Displayed Name: Generate GBCD Pole Figure

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| cell_attribute_matrix_name | Cell Attribute Matrix Name | The name of the cell attribute matrix for the created image geometry | complex.DataObjectNameParameter |
| cell_intensity_array_name | Cell MRD Array Name | The name of the created cell intensity data array | complex.DataObjectNameParameter |
| crystal_structures_array_path | Crystal Structures | Enumeration representing the crystal structure for each Ensemble | complex.ArraySelectionParameter |
| gbcd_array_path | Input GBCD | 5 parameter GBCD data. The 6th component is used internally to track the northern vs. southern hemisphere of the Lambert sphere | complex.ArraySelectionParameter |
| image_geometry_name | Image Geometry | The path to the created image geometry | complex.DataGroupCreationParameter |
| misorientation_rotation | Misorientation Angle-Axis | Angle-Axis values for drawing GBCD | complex.VectorFloat32Parameter |
| output_image_dimension | Output Image Dimension | The value to use for the dimensions for the image geometry | complex.Int32Parameter |
| phase_of_interest | Phase of Interest | Index of the Ensemble for which to plot the pole figure | complex.Int32Parameter |

