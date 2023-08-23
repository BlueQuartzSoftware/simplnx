# Align Geometries

## Group (Subgroup) ##

DREAM3DReview (DREAM3DReview)

## Description ##

This **Filter** will align 2 Geometry objects using 1 of several alignment methods:

* Centroid
* Origin
* XY Min Plane
* XY Max Plane
* XZ Min Plane
* XZ Max Plane
* YZ Min Plane
* YZ Max Plane

The input geometries can be of any type. The *Moving* geometry is moved in space to the *Target* geometry.

## Parameters ##

| Name | Type | Description |
|------|------|------|
| Method | Integer | 0=Centroid, 1=Origin, 2-7 for the various planes |

## Required Geometry ##

Required Geometry Type -or- Not Applicable

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|-------------|---------|-----|
| **Data Container** | Moving Geometry | Must have a geometry | N/A | The Geometry that is going to be moved |
| **Data Container** | Target Geometry |Must have a geometry | N/A | The Target Geometry that the moving is going to be aligned to. |

## Created Objects ##

None. The operation is done in place

## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: complex
+ Class Name: AlignGeometries
+ Displayed Name: Align Geometries

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| alignment_type | Alignment Type | The type of alignment to perform (Origin or Centroid. | complex.ChoicesParameter |
| moving_geometry | Moving Geometry | The geometry that will be moved. | complex.GeometrySelectionParameter |
| target_geometry | Fixed Geometry | The geometry that does *not* move. | complex.GeometrySelectionParameter |

